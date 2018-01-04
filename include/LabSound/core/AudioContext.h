// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef AUDIO_CONTEXT_H
#define AUDIO_CONTEXT_H

#include "LabSound/core/ConcurrentQueue.h"
#include "LabSound/core/AudioScheduledSourceNode.h"

#include <set>
#include <atomic>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <string>

namespace lab
{

class AudioBuffer;
class AudioDestinationNode;
class AudioListener;
class AudioNode;
class AudioScheduledSourceNode;
class AudioHardwareSourceNode;
class AudioNodeInput;
class AudioNodeOutput;
class ContextGraphLock;
class ContextRenderLock;

// @tofix - refactor such that this factory function doesn't need to exist
std::shared_ptr<AudioHardwareSourceNode> MakeHardwareSourceNode(ContextRenderLock & r);

class AudioContext
{
    
    friend class ContextGraphLock;
    friend class ContextRenderLock;
    
public:

    // Somewhat arbitrary and could be increased if necessary
    static const uint32_t maxNumberOfChannels;

    // Debugging/Sanity Checking
    std::string m_graphLocker;
    std::string m_renderLocker;

    // Realtime Context
    AudioContext();

    // Offline (non-realtime) Context
    AudioContext(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate);

    ~AudioContext();

    bool isInitialized() const;

    // Eexternal users shouldn't use this; it should be called by LabSound::MakeAudioContext()
    // It *is* harmless to call it though, it's just not necessary.
    void lazyInitialize();

    void update(ContextGraphLock & g);

    void stop(ContextGraphLock & g);

    void setDestinationNode(std::shared_ptr<AudioDestinationNode> node);

    std::shared_ptr<AudioDestinationNode> destination();

    bool isOfflineContext();

    size_t currentSampleFrame() const;

    double currentTime() const;

    float sampleRate() const;

    std::shared_ptr<AudioListener> listener();

    unsigned long activeSourceCount() const;

    void incrementActiveSourceCount();
    void decrementActiveSourceCount();

    void handlePreRenderTasks(ContextRenderLock &); // Called at the start of each render quantum.
    void handlePostRenderTasks(ContextRenderLock &); // Called at the end of each render quantum.

    // We schedule deletion of all marked nodes at the end of each realtime render quantum.
    void deleteMarkedNodes();

    // AudioContext can pull node(s) at the end of each render quantum even when they are not connected to any downstream nodes.
    // These two methods are called by the nodes who want to add/remove themselves into/from the automatic pull lists.
    void addAutomaticPullNode(std::shared_ptr<AudioNode>);
    void removeAutomaticPullNode(std::shared_ptr<AudioNode>);

    // Called right before handlePostRenderTasks() to handle nodes which need to be pulled even when they are not connected to anything.
    // Only an AudioDestinationNode should call this. 
    void processAutomaticPullNodes(ContextRenderLock &, size_t framesToProcess);
    
    // Keeps track of the number of connections made.
    void incrementConnectionCount();
    
    unsigned connectionCount() const { return m_connectionCount;}
    
    void connect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx = 0, uint32_t srcIdx = 0);
    void disconnect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx = 0, uint32_t srcidx = 0);

    void holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode>);
    
    // Necessary to call when using an OfflineAudioDestinationNode
    void startRendering();
    
    std::shared_ptr<AudioBuffer> getOfflineRenderTarget() { return m_renderTarget; }
    std::function<void()> offlineRenderCompleteCallback;

private:

    std::mutex m_graphLock;
    std::mutex m_renderLock;
    std::mutex automaticSourcesMutex;

    bool m_isStopScheduled = false;
    bool m_isInitialized = false;
    bool m_isAudioThreadFinished = false;
    bool m_isOfflineContext = false;
    bool m_isDeletionScheduled = false;
    bool m_automaticPullNodesNeedUpdating = false; // keeps track if m_automaticPullNodes is modified.

    // Number of AudioBufferSourceNodes that are active (playing).
    std::atomic<int> m_activeSourceCount;
    std::atomic<int> m_connectionCount;

    void uninitialize(ContextGraphLock &);

    // Audio thread is dead. Nobody will schedule node deletion action. Let's do it ourselves.
    void clear();

    void scheduleNodeDeletion(ContextRenderLock & g);

    void handleAutomaticSources();
    void updateAutomaticPullNodes();

    std::shared_ptr<AudioDestinationNode> m_destinationNode;
    std::shared_ptr<AudioListener> m_listener;
    std::shared_ptr<AudioBuffer> m_renderTarget;

    std::vector<std::shared_ptr<AudioNode>> m_nodesToDelete;
    std::vector<std::shared_ptr<AudioNode>> m_nodesMarkedForDeletion;

    std::set<std::shared_ptr<AudioNode>> m_automaticPullNodes; // queue for added pull nodes
    std::vector<std::shared_ptr<AudioNode>> m_renderingAutomaticPullNodes; // vector of known pull nodes

    std::vector<std::shared_ptr<AudioScheduledSourceNode>> automaticSources;
    
    enum class ConnectionType : int
    {
        Disconnect = 0,
        Connect = 1
    };

    struct PendingConnection
    {
        ConnectionType type;
        std::shared_ptr<AudioNode> destination;
        std::shared_ptr<AudioNode> source;
        uint32_t destIndex;
        uint32_t srcIndex;
        PendingConnection(
            std::shared_ptr<AudioNode> destination,
            std::shared_ptr<AudioNode> source,
            ConnectionType t,
            uint32_t destIndex = 0,
            uint32_t srcIndex = 0)
            : destination(destination), source(source), type(t), destIndex(destIndex), srcIndex(srcIndex) { }
    };

    struct CompareScheduledTime
    {
        bool operator()(const PendingConnection & p1, const PendingConnection & p2)
        {
            return false;
            /*
            if (!p2.destination->isScheduledNode()) return true;
            if (!p1.destination->isScheduledNode()) return false;
            AudioScheduledSourceNode * ap1 = static_cast<AudioScheduledSourceNode*>(p1.destination.get());
            AudioScheduledSourceNode * ap2 = static_cast<AudioScheduledSourceNode*>(p2.destination.get());
            return ap2->startTime() < ap1->startTime();
            */
        }
    };
    
    std::priority_queue<PendingConnection, std::deque<PendingConnection>, CompareScheduledTime> pendingNodeConnections;
};

} // End namespace lab

#endif // AudioContext_h
