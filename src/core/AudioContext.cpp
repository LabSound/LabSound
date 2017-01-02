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

namespace lab
{

const uint32_t lab::AudioContext::maxNumberOfChannels = 32;
    
std::shared_ptr<AudioHardwareSourceNode> MakeHardwareSourceNode(lab::ContextRenderLock & r)
{
    AudioSourceProvider * provider = nullptr;
    
    provider = r.contextPtr()->destination()->localAudioInputProvider();
    
    auto sampleRate = r.contextPtr()->sampleRate();
    
    std::shared_ptr<AudioHardwareSourceNode> inputNode(new AudioHardwareSourceNode(provider, sampleRate));
    
    // FIXME: Only stereo streams are supported right now. We should be able to accept multi-channel streams.
    inputNode->setFormat(r, 2, sampleRate);
    
    //m_referencedNodes.push_back(inputNode); // context keeps reference until node is disconnected
    
    return inputNode;
}
    
// Constructor for realtime rendering
AudioContext::AudioContext()
{
    m_isOfflineContext = false;
    m_listener = std::make_shared<AudioListener>();
}

// Constructor for offline (non-realtime) rendering.
AudioContext::AudioContext(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate)
{
    m_isOfflineContext = true;
    m_listener = std::make_shared<AudioListener>();

    // Create a new destination for offline rendering.
    m_renderTarget = std::make_shared<AudioBuffer>(numberOfChannels, numberOfFrames, sampleRate);
}

AudioContext::~AudioContext()
{
#if DEBUG_AUDIONODE_REFERENCES
    fprintf(stderr, "%p: AudioContext::~AudioContext()\n", this);
#endif
    
#if USE_ACCELERATE_FFT
    FFTFrame::cleanup();
#endif
    
    ASSERT(!m_isInitialized);
    ASSERT(m_isStopScheduled);
    ASSERT(!m_nodesToDelete.size());
    ASSERT(!m_referencedNodes.size());
    ASSERT(!m_automaticPullNodes.size());
    ASSERT(!m_renderingAutomaticPullNodes.size());
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

            }
            m_isInitialized = true;
        }
    }
}

void AudioContext::clear()
{
    // Audio thread is dead. Nobody will schedule node deletion action. Let's do it ourselves.
    if (m_destinationNode.get())
        m_destinationNode.reset();
    
    do
    {
        deleteMarkedNodes();
        m_nodesToDelete.insert(m_nodesToDelete.end(), m_nodesMarkedForDeletion.begin(), m_nodesMarkedForDeletion.end());
        m_nodesMarkedForDeletion.clear();
    }
    while (m_nodesToDelete.size());
}

void AudioContext::uninitialize(ContextGraphLock& g)
{
    if (!m_isInitialized)
        return;

    // This stops the audio thread and all audio rendering.
    m_destinationNode->uninitialize();

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    m_isAudioThreadFinished = true;
    
    updateAutomaticPullNodes(); // added for the case where an OfflineAudioDestinationNode needs to update the graph

    m_referencedNodes.clear();
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

void AudioContext::stop(ContextGraphLock& g)
{
    if (m_isStopScheduled)
        return;

    m_isStopScheduled = true;

    deleteMarkedNodes();

    uninitialize(g);
    clear();
}

void AudioContext::referenceSourceNode(ContextGraphLock & g, std::shared_ptr<AudioNode> node)
{
    m_referencedNodes.push_back(node);
}

void AudioContext::dereferenceSourceNode(ContextGraphLock & g, std::shared_ptr<AudioNode> node)
{
    ASSERT(g.context());

    for (std::vector<std::shared_ptr<AudioNode>>::iterator i = m_referencedNodes.begin(); i != m_referencedNodes.end(); ++i)
    {
        if (node == *i)
        {
            m_referencedNodes.erase(i);
            break;
        }
    }
}

void AudioContext::holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode> sn)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    automaticSources.push_back(sn);
}

void AudioContext::handleAutomaticSources()
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    for (auto i = automaticSources.begin(); i != automaticSources.end(); ++i)
    {
        if ((*i)->hasFinished())
        {
            pendingNodeConnections.emplace(*i, std::shared_ptr<AudioNode>(), false);
            i = automaticSources.erase(i);
            if (i == automaticSources.end())
                break;
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

    // Don't delete in the real-time thread. Let the main thread do it because the clean up may take time
    scheduleNodeDeletion(r);

    AudioSummingJunction::handleDirtyAudioSummingJunctions(r);
    updateAutomaticPullNodes();

    handleAutomaticSources();
}

// ---
    
void AudioContext::connect(std::shared_ptr<AudioNode> from, std::shared_ptr<AudioNode> to)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    pendingNodeConnections.emplace(from, to, true);
}

void AudioContext::disconnect(std::shared_ptr<AudioNode> from, std::shared_ptr<AudioNode> to)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    pendingNodeConnections.emplace(from, to, false);
}

// --- 
    
void AudioContext::connect(std::shared_ptr<AudioNodeInput> fromInput, std::shared_ptr<AudioNodeOutput> toOutput)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    pendingConnections.emplace_back(PendingConnection<AudioNodeInput, AudioNodeOutput>(fromInput, toOutput, true));
}

void AudioContext::disconnect(std::shared_ptr<AudioNodeOutput> toOutput)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    auto nullSource = std::shared_ptr<AudioNodeInput>(nullptr);
    pendingConnections.emplace_back(PendingConnection<AudioNodeInput, AudioNodeOutput>(nullSource, toOutput, false));
}
    
// ---
    
void AudioContext::disconnect(std::shared_ptr<AudioNode> from)
{
    std::lock_guard<std::mutex> lock(automaticSourcesMutex);
    pendingNodeConnections.emplace(from, std::shared_ptr<AudioNode>(), false);
}

void AudioContext::update(ContextGraphLock& g)
{
    {
        std::lock_guard<std::mutex> lock(automaticSourcesMutex);
        
        for (auto i : pendingConnections)
        {
            if (i.connect)
            {
                //LOG("Connect from: %p", i.from.get());
                //LOG("Connect to: %p", i.to.get());
                AudioNodeInput::connect(g, i.from, i.to);
            }
            else
            {
                //LOG("Disconnect from: %p", i.from.get());
                //LOG("Disconnect to: %p", i.to.get());
                AudioNodeOutput::disconnectAll(g, i.to); // Params + Inputs
            }
        }
        pendingConnections.clear();

        double now = currentTime();
        
        //for (auto i : pendingNodeConnections)
        while (!pendingNodeConnections.empty())
        {
            auto& testMe = pendingNodeConnections.top();
            if (testMe.from->isScheduledNode()) {
                AudioScheduledSourceNode* node = dynamic_cast<AudioScheduledSourceNode*>(testMe.from.get());
                
                if (node->startTime() > now + sampleRate() * 1.f/1000000.f * 1.f/10.f)
                    break; // stop processing the queue if the scheduled time is > 100ms away
            }
            
            auto i = pendingNodeConnections.top();
            pendingNodeConnections.pop();
            
            if (i.connect)
            {
                AudioNodeInput::connect(g, i.to->input(0), i.from->output(0));
                referenceSourceNode(g, i.from);
                referenceSourceNode(g, i.to);
                ++i.from->m_connectionRefCount;
                ++i.to->m_connectionRefCount;
            }
            else
            {
                if (i.to && i.from)
                {
                    --i.from->m_connectionRefCount;
                    --i.to->m_connectionRefCount;
                    AudioNodeInput::disconnect(g, i.from->input(0), i.to->output(0));
                    dereferenceSourceNode(g, i.from);
                    dereferenceSourceNode(g, i.to);
                }
                else if (i.from)
                {
                    --i.from->m_connectionRefCount;
                    for (size_t out = 0; out < i.from->numberOfOutputs(); ++out)
                    {
                        auto output = i.from->output(out);
                        if (!output)
                            continue;
                        
                        AudioNodeOutput::disconnectAllInputs(g, output);
                        AudioNodeOutput::disconnectAllParams(g, output);
                    }
                }
                else if (i.to)
                {
                    --i.to->m_connectionRefCount;
                    for (size_t out = 0; out < i.to->numberOfOutputs(); ++out)
                    {
                        auto output = i.to->output(out);
                        if (!output)
                            continue;
                        
                        AudioNodeOutput::disconnectAllInputs(g, output);
                        AudioNodeOutput::disconnectAllParams(g, output);
                    }
                }
            }
        }
        
        //auto d = destination();
        //auto in = d->input(0);
        //printf("%d\n", (int) in->numberOfRenderingConnections());
        //pendingNodeConnections.clear();
    }
}

void AudioContext::markForDeletion(ContextRenderLock& r, AudioNode* node)
{
    ASSERT(r.context());
    for (auto i : m_referencedNodes)
    {
        if (i.get() == node)
        {
            m_nodesMarkedForDeletion.push_back(i);
            return;
        }
    }
}

void AudioContext::scheduleNodeDeletion(ContextRenderLock& r)
{
    //@fixme
    // &&& all this deletion stuff should be handled by a concurrent queue - simply have only a m_nodesToDelete concurrent queue and ditch the marked vector
    // then this routine sould go away completely
    // node->deref is the only caller, it should simply add itself to the scheduled deletion queue
    // marked for deletion should go away too

    bool isGood = m_isInitialized && r.context();
    ASSERT(isGood);
    if (!isGood)
        return;

    if (m_nodesMarkedForDeletion.size() && !m_isDeletionScheduled)
    {
        m_nodesToDelete.insert(m_nodesToDelete.end(), m_nodesMarkedForDeletion.begin(), m_nodesMarkedForDeletion.end());
        m_nodesMarkedForDeletion.clear();

        m_isDeletionScheduled = true;

        deleteMarkedNodes();
    }
}

void AudioContext::deleteMarkedNodes()
{
    //@fixme thread safety
    m_nodesToDelete.clear();
    m_isDeletionScheduled = false;
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

void AudioContext::processAutomaticPullNodes(ContextRenderLock& r, size_t framesToProcess)
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
    return m_destinationNode ? m_destinationNode->sampleRate() : AudioDestination::hardwareSampleRate(); 
}

std::shared_ptr<AudioListener> AudioContext::listener() 
{ 
    return m_listener; 
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
