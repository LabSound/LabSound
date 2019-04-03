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
#include <condition_variable>
#include <functional>

namespace lab
{

class AudioDestinationNode;
class AudioListener;
class AudioNode;
class AudioScheduledSourceNode;
class AudioHardwareSourceNode;
class AudioNodeInput;
class AudioNodeOutput;
class ContextGraphLock;
class ContextRenderLock;

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

    explicit AudioContext(bool isOffline, bool autoDispatchEvents = true);
    ~AudioContext();

    bool isInitialized() const;

    // External users shouldn't use this; it should be called by LabSound::MakeRealtimeAudioContext(lab::Channels::Stereo)
    // It *is* harmless to call it though, it's just not necessary.
    void lazyInitialize();

    void setDestinationNode(std::shared_ptr<AudioDestinationNode> node);

    std::shared_ptr<AudioDestinationNode> destination();

    bool isOfflineContext();

    uint64_t currentSampleFrame() const;

    double currentTime() const;

    float sampleRate() const;

    AudioListener & listener();

    void handlePreRenderTasks(ContextRenderLock &); // Called at the start of each render quantum.
    void handlePostRenderTasks(ContextRenderLock &); // Called at the end of each render quantum.

    // AudioContext can pull node(s) at the end of each render quantum even when they are not connected to any downstream nodes.
    // These two methods are called by the nodes who want to add/remove themselves into/from the automatic pull lists.
    void addAutomaticPullNode(std::shared_ptr<AudioNode>);
    void removeAutomaticPullNode(std::shared_ptr<AudioNode>);

    // Called right before handlePostRenderTasks() to handle nodes which need to be pulled even when they are not connected to anything.
    // Only an AudioDestinationNode should call this.
    void processAutomaticPullNodes(ContextRenderLock &, size_t framesToProcess);

    void connect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx = 0, uint32_t srcIdx = 0);
    void disconnect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx = 0, uint32_t srcidx = 0);

    void connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, uint32_t index);

    void holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode> node);

    // Necessary to call when using an OfflineAudioDestinationNode
    void startRendering();
    std::function<void()> offlineRenderCompleteCallback;

    // event dispatching will be called automatically, depending on constructor
    // argument. If not automatically dispatching, it is the user's responsibility
    // to call dispatchEvents often enough to satisfy the user's needs.
    void enqueueEvent(std::function<void()>&);
    void dispatchEvents();

private:

    std::mutex m_graphLock;
    std::mutex m_renderLock;
    std::mutex m_updateMutex;
    std::condition_variable cv;

    std::atomic<bool> updateThreadShouldRun{ true };
    std::thread graphUpdateThread;
    void update();
    float graphKeepAlive{ 0.f };
    float lastGraphUpdateTime{ 0.f };

    bool m_isInitialized = false;
    bool m_isAudioThreadFinished = false;
    bool m_isOfflineContext = false;
    bool m_automaticPullNodesNeedUpdating = false; // keeps track if m_automaticPullNodes is modified.

    // Number of SampledAudioNode that are active (playing).
    std::atomic<int> m_activeSourceCount;

    void uninitialize();

    void handleAutomaticSources();
    void updateAutomaticPullNodes();

    std::shared_ptr<AudioDestinationNode> m_destinationNode;
    std::shared_ptr<AudioListener> m_listener;

    // @TODO migrate most of the internal datastructures such as PendingConnection
    // into Internals as there's no need to expose these at all.
    struct Internals;
    std::unique_ptr<Internals> m_internal;

    std::set<std::shared_ptr<AudioNode>> m_automaticPullNodes; // queue for added pull nodes
    std::vector<std::shared_ptr<AudioNode>> m_renderingAutomaticPullNodes; // vector of known pull nodes

    std::vector<std::shared_ptr<AudioScheduledSourceNode>> automaticSources;

    enum class ConnectionType : int
    {
        Disconnect = 0,
        Connect,
        FinishDisconnect
    };

    struct PendingConnection
    {
        ConnectionType type;
        std::shared_ptr<AudioNode> destination;
        std::shared_ptr<AudioNode> source;
        uint32_t destIndex;
        uint32_t srcIndex;
        float duration = 0.1f;
        
        PendingConnection(
            std::shared_ptr<AudioNode> destination,
            std::shared_ptr<AudioNode> source,
            ConnectionType t,
            uint32_t destIndex = 0,
            uint32_t srcIndex = 0)
        : type(t), destination(destination), source(source), destIndex(destIndex), srcIndex(srcIndex) { }
    };

    struct CompareScheduledTime
    {
        bool operator()(const PendingConnection & p1, const PendingConnection & p2)
        {
            if (!p1.destination || !p2.destination) return false;
            if (!p2.destination->isScheduledNode()) return false; // src cannot be compared
            if (!p1.destination->isScheduledNode()) return false; // dest cannot be compared
            AudioScheduledSourceNode * ap2 = static_cast<AudioScheduledSourceNode*>(p2.destination.get());
            AudioScheduledSourceNode * ap1 = static_cast<AudioScheduledSourceNode*>(p1.destination.get());
            return ap2->startTime() < ap1->startTime();
        }
    };

    std::priority_queue<PendingConnection, std::deque<PendingConnection>, CompareScheduledTime> pendingNodeConnections;
    std::queue<std::tuple<std::shared_ptr<AudioParam>, std::shared_ptr<AudioNode>, uint32_t>> pendingParamConnections;
};

} // End namespace lab

#endif // AudioContext_h
