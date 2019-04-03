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

#include "readerwriterqueue/readerwriterqueue.h"

#include <stdio.h>
#include <queue>
#include <assert.h>

namespace lab
{

struct AudioContext::Internals
{
    Internals(bool a) : autoDispatchEvents(a) {}
    ~Internals() = default;

    moodycamel::ReaderWriterQueue<std::function<void()>> enqueuedEvents;
    bool autoDispatchEvents;
};

const uint32_t lab::AudioContext::maxNumberOfChannels = 32;

// Constructor for realtime rendering
AudioContext::AudioContext(bool isOffline, bool autoDispatchEvents) : m_isOfflineContext(isOffline)
{
    m_internal.reset(new AudioContext::Internals(autoDispatchEvents));
    m_listener.reset(new AudioListener());
}

AudioContext::~AudioContext()
{
    // LOG can block.
    // LOG("Begin AudioContext::~AudioContext()");

    if (!isOfflineContext()) graphKeepAlive = 0.25f;

    updateThreadShouldRun = false;
    if (graphUpdateThread.joinable())
    {
        cv.notify_all();
        graphUpdateThread.join();
    }

    //std::unique_lock<std::mutex> lk(m_updateMutex); // do we need this?

    uninitialize();

    // Audio thread is dead. Nobody will schedule node deletion action. Let's do it ourselves.
    if (m_destinationNode.get()) m_destinationNode.reset();

#if USE_ACCELERATE_FFT
    FFTFrame::cleanup();
#endif

    ASSERT(!m_isInitialized);
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

                graphKeepAlive = 0.25f; // pump the graph for the first 0.25 seconds
                graphUpdateThread = std::thread(&AudioContext::update, this);

                if (!isOfflineContext())
                {
                    // This starts the audio thread. The destination node's provideInput() method will now be called repeatedly to render audio.
                    // Each time provideInput() is called, a portion of the audio stream is rendered. Let's call this time period a "render quantum".
                    m_destinationNode->startRendering();
                }

                cv.notify_all();
            }
            else
            {
                LOG_ERROR("m_destinationNode not specified");
            }
            m_isInitialized = true;
        }
    }
}

void AudioContext::uninitialize()
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

void AudioContext::holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode> node)
{
    std::lock_guard<std::mutex> lock(m_updateMutex);
    automaticSources.push_back(node);
}

void AudioContext::handleAutomaticSources()
{
    std::lock_guard<std::mutex> lock(m_updateMutex);
    for (auto i = automaticSources.begin(); i != automaticSources.end(); ++i)
    {
        if ((*i)->hasFinished())
        {
            pendingNodeConnections.emplace(*i, std::shared_ptr<AudioNode>(), ConnectionType::Disconnect, 0, 0);
            i = automaticSources.erase(i);
            if (i == automaticSources.end()) break;
        }
    }
}

void AudioContext::handlePreRenderTasks(ContextRenderLock & r)
{
    ASSERT(r.context());

    // At the beginning of every render quantum, try to update the internal rendering graph state (from main thread changes).
    AudioSummingJunction::handleDirtyAudioSummingJunctions(r);
    updateAutomaticPullNodes();
}

void AudioContext::handlePostRenderTasks(ContextRenderLock & r)
{
    ASSERT(r.context());

    AudioSummingJunction::handleDirtyAudioSummingJunctions(r);

    updateAutomaticPullNodes();
    handleAutomaticSources();
}

void AudioContext::connect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx, uint32_t srcIdx)
{
    if (!destination) throw std::runtime_error("Cannot connect to null destination");
    if (!destination) throw std::runtime_error("Cannot connect from null source");
    std::lock_guard<std::mutex> lock(m_updateMutex);
    if (srcIdx > source->numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs");
    if (destIdx > destination->numberOfInputs()) throw std::out_of_range("Input index greater than available inputs");
    pendingNodeConnections.emplace(destination, source, ConnectionType::Connect, destIdx, srcIdx);
    cv.notify_all();
}

void AudioContext::disconnect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx, uint32_t srcIdx)
{
    std::lock_guard<std::mutex> lock(m_updateMutex);
    if (source && srcIdx > source->numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs");
    if (destination && destIdx > destination->numberOfInputs()) throw std::out_of_range("Input index greater than available inputs");
    pendingNodeConnections.emplace(destination, source, ConnectionType::Disconnect, destIdx, srcIdx);
    cv.notify_all();
}

void AudioContext::connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, uint32_t index)
{
    if (!param) throw std::invalid_argument("No parameter specified");
    if (index >= driver->numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs on the driver");
    pendingParamConnections.push(std::make_tuple(param, driver, index));
    cv.notify_all();
}

void AudioContext::update()
{
    LOG("Begin UpdateGraphThread");

    const float frameSizeMs = (sampleRate() / (float)AudioNode::ProcessingSizeInFrames) / 1000.f; // = ~0.345ms @ 44.1k/128
    const float graphTickDurationMs = frameSizeMs * 16; // = ~5.5ms
    const int graphTickDurationUs = static_cast<int>(graphTickDurationMs * 1000.f);  // = ~5550us

    // graphKeepAlive keeps the thread alive momentarily (letting tail tasks
    // finish) even updateThreadShouldRun has been signaled.
    while (updateThreadShouldRun || graphKeepAlive > 0)
    {
        // A `unique_lock` automatically acquires a lock on construction. The purpose of
        // this mutex is to synchronize updates to the graph from the main thread,
        // primarily through `connect(...)` and `disconnect(...)`.
        std::unique_lock<std::mutex> lk;

        if (!m_isOfflineContext)
        {
            lk = std::unique_lock<std::mutex>(m_updateMutex);
            // A condition variable is used to notify this thread that a graph update is pending
            // in one of the queues.

            // graph needs to tick to complete
            if ((currentTime() + graphKeepAlive) > currentTime())
            {
                cv.wait_until(lk, std::chrono::steady_clock::now() + std::chrono::microseconds(graphTickDurationUs));
            }
            else
            {
                // otherwise wait for someone to connect or disconnect something
                cv.wait(lk);
            }
        }

        if (m_internal->autoDispatchEvents)
            dispatchEvents();

        {
            ContextGraphLock gLock(this, "AudioContext::Update()");

            const double now = currentTime();
            const float delta = static_cast<float>(now - lastGraphUpdateTime);
            lastGraphUpdateTime = static_cast<float>(now);
            graphKeepAlive -= delta;

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
                    // requeue this node if the scheduled time is > 100ms away
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

                    connection.source->scheduleConnect();

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
                        connection.source->scheduleDisconnect();
                    }
                    else if (connection.destination)
                    {
                        // this case is a disconnect where source is nothing, and destination is something
                        // probably this case should be disallowed because we have to study it to find out
                        // if it is any different than a source with no destination. Answer: it's the same. source or dest by itself means disconnect all
                        connection.destination->scheduleDisconnect();
                    }
                    graphKeepAlive = updateThreadShouldRun ? connection.duration : graphKeepAlive;
                }
                break;

                // @TODO disconnect should occur not in the next quantum, but when node->disconnectionReady() is true
                case ConnectionType::FinishDisconnect:
                {
                    if (connection.duration > 0)
                    {
                        connection.duration -= delta;
                        skippedConnections.push_back(connection);
                        continue;
                    }

                    if (connection.source && connection.destination)
                    {
                        AudioNodeInput::disconnect(gLock, connection.destination->input(connection.destIndex), connection.source->output(connection.srcIndex));
                    }
                    else if (connection.destination)
                    {
                        for (unsigned int out = 0; out < connection.destination->numberOfOutputs(); ++out)
                        {
                            auto output = connection.destination->output(out);
                            if (!output) continue;

                            AudioNodeOutput::disconnectAll(gLock, output);
                        }
                    }
                    else if (connection.source)
                    {
                        for (unsigned int out = 0; out < connection.source->numberOfOutputs(); ++out)
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

            // We have incompletely connected nodes, so next time the thread ticks we can re-check them
            for (auto & sc : skippedConnections)
            {
                pendingNodeConnections.push(sc);
            }

        }

        if (lk.owns_lock()) lk.unlock();
    }

    LOG("End UpdateGraphThread");
}

void AudioContext::addAutomaticPullNode(std::shared_ptr<AudioNode> node)
{
    std::lock_guard<std::mutex> lock(m_updateMutex);
    if (m_automaticPullNodes.find(node) == m_automaticPullNodes.end())
    {
        m_automaticPullNodes.insert(node);
        m_automaticPullNodesNeedUpdating = true;
    }
}

void AudioContext::removeAutomaticPullNode(std::shared_ptr<AudioNode> node)
{
    std::lock_guard<std::mutex> lock(m_updateMutex);
    auto it = m_automaticPullNodes.find(node);
    if (it != m_automaticPullNodes.end())
    {
        m_automaticPullNodes.erase(it);
        m_automaticPullNodesNeedUpdating = true;
    }
}

void AudioContext::updateAutomaticPullNodes()
{
    /// @TODO this seems like work for the update thread.
    /// m_automaticPullNodesNeedUpdating can go away in favor of
    /// add and remove doing a cv.notify.
    /// m_automaticPullNodes should be an add/remove vector
    /// m_renderingAutomaticPullNodes should be the actual live vector
    if (m_automaticPullNodesNeedUpdating)
    {
        std::lock_guard<std::mutex> lock(m_updateMutex);

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

void AudioContext::enqueueEvent(std::function<void()>& fn)
{
    m_internal->enqueuedEvents.enqueue(fn);
    cv.notify_all();    // processing thread must dispatch events
}

void AudioContext::dispatchEvents()
{
    std::function<void()> event_fn;
    while (m_internal->enqueuedEvents.try_dequeue(event_fn))
    {
        if (event_fn)
            event_fn();
    }
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

uint64_t AudioContext::currentSampleFrame() const
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

void AudioContext::startRendering()
{
    destination()->startRendering();
}

} // End namespace lab
