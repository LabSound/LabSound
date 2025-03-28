// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef lab_audio_context_h
#define lab_audio_context_h

#include "LabSound/core/AudioScheduledSourceNode.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace lab {

class AudioBus;
class AudioContext;
class AudioHardwareInputNode;
class AudioListener;
class AudioNode;
class AudioNodeInput;
class AudioNodeOutput;
class AudioDestinationNode;
class AudioScheduledSourceNode;
class ContextGraphLock;
class ContextRenderLock;
class HRTFDatabaseLoader;



/**
 * Example: Creating a wet/dry audio processing chain using LabSound nodes.
 *
 * Graph Overview:
 * - A SampledAudioNode plays a sound.
 * - The dry chain connects directly to the AudioContext.
 * - The wet chain passes through:
 *   - A reverb effect (ConvolverNode).
 *   - A compressor (DynamicsCompressorNode) before merging.
 * - Both chains merge at a GainNode before reaching the AudioContext.
 *
 * Example 1: Explicit Chains
 * --------------------------------------------
    auto source = std::make_shared<SampledAudioNode>(ctx);
    auto reverb = std::make_shared<ConvolverNode>(ctx);
    auto compressor = std::make_shared<DynamicsCompressorNode>(ctx);
    auto dryGain = std::make_shared<GainNode>(ctx);
    auto wetGain = std::make_shared<GainNode>(ctx);
    auto masterGain = std::make_shared<GainNode>(ctx);

    dryGain->gain()->setValue(0.5f);
    wetGain->gain()->setValue(0.5f);

    auto dryChain = source >> dryGain;
    auto wetChain = source >> reverb >> compressor >> wetGain;

    dryChain >> masterGain;
    wetChain >> masterGain;
    masterGain >> ctx;
 *
 * Example 2: Alternative Construction
 * --------------------------------------------
 * Using a single stream, merging inside the chain:
 *
    source >> (dryGain >> masterGain);
    source >> (reverb >> compressor >> wetGain >> masterGain);
    masterGain >> ctx;
 *
 * Both dry and wet paths flow into masterGain, then into ctx.
 *
 * Example 3: Creating a Subgraph
 * --------------------------------------------
    auto wetEffect = reverb >> compressor >> wetGain;
    source >> dryGain >> masterGain;
    source >> wetEffect >> masterGain;
    masterGain >> ctx;
 *
 * Here, 'wetEffect' is a reusable subgraph, making the graph modular.
 *
 * Example 4: A Sidechain helper
 * ---------------------------------------------
    auto createFilterChain = [&](AudioContext& ctx) {
        auto gain = std::make_shared<GainNode>(ctx);
        auto compressor = std::make_shared<DynamicsCompressorNode>(ctx);
        auto postGain = std::make_shared<GainNode>(ctx);
        
        return gain >> compressor >> postGain;
    };

    // Now apply it separately to different sources:
    auto music1 = std::make_shared<SampledAudioNode>(ctx);
    auto music2 = std::make_shared<SampledAudioNode>(ctx);

    music1 >> createFilterChain(ctx) >> ctx;
    music2 >> createFilterChain(ctx) >> ctx;
 *
 * Here, a function creates a subgraph for sidechain processing.
 *
 * Example 5: Connecting to a param
 * ---------------------------------------------

auto lfo = std::make_shared<OscillatorNode>(ctx);
auto gain = std::make_shared<GainNode>(ctx);
auto filter = std::make_shared<BiquadFilterNode>(ctx);
auto gain2 = std::make_shared<GainNode>(ctx);

// Valid: Modulating the filter's frequency with an LFO
lfo >> gain >> filter->param("frequency") >> ctx;

// Compile-time error: Connecting an AudioParam to an AudioNode is illegal
// lfo >> gain >> filter->param("frequency") >> gain2;

* It's easy to create a chain of nodes and drive a parameter from them.
*
* ==============================================
 * Takeaways:
 * - Both explicit and concise constructions are possible.
 * - Encourages graph-style thinking.
 * - Modular subgraphs enable reuse.
 */


class ContextConnector {
public:
    explicit ContextConnector(AudioContext& ctx) : ctx(ctx) {}

    // Connects the last node in the chain to the destination node of ctx
    void operator>>(std::shared_ptr<AudioNode> lastNode) const;

private:
    AudioContext& ctx;
};

class ConnectionChain {
public:
    using NodePtr = std::shared_ptr<AudioNode>;
    using ParamPtr = std::shared_ptr<AudioParam>;

    explicit ConnectionChain(NodePtr node) {
        nodes.push_back(node);
    }

    // Node-to-Node connection (valid)
    ConnectionChain& operator>>(NodePtr next) {
        if (targetParam) {
            //throw std::logic_error("Cannot connect a node after an AudioParam!");
        }
        else {
            nodes.push_back(next);
        }
        return *this;
    }

    // Node-to-Param connection (valid)
    ConnectionChain& operator>>(ParamPtr param) {
        if (!nodes.empty()) {
            targetParam = param;
        } 
        else {
            //throw std::logic_error("Cannot connect an AudioParam without a source node!");
        }
        return *this;
    }

    // Prevent Param-to-Node connection at compile time
    template <typename T>
    ConnectionChain& operator>>(std::shared_ptr<T> next) {
        static_assert(!std::is_same<T, AudioNode>::value,
                    "Invalid connection: Cannot connect an AudioParam to an AudioNode!");
        return *this;
    }

    void operator>>(AudioContext& ctx);

private:
    std::vector<NodePtr> nodes;
    ParamPtr targetParam ;
};

// Overload >> for node-to-node connection
inline ConnectionChain operator>>(std::shared_ptr<AudioNode> src, std::shared_ptr<AudioNode> dst) {
    return ConnectionChain(src) >> dst;
}

// Overload >> for node-to-param connection
inline ConnectionChain operator>>(std::shared_ptr<AudioNode> src, std::shared_ptr<AudioParam> param) {
    return ConnectionChain(src) >> param;
}

// Prevent param-to-node connection at compile time
template <typename T>
inline ConnectionChain operator>>(std::shared_ptr<AudioParam> src, std::shared_ptr<T> dst) {
    static_assert(!std::is_same<T, AudioNode>::value,
                  "Invalid connection: Cannot connect an AudioParam to an AudioNode!");
    return ConnectionChain(nullptr); // Won't be reached due to static_assert
}

class AudioContext
{
    friend class ContextGraphLock;
    friend class ContextRenderLock;
    friend class AudioDestinationNode;

public:

    // AudioNodes should not retain the AudioContext. The majority of
    // methods that take AudioContext as a parameter do so via reference.
    // 
    // If methods from the context, such as currentTime() are
    // required in methods that do not take AudioContext as a parameter, 
    // they should instead retain a weak pointer to the 
    // AudioNodeInterface from the AudioContext they were instantiated from. 
    // 
    // Locking the AudioNodeInterface pointer is a way to check if the 
    // AudioContext is still instantiated.
    // 
    // AudioContextInterface contains data that is safe for any thread, or
    // audio processing callback to read, even if the audio context is in
    // the process of stopping or destruction.

    class AudioContextInterface
    {
        friend class AudioContext;

        int _id = 0;
        AudioContext * _ac = nullptr;
        double _currentTime = 0;

    public:
        AudioContextInterface(AudioContext * ac, int id)
            : _id(id)
            , _ac(ac)
        {
        }

        ~AudioContextInterface() { }

        // the contextId of two AudioNodeInterfaces can be compared
        // to discover if they refer to the same context.
        int contextId() const { return _id; }
        double currentTime() const { return _currentTime; }
    };

    std::weak_ptr<AudioContextInterface> audioContextInterface() { return m_audioContextInterface; }


    // ctor/dtor
    explicit AudioContext(bool isOffline);
    explicit AudioContext(bool isOffline, bool autoDispatchEvents);
    ~AudioContext();

    // External users shouldn't use this; it should be called by
    // LabSound::MakeRealtimeAudioContext(lab::Channels::Stereo)
    // It *is* harmless to call it though, it's just not necessary.
    void lazyInitialize();
    bool isInitialized() const;

    // configuration
    bool isAutodispatchingEvents() const;
    bool isOfflineContext() const;
    bool loadHrtfDatabase(const std::string & searchPath);
    std::shared_ptr<HRTFDatabaseLoader> hrtfDatabaseLoader() const;

    float sampleRate() const;

    void setDestinationNode(std::shared_ptr<AudioDestinationNode> node);
    std::shared_ptr<AudioDestinationNode> destinationNode();
    std::shared_ptr<AudioListener> listener();

    // Debugging/Sanity Checking
    std::string m_graphLocker;
    std::string m_renderLocker;
    void debugTraverse(AudioNode * root);
    void diagnose(std::shared_ptr<AudioNode>);
    void diagnosed_silence(const char*  msg);
    std::shared_ptr<AudioNode> diagnosing() const { return _diagnose; }

    // Timing related

    // The current time, measured at the start of the render quantum currently
    // being processed. Most useful in a Node's process routine.
    double currentTime() const;

    // The current epoch (total count of previously processed render quanta)
    // Useful in recurrent graphs to discover if a node has already done its
    // processing for the present quanta.
    uint64_t currentSampleFrame() const;

    // The current time, accurate versus the audio clock. Whereas currentTime
    // advances discretely, once per render quanta, predictedCurrentTime
    // is the sum of currentTime, and the amount of realworld time that has
    // elapsed since the start of the current render quanta. This is useful on
    // the main thread of an application in order to precisely synchronize
    // expected audio events and other systems.
    double predictedCurrentTime() const;

    // engine
    
    void startOfflineRendering();
    std::function<void()> offlineRenderCompleteCallback;

    // suspend the progression of time and processing in the audio context,
    // any queued samples will play
    void suspend();

    // if the context was suspended, resume the progression of time and processing
    // in the audio context
    void resume();
    
    // Close releases any system audio resources that it uses. It is asynchronous,
    // returning a promise object that can wait() until all AudioContext-creation-blocking
    // resources have been released. Closed contexts can decode audio data, create
    // buffers, etc. Closing the context will forcibly release any system audio resources
    // that might prevent additional AudioContexts from being created and used, suspend
    // the progression of audio time in the audio context, and stop processing audio data.
    void close();

    // Called at the start of each render quantum.
    void handlePreRenderTasks(ContextRenderLock &);

    // Called at the end of each render quantum.
    void handlePostRenderTasks(ContextRenderLock &);

    // AudioContext can pull node(s) at the end of each render quantum even
    // when they are not connected to any downstream nodes. These two methods
    // are called by the nodes who want to add/remove themselves into/from the
    // automatic pull lists.
    void addAutomaticPullNode(std::shared_ptr<AudioNode>);
    void removeAutomaticPullNode(std::shared_ptr<AudioNode>);

    // Called right before handlePostRenderTasks() to handle nodes which need to
    // be pulled even when they are not connected to anything.
    // Only an AudioDestinationNode should call this.
    void processAutomaticPullNodes(ContextRenderLock &, int framesToProcess);

    // graph management
    //
    void connect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, int destIdx = 0, int srcIdx = 0);
    void disconnect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, int destIdx = 0, int srcidx = 0);
    bool isConnected(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source);

    // completely disconnect the node from the graph
    void disconnect(std::shared_ptr<AudioNode> node, int destIdx = 0);

    ContextConnector connector() {
        return ContextConnector(*this);
    }

    // connecting and disconnecting busses and parameters occurs asynchronously.
    // synchronizeConnections will block until there are no pending connections,
    // or until the timeout occurs.
    void synchronizeConnections(int timeOut_ms = 1000);

    // parameter management
    //
    // connect a parameter to receive the indexed output of a node
    void connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index);

    // connect destinationNode's named parameter input to driver's indexed output
    void connectParam(std::shared_ptr<AudioNode> destinationNode, char const*const parameterName, std::shared_ptr<AudioNode> driver, int index);

    // disconnect a parameter from the indexed output of a node
    void disconnectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index);

    // events
    //    
    // event dispatching will be called automatically, depending on constructor
    // argument. If not automatically dispatching, it is the user's responsibility
    // to call dispatchEvents often enough to satisfy the user's needs.
    void enqueueEvent(std::function<void()> &);
    void dispatchEvents();

    void appendDebugBuffer(AudioBus* bus, int channel, int count);
    void flushDebugBuffer(char const* const wavFilePath);

private:
    // @TODO migrate most of the internal datastructures such as
    // PendingConnection into Internals as there's no need to expose these at all.
    struct Internals;
    std::unique_ptr<Internals> m_internal;

    std::shared_ptr<AudioContextInterface> m_audioContextInterface;

    std::mutex m_graphLock;
    std::mutex m_renderLock;
    std::mutex m_updateMutex;
    std::condition_variable cv;

    // -1 means run forever, 0 means stop, n > 0 means run this many times
    // n > 0 will decrement to zero each time update runs.
    std::atomic<int> updateThreadShouldRun{-1};
    std::thread graphUpdateThread;
    float graphKeepAlive{0.f};
    float lastGraphUpdateTime{0.f};

    std::atomic<int> _contextIsInitialized{0};
    bool m_isAudioThreadFinished = false;
    bool m_isOfflineContext = false;
    bool m_automaticPullNodesNeedUpdating = false;  // indicates m_automaticPullNodes was modified.

    friend class NullDeviceNode; // needs to be able to call update()
    void update();
    void updateAutomaticPullNodes();
    void uninitialize();

    std::shared_ptr<AudioDestinationNode> _destinationNode;

    std::shared_ptr<AudioListener> m_listener;
    std::shared_ptr<AudioNode> _diagnose;
    std::set<std::shared_ptr<AudioNode>> m_automaticPullNodes;  // queue for added pull nodes
    std::vector<std::shared_ptr<AudioNode>> m_renderingAutomaticPullNodes;  // vector of known pull nodes
};


}  // End namespace lab

#endif  // lab_audio_context_h
