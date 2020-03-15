// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef lab_audio_context_h
#define lab_audio_context_h

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/ConcurrentQueue.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace lab
{

class AudioHardwareDeviceNode;
class AudioListener;
class AudioNode;
class AudioScheduledSourceNode;
class AudioHardwareInputNode;
class AudioNodeInput;
class AudioNodeOutput;
class ContextGraphLock;
class ContextRenderLock;

class AudioContext
{
    friend class ContextGraphLock;
    friend class ContextRenderLock;

public:
    // Debugging/Sanity Checking
    std::string m_graphLocker;
    std::string m_renderLocker;

    explicit AudioContext(bool isOffline, bool autoDispatchEvents = true);
    ~AudioContext();

    bool isInitialized() const;

    // External users shouldn't use this; it should be called by LabSound::MakeRealtimeAudioContext(lab::Channels::Stereo)
    // It *is* harmless to call it though, it's just not necessary.
    void lazyInitialize();

    void setDeviceNode(std::shared_ptr<AudioNode> device);
    std::shared_ptr<AudioNode> device();

    bool isOfflineContext() const;

    // Query AudioDeviceRenderCallback properties
    double currentTime() const;
    uint64_t currentSampleFrame() const;
    float sampleRate() const;

    std::shared_ptr<AudioListener> listener();

    void handlePreRenderTasks(ContextRenderLock &);  // Called at the start of each render quantum.
    void handlePostRenderTasks(ContextRenderLock &);  // Called at the end of each render quantum.

    // AudioContext can pull node(s) at the end of each render quantum even when they are not connected to any downstream nodes.
    // These two methods are called by the nodes who want to add/remove themselves into/from the automatic pull lists.
    void addAutomaticPullNode(std::shared_ptr<AudioNode>);
    void removeAutomaticPullNode(std::shared_ptr<AudioNode>);

    // Called right before handlePostRenderTasks() to handle nodes which need to be pulled even when they are not connected to anything.
    // Only an AudioHardwareDeviceNode should call this.
    void processAutomaticPullNodes(ContextRenderLock &, size_t framesToProcess);

    void connect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx = 0, uint32_t srcIdx = 0);
    void disconnect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, uint32_t destIdx = 0, uint32_t srcidx = 0);

    // completely disconnect the node from the graph
    void disconnect(std::shared_ptr<AudioNode> node, uint32_t destIdx = 0);

    void connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, uint32_t index);
    void disconnectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, uint32_t index);

    void holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode> node);

    void startOfflineRendering();
    std::function<void()> offlineRenderCompleteCallback;

    // event dispatching will be called automatically, depending on constructor
    // argument. If not automatically dispatching, it is the user's responsibility
    // to call dispatchEvents often enough to satisfy the user's needs.
    void enqueueEvent(std::function<void()> &);
    void dispatchEvents();

private:
    // @TODO migrate most of the internal datastructures such as PendingConnection into Internals as there's no need to expose these at all.
    struct Internals;
    std::unique_ptr<Internals> m_internal;

    std::mutex m_graphLock;
    std::mutex m_renderLock;
    std::mutex m_updateMutex;
    std::condition_variable cv;

    std::atomic<bool> updateThreadShouldRun{true};
    std::thread graphUpdateThread;
    void update();
    float graphKeepAlive{0.f};
    float lastGraphUpdateTime{0.f};

    bool m_isInitialized = false;
    bool m_isAudioThreadFinished = false;
    bool m_isOfflineContext = false;
    bool m_automaticPullNodesNeedUpdating = false;  // keeps track if m_automaticPullNodes is modified.

    void uninitialize();

    void handleAutomaticSources();
    void updateAutomaticPullNodes();

    AudioDeviceRenderCallback * device_callback{nullptr};
    std::shared_ptr<AudioNode> m_device;

    std::shared_ptr<AudioListener> m_listener;

    std::set<std::shared_ptr<AudioNode>> m_automaticPullNodes;  // queue for added pull nodes
    std::vector<std::shared_ptr<AudioNode>> m_renderingAutomaticPullNodes;  // vector of known pull nodes
    std::vector<std::shared_ptr<AudioScheduledSourceNode>> automaticSources;
};

}  // End namespace lab

#endif  // lab_audio_context_h
