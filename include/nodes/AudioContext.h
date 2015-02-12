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

#ifndef AudioContext_h
#define AudioContext_h

#include "ConcurrentQueue.h"
#include "ExceptionCodes.h"
#include "WTF/RefPtr.h"
#include <mutex>
#include <set>
#include <vector>

namespace LabSound {
    class ContextGraphLock;
    class ContextRenderLock;
}

namespace WebCore {
    
    using namespace LabSound;

// AudioContext is the cornerstone of the web audio API and all AudioNodes are created from it.
// For thread safety between the audio thread and the main thread, it has a rendering graph locking mechanism. 

    class AudioBuffer;
    class AudioBufferCallback;
    class AudioDestinationNode;
    class AudioListener;
    class AudioNode;
    class AudioScheduledSourceNode;
    class AudioSummingJunction;
    class HRTFDatabaseLoader;
    class MediaStreamAudioSourceNode;
    class AsyncAudioDecoder;
    
class AudioContext {
public:
    
    const char* m_graphLocker;
    const char* m_renderLocker;
    
    // Create an AudioContext for rendering to the audio hardware.
    // A default audio destination node MUST be create and assigned as a next step.
    // Things won't work if the context is not created in this way.
    // ie:
    //     context->setDestinationNode(DefaultAudioDestinationNode::create(context));
    // then call initHRTFDatabase()
    static std::unique_ptr<AudioContext> create(ExceptionCode&);

    // Create an AudioContext for offline (non-realtime) rendering.
    // a destination node MUST be created and assigned as a next step.
    // Things won't work if the context is not created in this way.
    // ie:
    //     context->setDestinationNode(OfflineAudioDestinationNode::create(this, m_renderTarget.get()));
    // then call initHRTFDatabase()
    static std::unique_ptr<AudioContext> createOfflineContext(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate, ExceptionCode&);

    virtual ~AudioContext();

    void initHRTFDatabase();

    
    bool isInitialized() const;
    
    bool isOfflineContext() { return m_isOfflineContext; }

    // Returns true when initialize() was called AND all asynchronous initialization has completed.
    bool isRunnable() const;

    virtual void stop(ContextGraphLock&, ContextRenderLock&);

    void setDestinationNode(std::shared_ptr<AudioDestinationNode> node) { m_destinationNode = node; }
    std::shared_ptr<AudioDestinationNode> destination() { return m_destinationNode; }
    
    size_t currentSampleFrame() const;
    double currentTime() const;
    float sampleRate() const;
    unsigned long activeSourceCount() const { return static_cast<unsigned long>(m_activeSourceCount); }

    void incrementActiveSourceCount();
    void decrementActiveSourceCount();

    // Asynchronous audio file data decoding.
    void decodeAudioData(std::shared_ptr<std::vector<uint8_t>> data, PassRefPtr<AudioBufferCallback>, PassRefPtr<AudioBufferCallback>, ExceptionCode& ec);

    AudioListener* listener() { return m_listener.get(); }

    std::shared_ptr<MediaStreamAudioSourceNode> createMediaStreamSource(ContextGraphLock& g, ContextRenderLock& r, ExceptionCode&);

    // When a source node has no more processing to do (has finished playing), then it tells the context to dereference it.
    void notifyNodeFinishedProcessing(ContextRenderLock&, AudioNode*);

    // Called at the start of each render quantum.
    void handlePreRenderTasks(ContextGraphLock& g, ContextRenderLock&);

    // Called at the end of each render quantum.
    void handlePostRenderTasks(ContextGraphLock& g, ContextRenderLock&);

    // Called periodically at the end of each render quantum to dereference finished source nodes.
    void derefFinishedSourceNodes(ContextGraphLock& g, ContextRenderLock& r);

    // We schedule deletion of all marked nodes at the end of each realtime render quantum.
    void markForDeletion(ContextGraphLock& g, ContextRenderLock& r, AudioNode*);
    void deleteMarkedNodes();

    // AudioContext can pull node(s) at the end of each render quantum even when they are not connected to any downstream nodes.
    // These two methods are called by the nodes who want to add/remove themselves into/from the automatic pull lists.
    void addAutomaticPullNode(ContextGraphLock& g, ContextRenderLock& r, AudioNode*);
    void removeAutomaticPullNode(ContextGraphLock& g, ContextRenderLock& r, AudioNode*);

    // Called right before handlePostRenderTasks() to handle nodes which need to be pulled even when they are not connected to anything.
    void processAutomaticPullNodes(ContextGraphLock& g, ContextRenderLock&, size_t framesToProcess);

    // Keeps track of the number of connections made.
    void incrementConnectionCount();

    unsigned connectionCount() const { return m_connectionCount; }

    // Returns the maximum numuber of channels we can support.
    static unsigned maxNumberOfChannels() { return MaxNumberOfChannels;}
    
    // In AudioNode::deref() a tryLock() is used for calling finishDeref(), but if it fails keep track here.
    void addDeferredFinishDeref(ContextGraphLock& g, AudioNode*);

    // In the audio thread at the start of each render cycle, we'll call handleDeferredFinishDerefs().
    void handleDeferredFinishDerefs(ContextGraphLock&, ContextRenderLock&);

    void markSummingJunctionDirty(std::shared_ptr<AudioSummingJunction>);

    void startRendering();
    void fireCompletionEvent();
    
    static unsigned s_hardwareContextCount;

    /// @LabSound - external users shouldn't use this; it is called either by one of the
    /// context's object creation routines, such as createGainNode, or by LabSound::init.
    /// It *is* harmless to call it though, it's just not necessary.
    void lazyInitialize();

private:
    explicit AudioContext();
    AudioContext(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate);
    void constructCommon();

public:
    void uninitialize(ContextGraphLock&, ContextRenderLock&);

private:
    friend class LabSound::ContextGraphLock;
    friend class LabSound::ContextRenderLock;
    std::mutex m_graphLock;
    std::mutex m_renderLock;
    
    bool m_isStopScheduled;
    void clear();

    void scheduleNodeDeletion(ContextGraphLock& g);
    static void deleteMarkedNodesDispatch(void* userData);
    
    bool m_isInitialized;
    bool m_isAudioThreadFinished;

    // The context itself keeps a reference to all source nodes.  The source nodes, then reference all nodes they're connected to.
    // In turn, these nodes reference all nodes they're connected to.  All nodes are ultimately connected to the AudioDestinationNode.
    // When the context dereferences a source node, it will be deactivated from the rendering graph along with all other nodes it is
    // uniquely connected to.  See the AudioNode::ref() and AudioNode::deref() methods for more details.
    void refNode(ContextGraphLock&, std::shared_ptr<AudioNode>);
    void derefNode(ContextGraphLock&, ContextRenderLock&, std::shared_ptr<AudioNode>);

    // When the context goes away, there might still be some sources which haven't finished playing.
    // Make sure to dereference them here.
    void derefUnfinishedSourceNodes(ContextGraphLock&, ContextRenderLock&);
    
public:
    void holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode>);
private:
    void handleAutomaticSources();


    std::shared_ptr<AudioDestinationNode> m_destinationNode;
    std::shared_ptr<AudioListener> m_listener;

    // Only accessed in the audio thread.
    std::vector<std::shared_ptr<AudioNode>> m_finishedNodes;

    // We don't use RefPtr<AudioNode> here because AudioNode has a more complex ref() / deref() implementation
    // with an optional argument for refType.  We need to use the special refType: RefTypeConnection
    // Either accessed when the graph lock is held, or on the main thread when the audio thread has finished.
    std::vector<std::shared_ptr<AudioNode>> m_referencedNodes;

    // Accumulate nodes which need to be deleted here.
    // This is copied to m_nodesToDelete at the end of a render cycle in handlePostRenderTasks(), where we're assured of a stable graph
    // state which will have no references to any of the nodes in m_nodesToDelete once the context lock is released
    // (when handlePostRenderTasks() has completed).
    std::vector<std::shared_ptr<AudioNode>> m_nodesMarkedForDeletion;

    // They will be scheduled for deletion (on the main thread) at the end of a render cycle (in realtime thread).
    std::vector<std::shared_ptr<AudioNode>> m_nodesToDelete;
    bool m_isDeletionScheduled;

    // Only accessed when the graph lock is held.
    LabSound::concurrent_queue<std::shared_ptr<AudioSummingJunction>> m_dirtySummingJunctions;
    void handleDirtyAudioSummingJunctions(ContextGraphLock& g, ContextRenderLock& r);

    // For the sake of thread safety, we maintain a seperate vector of automatic pull nodes for rendering in m_renderingAutomaticPullNodes.
    // It will be copied from m_automaticPullNodes by updateAutomaticPullNodes() at the very start or end of the rendering quantum.
    std::set<AudioNode*> m_automaticPullNodes;
    std::vector<AudioNode*> m_renderingAutomaticPullNodes;
    // m_automaticPullNodesNeedUpdating keeps track if m_automaticPullNodes is modified.
    bool m_automaticPullNodesNeedUpdating;
    void updateAutomaticPullNodes(ContextGraphLock& g, ContextRenderLock& r);

    int m_connectionCount;

    // Graph locking.
    
    // Only accessed in the audio thread.
    std::vector<AudioNode*> m_deferredFinishDerefList;
    
    // HRTF Database loader
    std::shared_ptr<HRTFDatabaseLoader> m_hrtfDatabaseLoader;

    std::shared_ptr<AudioBuffer> m_renderTarget;
    
    bool m_isOfflineContext;

    std::unique_ptr<AsyncAudioDecoder> m_audioDecoder;

    // This is considering 32 is large enough for multiple channels audio. 
    // It is somewhat arbitrary and could be increased if necessary.
    enum { MaxNumberOfChannels = 32 };

    // Number of AudioBufferSourceNodes that are active (playing).
    int m_activeSourceCount;
    
    std::mutex automaticSourcesMutex;
    std::vector<std::shared_ptr<AudioScheduledSourceNode>> automaticSources;
};

} // WebCore

#endif // AudioContext_h
