// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioListener.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/DefaultAudioDestinationNode.h"
#include "LabSound/core/OfflineAudioDestinationNode.h"
#include "LabSound/core/OscillatorNode.h"
#include "LabSound/core/AudioHardwareSourceNode.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioDestination.h"
#include "internal/Assertions.h"

#include <stdio.h>
#include <queue>
#include <assert.h>

namespace lab
{

const uint32_t lab::AudioContext::maxNumberOfChannels = 32;

// Constructor for realtime rendering
AudioContext::AudioContext(bool isOffline)
{
    m_isOfflineContext = isOffline;
    m_listener.reset(new AudioListener());
}

AudioContext::~AudioContext()
{
    LOG("Begin AudioContext::~AudioContext()");

    ContextGraphLock g(this, "AudioContext::~AudioContext");
    stop(g);

    // Join update thread
    updateThreadShouldRun = false;
    if (graphUpdateThread.joinable())
    {
        graphUpdateThread.join();
    }

#if USE_ACCELERATE_FFT
    FFTFrame::cleanup();
#endif
    
    ASSERT(!m_isInitialized);
    ASSERT(m_isStopScheduled);
    ASSERT(!m_automaticPullNodes.size());
    ASSERT(!m_renderingAutomaticPullNodes.size());

    LOG("Finish AudioContext::~AudioContext()");
}

void AudioContext::lazyInitialize()
{
    if (!m_isInitialized)
    {
        // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
        ASSERT(!m_isAudioThreadFinished);
        if (!m_isAudioThreadFinished)
        {
            if (m_destinationNode.get())
            {
                m_destinationNode->initialize();

                if (!isOfflineContext())
                {
                    // This starts the audio thread. The destination node's provideInput() method will now be called repeatedly to render audio.
                    // Each time provideInput() is called, a portion of the audio stream is rendered. Let's call this time period a "render quantum".
                    m_destinationNode->startRendering();
                }

                graphUpdateThread = std::thread(&AudioContext::update, this);
            }
            else
            {
                LOG_ERROR("m_destinationNode not specified");
            }
            m_isInitialized = true;
        }
    }
}

void AudioContext::clear()
{
    // Audio thread is dead. Nobody will schedule node deletion action. Let's do it ourselves.
    if (m_destinationNode.get()) m_destinationNode.reset();
}

void AudioContext::uninitialize(ContextGraphLock& g)
{
    LOG("AudioContext::uninitialize()");

    if (!m_isInitialized)
        return;

    // This stops the audio thread and all audio rendering.
    m_destinationNode->uninitialize();

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    m_isAudioThreadFinished = true;
    
    updateAutomaticPullNodes(); // added for the case where an OfflineAudioDestinationNode needs to update the graph

    m_isInitialized = false;
}

bool AudioContext::isInitialized() const
{
    return m_isInitialized;
}

void AudioContext::incrementConnectionCount()
{
    ++m_connectionCount;
}

void AudioContext::stop(ContextGraphLock & g)
{
    if (m_isStopScheduled) return;
    m_isStopScheduled = true;
    uninitialize(g);
    clear();
    LOG("AudioContext::stop()");
}

void AudioContext::holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode> node)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    automaticSources.push_back(node);
}

void AudioContext::handleAutomaticSources()
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    for (auto i = automaticSources.begin(); i != automaticSources.end(); ++i)
    {
        if ((*i)->hasFinished())
        {
            pendingNodeConnections.emplace(*i, std::shared_ptr<AudioNode>(), ConnectionType::Disconnect, 0, 0); // order? 
            i = automaticSources.erase(i);
            if (i == automaticSources.end()) break;
        }
    }
}

void AudioContext::handlePreRenderTasks(ContextRenderLock& r)
{
    ASSERT(r.context());

    // At the beginning of every render quantum, try to update the internal rendering graph state (from main thread changes).
    AudioSummingJunction::handleDirtyAudioSummingJunctions(r);
    updateAutomaticPullNodes();
}

void AudioContext::handlePostRenderTasks(ContextRenderLock& r)
{
    ASSERT(r.context());

    AudioSummingJunction::handleDirtyAudioSummingJunctions(r);

    updateAutomaticPullNodes();
    handleAutomaticSources();
}

void AudioContext::connect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx, uint32_t srcIdx)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    if (srcIdx > source->numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs");
    if (destIdx > destination->numberOfInputs()) throw std::out_of_range("Input index greater than available inputs");
    pendingNodeConnections.emplace(destination, source, ConnectionType::Connect, destIdx, srcIdx);
}

void AudioContext::disconnect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx, uint32_t srcIdx)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    if (source && srcIdx > source->numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs");
    if (destination && destIdx > destination->numberOfInputs()) throw std::out_of_range("Input index greater than available inputs");
    pendingNodeConnections.emplace(destination, source, ConnectionType::Disconnect, destIdx, srcIdx);
}

void AudioContext::connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, uint32_t index)
{
    if (!param) throw std::invalid_argument("No parameter specified");
    if (index >= driver->numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs on the driver");
    pendingParamConnections.push(std::make_tuple(param, driver, index));
}

void AudioContext::update()
{
    LOG("Begin UpdateGraphThread");

    while (updateThreadShouldRun && !m_isStopScheduled)
    {

        {
            ContextGraphLock gLock(this, "context::update");
            
            std::lock_guard<std::mutex> lock(automaticSourcesMutex);

            // Verify that we've acquired the lock, and check again 5 ms later if not
            if (!gLock.context())
            {
                if (!m_isOfflineContext) std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            const double now = currentTime();

            // Satisfy parameter connections
            while (!pendingParamConnections.empty())
            {
                auto connection = pendingParamConnections.front();
                pendingParamConnections.pop();
                AudioParam::connect(gLock, std::get<0>(connection), std::get<1>(connection)->output(std::get<2>(connection)));
            }

            std::vector<PendingConnection> skippedConnections;

            // Satisfy node connections
            while (!pendingNodeConnections.empty())
            {
                auto connection = pendingNodeConnections.top();
                pendingNodeConnections.pop();

                switch (connection.type)
                {
                case ConnectionType::Connect:
                {
                    // requue this node if the scheduled time is > 100ms away
                    if (connection.destination && connection.destination->isScheduledNode())
                    {
                        AudioScheduledSourceNode * node = dynamic_cast<AudioScheduledSourceNode*>(connection.destination.get());
                        if (node->startTime() > now + 0.1)
                        {
                            pendingNodeConnections.pop(); // pop from current queue
                            skippedConnections.push_back(connection); // save for later
                            continue;
                        }
                    }

                    connection.source->m_disconnectSchedule = -1.f;
                    connection.source->m_connectSchedule = 0.f;

                    AudioNodeInput::connect(gLock, connection.destination->input(connection.destIndex), connection.source->output(connection.srcIndex));
                }
                break;

                case ConnectionType::Disconnect:
                {
                    connection.type = ConnectionType::FinishDisconnect;
                    skippedConnections.push_back(connection); // save for later
                    if (connection.source)
                    {
                        // if source and destination are specified, then we don't ramp out the destination
                        connection.source->m_disconnectSchedule = 1.f;
                    }
                    else if (connection.destination)
                    {
                        // this case is a disconnect where source is nothing, and destination is something
                        // probably this case should be disallowed because we have to study it to find out
                        // if it is any different than a source with no destination. Answer: it's the same. source or dest by itself means disconnect all
                        connection.destination->m_disconnectSchedule = 1.f;
                    }
                }
                break;

                case ConnectionType::FinishDisconnect:
                {
                    if (connection.source && connection.destination)
                    {
                        AudioNodeInput::disconnect(gLock, connection.destination->input(connection.destIndex), connection.source->output(connection.srcIndex));
                    }
                    else if (connection.destination)
                    {
                        for (size_t out = 0; out < connection.destination->numberOfOutputs(); ++out)
                        {
                            auto output = connection.destination->output(out);
                            if (!output) continue;

                            AudioNodeOutput::disconnectAll(gLock, output);
                        }
                    }
                    else if (connection.source)
                    {
                        for (size_t out = 0; out < connection.source->numberOfOutputs(); ++out)
                        {
                            auto output = connection.source->output(out);
                            if (!output) continue;

                            AudioNodeOutput::disconnectAll(gLock, output);
                        }
                    }
                }
                break;
                }
            }

            // We have unsatisfied scheduled nodes, so next time the thread ticks we can re-check them 
            for (auto & sc : skippedConnections)
                pendingNodeConnections.push(sc);
        }

        if (!m_isOfflineContext)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // fixed 10ms timestep 
        }
    }

    LOG("End UpdateGraphThread");
}

void AudioContext::addAutomaticPullNode(std::shared_ptr<AudioNode> node)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    if (m_automaticPullNodes.find(node) == m_automaticPullNodes.end())
    {
        m_automaticPullNodes.insert(node);
        m_automaticPullNodesNeedUpdating = true;
    }
}

void AudioContext::removeAutomaticPullNode(std::shared_ptr<AudioNode> node)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    auto it = m_automaticPullNodes.find(node);
    if (it != m_automaticPullNodes.end())
    {
        m_automaticPullNodes.erase(it);
        m_automaticPullNodesNeedUpdating = true;
    }
}

void AudioContext::updateAutomaticPullNodes()
{
    if (m_automaticPullNodesNeedUpdating)
    {
        std::lock_guard<std::mutex> lock(automaticSourcesMutex);

        // Copy from m_automaticPullNodes to m_renderingAutomaticPullNodes.
        m_renderingAutomaticPullNodes.resize(m_automaticPullNodes.size());

        unsigned j = 0;
        for (auto i = m_automaticPullNodes.begin(); i != m_automaticPullNodes.end(); ++i, ++j)
        {
            m_renderingAutomaticPullNodes[j] = *i;
        }

        m_automaticPullNodesNeedUpdating = false;
    }
}

void AudioContext::processAutomaticPullNodes(ContextRenderLock & r, size_t framesToProcess)
{
    for (unsigned i = 0; i < m_renderingAutomaticPullNodes.size(); ++i)
        m_renderingAutomaticPullNodes[i]->processIfNecessary(r, framesToProcess);
}

void AudioContext::setDestinationNode(std::shared_ptr<AudioDestinationNode> node) 
{ 
    m_destinationNode = node; 
}

std::shared_ptr<AudioDestinationNode> AudioContext::destination() 
{ 
    return m_destinationNode; 
}

bool AudioContext::isOfflineContext() 
{ 
    return m_isOfflineContext; 
}

size_t AudioContext::currentSampleFrame() const 
{ 
    return m_destinationNode->currentSampleFrame(); 
}

double AudioContext::currentTime() const 
{  
    return m_destinationNode->currentTime();
}

float AudioContext::sampleRate() const 
{  
    ASSERT(m_destinationNode);
    return m_destinationNode->sampleRate();
}

AudioListener & AudioContext::listener() 
{ 
    return *m_listener.get(); 
}

unsigned long AudioContext::activeSourceCount() const 
{ 
    return static_cast<unsigned long>(m_activeSourceCount); 
}

void AudioContext::startRendering()
{
    destination()->startRendering();
}

void AudioContext::incrementActiveSourceCount()
{
    ++m_activeSourceCount;
}

void AudioContext::decrementActiveSourceCount()
{
    --m_activeSourceCount;
}

} // End namespace lab
