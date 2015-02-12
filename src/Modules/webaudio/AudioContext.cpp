/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSoundConfig.h"

#include "AudioContext.h"

#include "AnalyserNode.h"
#include "AsyncAudioDecoder.h"
#include "AudioBuffer.h"
#include "AudioBufferCallback.h"
#include "AudioBufferSourceNode.h"
#include "AudioContextLock.h"
#include "AudioListener.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "BiquadFilterNode.h"
#include "ChannelMergerNode.h"
#include "ChannelSplitterNode.h"
#include "ConvolverNode.h"
#include "DefaultAudioDestinationNode.h"
#include "DelayNode.h"
#include "DynamicsCompressorNode.h"
#include "FFTFrame.h"
#include "GainNode.h"
#include "HRTFDatabaseLoader.h"
#include "HRTFPanner.h"
#include "OfflineAudioDestinationNode.h"
#include "OscillatorNode.h"
#include "PannerNode.h"
#include "WaveShaperNode.h"
#include "WaveTable.h"

#include "MediaStream.h"
#include "MediaStreamAudioDestinationNode.h"
#include "MediaStreamAudioSourceNode.h"

#if DEBUG_AUDIONODE_REFERENCES
#include <stdio.h>
#endif

#include <wtf/Atomics.h>
#include <wtf/MainThread.h>

using namespace std;

namespace WebCore {
    
namespace {
    
bool isSampleRateRangeGood(float sampleRate)
{
    // FIXME: It would be nice if the minimum sample-rate could be less than 44.1KHz,
    // but that will require some fixes in HRTFPanner::fftSizeForSampleRate(), and some testing there.
    return sampleRate >= 44100 && sampleRate <= 96000;
}

}

// Don't allow more than this number of simultaneous AudioContexts talking to hardware.
const unsigned MaxHardwareContexts = 4;
unsigned AudioContext::s_hardwareContextCount = 0;
    
std::unique_ptr<AudioContext> AudioContext::create(ExceptionCode&)
{
    ASSERT(isMainThread());
    if (s_hardwareContextCount >= MaxHardwareContexts)
        return 0;

    return std::unique_ptr<AudioContext>(new AudioContext());
}

std::unique_ptr<AudioContext> AudioContext::createOfflineContext(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate, ExceptionCode& ec)
{
    // FIXME: offline contexts have limitations on supported sample-rates.
    // Currently all AudioContexts must have the same sample-rate.
    auto loader = HRTFDatabaseLoader::loader();
    if (numberOfChannels > 10 || !isSampleRateRangeGood(sampleRate) || (loader && loader->databaseSampleRate() != sampleRate)) {
        ec = SYNTAX_ERR;
        return 0;
    }

    return std::unique_ptr<AudioContext>(new AudioContext(numberOfChannels, numberOfFrames, sampleRate));
}

// Constructor for rendering to the audio hardware.
AudioContext::AudioContext()
    : m_isStopScheduled(false)
    , m_isInitialized(false)
    , m_isAudioThreadFinished(false)
    , m_destinationNode(0)
    , m_isDeletionScheduled(false)
    , m_automaticPullNodesNeedUpdating(false)
    , m_connectionCount(0)
    , m_isOfflineContext(false)
    , m_activeSourceCount(0)
{
    constructCommon();
}

// Constructor for offline (non-realtime) rendering.
AudioContext::AudioContext(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate)
    : m_isStopScheduled(false)
    , m_isInitialized(false)
    , m_isAudioThreadFinished(false)
    , m_destinationNode(0)
    , m_automaticPullNodesNeedUpdating(false)
    , m_connectionCount(0)
    , m_isOfflineContext(true)
    , m_activeSourceCount(0)
{
    constructCommon();

    // FIXME: the passed in sampleRate MUST match the hardware sample-rate since HRTFDatabaseLoader is a singleton.
    m_hrtfDatabaseLoader = HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(sampleRate);

    // Create a new destination for offline rendering.
    m_renderTarget = AudioBuffer::create(numberOfChannels, numberOfFrames, sampleRate);
    // a destination node must be created before this context can be used
}

void AudioContext::initHRTFDatabase() {
    
    // This sets in motion an asynchronous loading mechanism on another thread.
    // We can check m_hrtfDatabaseLoader->isLoaded() to find out whether or not it has been fully loaded.
    // It's not that useful to have a callback function for this since the audio thread automatically starts rendering on the graph
    // when this has finished (see AudioDestinationNode).
    m_hrtfDatabaseLoader = HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(sampleRate());
}

void AudioContext::constructCommon()
{
    FFTFrame::initialize();
    
    m_listener = std::make_shared<AudioListener>();
}

AudioContext::~AudioContext()
{
#if DEBUG_AUDIONODE_REFERENCES
    fprintf(stderr, "%p: AudioContext::~AudioContext()\n", this);
#endif
    
    ASSERT(!m_isInitialized);
    ASSERT(m_isStopScheduled);
    ASSERT(!m_nodesToDelete.size());
    ASSERT(!m_referencedNodes.size());
    ASSERT(!m_finishedNodes.size());
    ASSERT(!m_automaticPullNodes.size());
    ASSERT(!m_renderingAutomaticPullNodes.size());
}

void AudioContext::lazyInitialize()
{
    if (!m_isInitialized) {
        // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
        ASSERT(!m_isAudioThreadFinished);
        if (!m_isAudioThreadFinished) {
            if (m_destinationNode.get()) {
                m_destinationNode->initialize();

                if (!isOfflineContext()) {
                    // This starts the audio thread. The destination node's provideInput() method will now be called repeatedly to render audio.
                    // Each time provideInput() is called, a portion of the audio stream is rendered. Let's call this time period a "render quantum".
                    // NOTE: for now default AudioContext does not need an explicit startRendering() call from JavaScript.
                    // We may want to consider requiring it for symmetry with OfflineAudioContext.
                    m_destinationNode->startRendering();                    
                    ++s_hardwareContextCount;
                }

            }
            m_isInitialized = true;
        }
    }
}

void AudioContext::clear()
{
    // Audio thread is dead. Nobody will schedule node deletion action. Let's do it ourselves.
    do {
        deleteMarkedNodes();
        m_nodesToDelete.insert(m_nodesToDelete.end(), m_nodesMarkedForDeletion.begin(), m_nodesMarkedForDeletion.end());
        m_nodesMarkedForDeletion.clear();
    } while (m_nodesToDelete.size());
}

void AudioContext::uninitialize(ContextGraphLock& g, ContextRenderLock& r)
{
    // &&& This routine should be called during destruction
    
    ASSERT(isMainThread());

    if (!m_isInitialized)
        return;

    // This stops the audio thread and all audio rendering.
    m_destinationNode->uninitialize();

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    m_isAudioThreadFinished = true;

    if (!isOfflineContext()) {
        ASSERT(s_hardwareContextCount);
        --s_hardwareContextCount;
    }

    // Get rid of the sources which may still be playing.
    derefUnfinishedSourceNodes(g, r);

    m_isInitialized = false;
}

bool AudioContext::isInitialized() const
{
    return m_isInitialized;
}

    size_t AudioContext::currentSampleFrame() const { return m_destinationNode->currentSampleFrame(); }
    double AudioContext::currentTime() const { return m_destinationNode->currentTime(); }
    float AudioContext::sampleRate() const { return m_destinationNode ? m_destinationNode->sampleRate() : AudioDestination::hardwareSampleRate(); }
    
    void AudioContext::incrementConnectionCount()
    {
        atomicIncrement(&m_connectionCount);    // running tally
    }

    
bool AudioContext::isRunnable() const
{
    if (!isInitialized())
        return false;
    
    // Check with the HRTF spatialization system to see if it's finished loading.
    return m_hrtfDatabaseLoader->isLoaded();
}

void AudioContext::stop(ContextGraphLock& g, ContextRenderLock& r)
{
    if (m_isStopScheduled)
        return;
    
    ASSERT(isMainThread());
    
    m_isStopScheduled = true;
    
    uninitialize(g, r);
    clear();
}

void AudioContext::decodeAudioData(std::shared_ptr<std::vector<uint8_t>> audioData,
                                   PassRefPtr<AudioBufferCallback> successCallback, PassRefPtr<AudioBufferCallback> errorCallback, ExceptionCode& ec)
{
    if (!audioData) {
        ec = SYNTAX_ERR;
        return;
    }
    m_audioDecoder->decodeAsync(audioData, sampleRate(), successCallback, errorCallback);
}

std::shared_ptr<MediaStreamAudioSourceNode> AudioContext::createMediaStreamSource(ContextGraphLock& g, ContextRenderLock& r, ExceptionCode& ec)
{
    std::shared_ptr<MediaStream> mediaStream = std::make_shared<MediaStream>();

    AudioSourceProvider* provider = 0;

    if (mediaStream->isLocal() && mediaStream->audioTracks()->length())
        provider = destination()->localAudioInputProvider();
    else {
        // FIXME: get a provider for non-local MediaStreams (like from a remote peer).
        provider = 0;
    }

    std::shared_ptr<MediaStreamAudioSourceNode> node(new MediaStreamAudioSourceNode(mediaStream, provider, sampleRate()));

    // FIXME: Only stereo streams are supported right now. We should be able to accept multi-channel streams.
    node->setFormat(g, r, 2, sampleRate());

    refNode(g, node); // context keeps reference until node is disconnected
    return node;
}

void AudioContext::notifyNodeFinishedProcessing(ContextRenderLock& r, AudioNode* node)
{
    ASSERT(r.context());

    for (auto i : m_referencedNodes) {
        if (i.get() == node) {
            m_finishedNodes.push_back(i);
            return;
        }
    }
    ASSERT(0 == "node to finish not referenced");
}

void AudioContext::derefFinishedSourceNodes(ContextGraphLock& g, ContextRenderLock& r)
{
    ASSERT(g.context() && r.context());
    for (unsigned i = 0; i < m_finishedNodes.size(); i++)
        derefNode(g, r, m_finishedNodes[i]);

    m_finishedNodes.clear();
}

void AudioContext::refNode(ContextGraphLock& g, std::shared_ptr<AudioNode> node)
{
    ASSERT(g.context());
    node->ref(g.contextPtr(), AudioNode::RefTypeConnection);
    m_referencedNodes.push_back(node);
}

void AudioContext::derefNode(ContextGraphLock& g, ContextRenderLock& r, std::shared_ptr<AudioNode> node)
{
    ASSERT(g.context());
    
    node->deref(g, r, AudioNode::RefTypeConnection);

    for (std::vector<std::shared_ptr<AudioNode>>::iterator i = m_referencedNodes.begin(); i != m_referencedNodes.end(); ++i) {
        if (node == *i) {
            m_referencedNodes.erase(i);
            break;
        }
    }
}

void AudioContext::derefUnfinishedSourceNodes(ContextGraphLock& g, ContextRenderLock& r)
{
    ASSERT(g.context());
    for (unsigned i = 0; i < m_referencedNodes.size(); ++i)
        m_referencedNodes[i]->deref(g, r, AudioNode::RefTypeConnection);

    m_referencedNodes.clear();
}

    
void AudioContext::holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode> sn) {
    lock_guard<mutex> lock(automaticSourcesMutex);
    automaticSources.push_back(sn);
}
    
void AudioContext::handleAutomaticSources() {
    lock_guard<mutex> lock(automaticSourcesMutex);
    for (auto i = automaticSources.begin(); i != automaticSources.end(); ++i) {
        if ((*i)->hasFinished()) {
            i = automaticSources.erase(i);
            if (i == automaticSources.end())
                break;
        }
    }
}

void AudioContext::addDeferredFinishDeref(ContextGraphLock& g, AudioNode* node)
{
    ASSERT(g.context());
    m_deferredFinishDerefList.push_back(node);
}

void AudioContext::handlePreRenderTasks(ContextGraphLock& g, ContextRenderLock& r)
{
    ASSERT(r.context());
 
    // At the beginning of every render quantum, try to update the internal rendering graph state (from main thread changes).
    handleDirtyAudioSummingJunctions(g, r);
    updateAutomaticPullNodes(g, r);
}

void AudioContext::handlePostRenderTasks(ContextGraphLock& g, ContextRenderLock& r)
{
    ASSERT(r.context());
 
    // Take care of finishing any derefs where the tryLock() failed previously.
    handleDeferredFinishDerefs(g, r);

    // Dynamically clean up nodes which are no longer needed.
    derefFinishedSourceNodes(g, r);

    // Don't delete in the real-time thread. Let the main thread do it because the clean up may take time
    scheduleNodeDeletion(g);

    // Fixup the state of any dirty AudioSummingJunctions and AudioNodeOutputs.
    handleDirtyAudioSummingJunctions(g, r);

    updateAutomaticPullNodes(g, r);
    
    handleAutomaticSources();
}

void AudioContext::handleDeferredFinishDerefs(ContextGraphLock& g, ContextRenderLock& r)
{
    ASSERT(g.context());
    for (unsigned i = 0; i < m_deferredFinishDerefList.size(); ++i) {
        AudioNode* node = m_deferredFinishDerefList[i];
        node->finishDeref(g, r, AudioNode::RefTypeConnection);
    }
    
    m_deferredFinishDerefList.clear();
}

void AudioContext::markForDeletion(ContextGraphLock& g, ContextRenderLock& r, AudioNode* node)
{
    ASSERT(g.context());
    for (auto i : m_referencedNodes) {
        if (i.get() == node) {
            m_nodesMarkedForDeletion.push_back(i);
            
            // This is probably the best time for us to remove the node from automatic pull list,
            // since all connections are gone and we hold the graph lock. Then when handlePostRenderTasks()
            // gets a chance to schedule the deletion work, updateAutomaticPullNodes() also gets a chance to
            // modify m_renderingAutomaticPullNodes.
            removeAutomaticPullNode(g, r, node);
            return;
        }
    }
    
    ASSERT(0 == "Attempting to delete unreferenced node");
}

void AudioContext::scheduleNodeDeletion(ContextGraphLock& g)
{
    // &&& all this deletion stuff should be handled by a concurrent queue
    
    bool isGood = m_isInitialized && g.context();
    ASSERT(isGood);
    if (!isGood)
        return;

    // Make sure to call deleteMarkedNodes() on main thread.    
    if (m_nodesMarkedForDeletion.size() && !m_isDeletionScheduled) {
        m_nodesToDelete.insert(m_nodesToDelete.end(), m_nodesMarkedForDeletion.begin(), m_nodesMarkedForDeletion.end());
        m_nodesMarkedForDeletion.clear();

        m_isDeletionScheduled = true;

        // Don't let ourself get deleted before the callback.
        // See matching deref() in deleteMarkedNodesDispatch().
        callOnMainThread(deleteMarkedNodesDispatch, this);
    }
}

void AudioContext::deleteMarkedNodesDispatch(void* userData)
{
    AudioContext* context = reinterpret_cast<AudioContext*>(userData);
    ASSERT(context);
    if (!context)
        return;

    context->deleteMarkedNodes();
}

void AudioContext::deleteMarkedNodes()
{
    ASSERT(isMainThread());
    m_nodesToDelete.clear();
    m_isDeletionScheduled = false;
}

void AudioContext::markSummingJunctionDirty(std::shared_ptr<AudioSummingJunction> summingJunction)
{
    if (summingJunction)
        m_dirtySummingJunctions.push(summingJunction);
}

void AudioContext::handleDirtyAudioSummingJunctions(ContextGraphLock& g, ContextRenderLock& r)
{
    ASSERT(r.context());

    std::shared_ptr<AudioSummingJunction> asj;
    while (m_dirtySummingJunctions.try_pop(asj))
        asj->updateRenderingState(g, r);
}


void AudioContext::addAutomaticPullNode(ContextGraphLock& g, ContextRenderLock& r, AudioNode* node)
{
    ASSERT(g.context() && r.context());

    if (m_automaticPullNodes.find(node) == m_automaticPullNodes.end()) {
        m_automaticPullNodes.insert(node);
        m_automaticPullNodesNeedUpdating = true;
    }
}

void AudioContext::removeAutomaticPullNode(ContextGraphLock& g, ContextRenderLock& r, AudioNode* node)
{
    ASSERT(g.context() && r.context());

    auto it = m_automaticPullNodes.find(node);
    if (it != m_automaticPullNodes.end()) {
        m_automaticPullNodes.erase(it);
        m_automaticPullNodesNeedUpdating = true;
    }
}

void AudioContext::updateAutomaticPullNodes(ContextGraphLock& g, ContextRenderLock& r)
{
    ASSERT(g.context() && r.context());

    if (m_automaticPullNodesNeedUpdating) {
        // Copy from m_automaticPullNodes to m_renderingAutomaticPullNodes.
        m_renderingAutomaticPullNodes.resize(m_automaticPullNodes.size());

        unsigned j = 0;
        for (auto i = m_automaticPullNodes.begin(); i != m_automaticPullNodes.end(); ++i, ++j) {
            AudioNode* output = *i;
            m_renderingAutomaticPullNodes[j] = output;
        }

        m_automaticPullNodesNeedUpdating = false;
    }
}

void AudioContext::processAutomaticPullNodes(ContextGraphLock& g, ContextRenderLock& r, size_t framesToProcess)
{
    ASSERT(r.context());

    for (unsigned i = 0; i < m_renderingAutomaticPullNodes.size(); ++i)
        m_renderingAutomaticPullNodes[i]->processIfNecessary(g, r, framesToProcess);
}

void AudioContext::startRendering()
{
    destination()->startRendering();
}

void AudioContext::fireCompletionEvent()
{
    ASSERT(isMainThread());
    if (!isMainThread())
        return;
        
    AudioBuffer* renderedBuffer = m_renderTarget.get();

    ASSERT(renderedBuffer);
    if (!renderedBuffer)
        return;
    /* LabSound
    // Avoid firing the event if the document has already gone away.
    if (scriptExecutionContext()) {
        // Call the offline rendering completion event listener.
        dispatchEvent(OfflineAudioCompletionEvent::create(renderedBuffer));
    }
     */
}

void AudioContext::incrementActiveSourceCount()
{
    atomicIncrement(&m_activeSourceCount);
}

void AudioContext::decrementActiveSourceCount()
{
    atomicDecrement(&m_activeSourceCount);
}

} // namespace WebCore
