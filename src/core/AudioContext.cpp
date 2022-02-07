// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"
#include "LabSound/core/AudioHardwareInputNode.h"
#include "LabSound/core/AudioListener.h"
#include "LabSound/core/NullDeviceNode.h"
#include "LabSound/core/OscillatorNode.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/DenormalDisabler.h"

#include "concurrentqueue/concurrentqueue.h"
#include "libnyquist/Encoders.h"

#include <assert.h>
#include <queue>
#include <stdio.h>

namespace lab
{
enum class ConnectionOperationKind : int
{
    None = 0,
    DisconnectNode,
    DisconnectInput,
    DisconnectParam,
    ConnectNode,
    ConnectParam,
    FinishDisconnectNode,
    FinishDisconnectInput,
};

struct PendingNodeConnection
{
    ConnectionOperationKind type = ConnectionOperationKind::None;
    std::shared_ptr<AudioNode> destination;
    std::shared_ptr<AudioNode> source;
    int dstIndex = 0;
    int srcIndex = 0;
    float duration = 0.1f;

    PendingNodeConnection() = default;
    ~PendingNodeConnection() = default;
};

struct PendingParamConnection
{
    ConnectionOperationKind type = ConnectionOperationKind::None;
    std::shared_ptr<AudioParam> destination;
    std::shared_ptr<AudioNode> source;
    int dstIndex = 0;

    PendingParamConnection() = default;
    ~PendingParamConnection() = default;
};

struct AudioContext::Internals
{
    Internals(bool a)
        : autoDispatchEvents(a)
    {
    }
    ~Internals() = default;

    bool autoDispatchEvents;
    moodycamel::ConcurrentQueue<std::function<void()>> enqueuedEvents;
    moodycamel::ConcurrentQueue<PendingNodeConnection> pendingNodeConnections;
    moodycamel::ConcurrentQueue<PendingParamConnection> pendingParamConnections;

    std::vector<float> debugBuffer;
    const int debugBufferCapacity = 1024 * 1024;
    int debugBufferIndex = 0;

    void appendDebugBuffer(AudioBus* bus, int channel, int count)
    {
        if (!bus || bus->numberOfChannels() < channel || !count)
            return;

        if (!debugBuffer.size())
        {
            debugBuffer.resize(debugBufferCapacity);
            memset(debugBuffer.data(), 0, debugBufferCapacity);
        }

        if (debugBufferIndex + count > debugBufferCapacity)
            debugBufferIndex = 0;

        memcpy(debugBuffer.data() + debugBufferIndex, bus->channel(channel)->data(), sizeof(float) * count);
        debugBufferIndex += count;
    }

    void flushDebugBuffer(char const* const wavFilePath)
    {
        if (!debugBufferIndex || !wavFilePath)
            return;

        nqr::AudioData fileData;
        fileData.samples.resize(debugBufferIndex + 32);
        fileData.channelCount = 1;
        float* dst = fileData.samples.data();
        memcpy(dst, debugBuffer.data(), sizeof(float) * debugBufferIndex);
        fileData.sampleRate = static_cast<int>(44100);
        fileData.sourceFormat = nqr::PCM_FLT;
        nqr::EncoderParams params = { 1, nqr::PCM_FLT, nqr::DITHER_NONE };
        int err = nqr::encode_wav_to_disk(params, &fileData, wavFilePath);
        debugBufferIndex = 0;
    }
};

void AudioContext::appendDebugBuffer(AudioBus* bus, int channel, int count)
{
    m_internal->appendDebugBuffer(bus, channel, count);
}

void AudioContext::flushDebugBuffer(char const* const wavFilePath)
{
    m_internal->flushDebugBuffer(wavFilePath);
}

AudioContext::AudioContext(bool isOffline, bool autoDispatchEvents)
    : m_isOfflineContext(isOffline)
{
    static std::atomic<int> id {1};
    m_internal.reset(new AudioContext::Internals(autoDispatchEvents));
    m_listener.reset(new AudioListener());
    m_audioContextInterface = std::make_shared<AudioContextInterface>(this, id);
    ++id;

    if (isOffline)
    {
        updateThreadShouldRun = 1;
        graphKeepAlive = 0;
    }
}

AudioContext::~AudioContext()
{
    LOG_TRACE("Begin AudioContext::~AudioContext()");

    m_audioContextInterface.reset();

    if (!isOfflineContext())
        graphKeepAlive = 0.25f;

    updateThreadShouldRun = 0;
    cv.notify_all();

    if (graphUpdateThread.joinable())
        graphUpdateThread.join();

    m_listener.reset();

    uninitialize();

#if USE_ACCELERATE_FFT
    FFTFrame::cleanup();
#endif

    ASSERT(!m_isInitialized);

    LOG_INFO("Finish AudioContext::~AudioContext()");
}

void AudioContext::lazyInitialize()
{
    if (m_isInitialized)
        return;

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    ASSERT(!m_isAudioThreadFinished);
    if (m_isAudioThreadFinished)
        return;

    if (m_device.get())
    {
        if (!isOfflineContext())
        {
            // This starts the audio thread and all audio rendering.
            // The destination node's provideInput() method will now be called repeatedly to render audio.
            // Each time provideInput() is called, a portion of the audio stream is rendered.

            graphKeepAlive = 0.25f;  // pump the graph for the first 0.25 seconds
            graphUpdateThread = std::thread(&AudioContext::update, this);
            device_callback->start();
        }

        cv.notify_all();
        m_isInitialized = true;
    }
    else
    {
        LOG_ERROR("m_device not specified");
        ASSERT(m_device);
    }
}

void AudioContext::uninitialize()
{
    LOG_TRACE("AudioContext::uninitialize()");

    if (!m_isInitialized)
        return;

    // This stops the audio thread and all audio rendering.
    device_callback->stop();

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    m_isAudioThreadFinished = true;

    m_isInitialized = false;
}

bool AudioContext::isInitialized() const
{
    return m_isInitialized;
}

void AudioContext::handlePreRenderTasks(ContextRenderLock & r)
{
    // these may be null during application exit.
    if (!r.context() || !m_audioContextInterface)
        return;

    // At the beginning of every render quantum, update the graph.

    m_audioContextInterface->_currentTime = currentTime();

    // check for pending connections
    if (m_internal->pendingParamConnections.size_approx() > 0 ||
        m_internal->pendingNodeConnections.size_approx() > 0)
    {
        // resolve parameter connections
        PendingParamConnection param_connection;
        while (m_internal->pendingParamConnections.try_dequeue(param_connection))
        {
            if (param_connection.type == ConnectionOperationKind::ConnectParam)
            {
                param_connection.destination->connect(param_connection.source, param_connection.dstIndex);

                // if unscheduled, the source should start to play as soon as possible
                if (!param_connection.source->isScheduledNode())
                    param_connection.source->_scheduler.start(0);
            }
            else
                param_connection.destination->disconnect(param_connection.source, param_connection.dstIndex);
        }

        // resolve node connections
        PendingNodeConnection node_connection;
        std::vector<PendingNodeConnection> requeued_connections;
        while (m_internal->pendingNodeConnections.try_dequeue(node_connection))
        {
            switch (node_connection.type)
            {
                case ConnectionOperationKind::ConnectNode:
                {
                    node_connection.destination->connect(r, node_connection.dstIndex, 
                        node_connection.source);

                    if (!node_connection.source->isScheduledNode())
                        node_connection.source->_scheduler.start(0);
                }
                break;

                case ConnectionOperationKind::DisconnectInput:
                {
                    if (node_connection.destination)
                    {
                        // destination will be completely disconnected
                        node_connection.destination->scheduleDisconnect();
                        node_connection.type = ConnectionOperationKind::FinishDisconnectInput;
                        requeued_connections.push_back(node_connection);  // enqueue Finish
                    }
                }
                break;

                case ConnectionOperationKind::DisconnectNode:
                {
                    if (node_connection.destination)
                    {
                        if (node_connection.source)
                        {
                            // destination will be completely disconnected
                            node_connection.destination->scheduleDisconnect();

                            node_connection.type = ConnectionOperationKind::FinishDisconnectNode;
                            requeued_connections.push_back(node_connection);  // save for later
                        }
                        else
                        {
                            node_connection.type = ConnectionOperationKind::FinishDisconnectInput;
                            requeued_connections.push_back(node_connection);  // save for later
                        }
                    }
                    else if (node_connection.source)
                    {
                        /// A source can't be disconnected from nothing, do nothing
                    }
                }
                break;

                case ConnectionOperationKind::FinishDisconnectInput:
                {
                    if (node_connection.duration > 0)
                    {
                        node_connection.duration -= AudioNode::ProcessingSizeInFrames / sampleRate();
                        requeued_connections.push_back(node_connection);
                        continue;
                    }

                    if (node_connection.dstIndex == -1)
                        node_connection.destination->disconnectAll();
                    else
                        node_connection.destination->disconnect(node_connection.dstIndex);
                }
                break;

                case ConnectionOperationKind::FinishDisconnectNode:
                {
                    if (node_connection.duration > 0)
                    {
                        node_connection.duration -= AudioNode::ProcessingSizeInFrames / sampleRate();
                        requeued_connections.push_back(node_connection);
                        continue;
                    }

                    // The logic says we can't get here with a null destination,
                    // and a null source would have been redirected to FinishDisconnectInput
                    ASSERT(node_connection.destination && node_connection.source);

                    /// @TODO nodes need a fine grained disconnect that use the inlet index
                    /// and outlet index as a predicate. AT the moment the disconnection
                    /// will disregard those
                    node_connection.destination->disconnect(r, node_connection.source);
                }
                break;
            }
        }

        // We have incompletely connected nodes, so next time the thread ticks we can re-check them
        for (auto & sc : requeued_connections)
            m_internal->pendingNodeConnections.enqueue(sc);
    }
}

void AudioContext::handlePostRenderTasks(ContextRenderLock & r)
{
    ASSERT(r.context());
    // there no post render tasks in the current architecture
}

void AudioContext::synchronizeConnections(int timeOut_ms)
{
    cv.notify_all();

    // don't synch if the context is suspended as that will simply max out the timeout
    if (!device_callback->isRunning())
        return;

    while (m_internal->pendingNodeConnections.size_approx() > 0 && timeOut_ms > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        timeOut_ms -= 5;
    }
}


void AudioContext::connect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, int destIdx)
{
    if (!destination)
        throw std::runtime_error("Cannot connect to null destination");
    if (!source)
        throw std::runtime_error("Cannot connect from null source");
    if (destIdx > destination->numberOfInputs())
        throw std::out_of_range("Input index greater than available inputs");
    m_internal->pendingNodeConnections.enqueue(
        {ConnectionOperationKind::ConnectNode, destination, source, destIdx, 0});
}



// disconnect source from destination, at index. -1 means disconnect the
// source node from all inputs it is connected to.
void AudioContext::disconnectNode(std::shared_ptr<AudioNode> destination,
                                  std::shared_ptr<AudioNode> source,
                                  int inletIdx)
{
    if (!destination && !source)
        return;
    if (destination && inletIdx > destination->numberOfInputs())
        throw std::out_of_range("Input index greater than available inputs");
    m_internal->pendingNodeConnections.enqueue(
        {ConnectionOperationKind::DisconnectNode, destination, source, 0, inletIdx});
}

// disconnect all the node's inputs at the specified inlet. -1 means disconnect all input inlets.
void AudioContext::disconnectInput(std::shared_ptr<AudioNode> node, int inlet)
{
    if (!node)
        return;
    m_internal->pendingNodeConnections.enqueue(
        {ConnectionOperationKind::DisconnectInput, node, std::shared_ptr<AudioNode>(), inlet, 0});
}

bool AudioContext::isConnected(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source)
{
    if (!destination || !source)
        return false;

    return destination->isConnected(source) || source->isConnected(destination);
}


void AudioContext::connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver)
{
    if (!param)
        throw std::invalid_argument("No parameter specified");
    if (!driver)
        throw std::invalid_argument("No driving node supplied");
    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::ConnectParam, param, driver, 0});
}


// connect a named parameter on a node to receive the indexed output of a node
void AudioContext::connectParam(std::shared_ptr<AudioNode> destinationNode, char const*const parameterName,
                                std::shared_ptr<AudioNode> driver)
{
    if (!parameterName)
        throw std::invalid_argument("No parameter specified");

    std::shared_ptr<AudioParam> param = destinationNode->param(parameterName);
    if (!param)
        throw std::invalid_argument("Parameter not found on node");

    if (!driver)
        throw std::invalid_argument("No driving node supplied");

    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::ConnectParam, param, driver, 0});
}


void AudioContext::disconnectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver)
{
    if (!param)
        throw std::invalid_argument("No parameter specified");

    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::DisconnectParam, param, driver, 0});
}

void AudioContext::update()
{
    if (!m_isOfflineContext) { LOG_TRACE("Begin UpdateGraphThread"); }

    const float frameLengthInMilliseconds = (sampleRate() / (float) AudioNode::ProcessingSizeInFrames) / 1000.f;  // = ~0.345ms @ 44.1k/128
    const float graphTickDurationMs = frameLengthInMilliseconds * 16;  // = ~5.5ms
    const uint32_t graphTickDurationUs = static_cast<uint32_t>(graphTickDurationMs * 1000.f);  // = ~5550us

    ASSERT(frameLengthInMilliseconds);
    ASSERT(graphTickDurationMs);
    ASSERT(graphTickDurationUs);

    // graphKeepAlive keeps the thread alive momentarily (letting tail tasks
    // finish) even updateThreadShouldRun has been signaled.
    while (updateThreadShouldRun != 0 || graphKeepAlive > 0)
    {
        if (updateThreadShouldRun > 0)
            --updateThreadShouldRun;

        // A `unique_lock` automatically acquires a lock on construction. The purpose of
        // this mutex is to synchronize updates to the graph from the main thread,
        // primarily through `connect(...)` and `disconnect(...)`.
        std::unique_lock<std::mutex> lk;

        if (!m_isOfflineContext)
        {
            lk = std::unique_lock<std::mutex>(m_updateMutex);
            cv.wait_until(lk, std::chrono::steady_clock::now() + std::chrono::microseconds(5000)); // awake every five milliseconds
        }

        if (m_internal->autoDispatchEvents)
            dispatchEvents();

        {
            const double now = currentTime();
            const float delta = static_cast<float>(now - lastGraphUpdateTime);
            lastGraphUpdateTime = static_cast<float>(now);
            graphKeepAlive -= delta;
        }

        if (lk.owns_lock())
            lk.unlock();

        if (!updateThreadShouldRun)
            break;
    }

    if (!m_isOfflineContext) { 
        LOG_TRACE("End UpdateGraphThread"); 
    }
}

void AudioContext::enqueueEvent(std::function<void()> & fn)
{
    m_internal->enqueuedEvents.enqueue(fn);
    cv.notify_all();  // processing thread must dispatch events
}

void AudioContext::dispatchEvents()
{
    std::function<void()> event_fn;
    while (m_internal->enqueuedEvents.try_dequeue(event_fn))
    {
        if (event_fn) event_fn();
    }
}

void AudioContext::setDeviceNode(std::shared_ptr<AudioNode> device)
{
    m_device = device;

    if (auto * callback = dynamic_cast<AudioDeviceRenderCallback *>(device.get()))
    {
        device_callback = callback;
    }
}

std::shared_ptr<AudioNode> AudioContext::device()
{
    return m_device;
}

bool AudioContext::isOfflineContext() const
{
    return m_isOfflineContext;
}

std::shared_ptr<AudioListener> AudioContext::listener()
{
    return m_listener;
}

double AudioContext::currentTime() const
{
    return device_callback->getSamplingInfo().current_time;
}

uint64_t AudioContext::currentSampleFrame() const
{
    return device_callback->getSamplingInfo().current_sample_frame;
}

double AudioContext::predictedCurrentTime() const
{
    auto info = device_callback->getSamplingInfo();
    uint64_t t = info.current_sample_frame;
    double val = t / info.sampling_rate;
    auto t2 = std::chrono::high_resolution_clock::now();
    int index = t & 1;

    if (!info.epoch[index].time_since_epoch().count())
        return val;

    std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - info.epoch[index];
    return val + elapsed.count();
}

float AudioContext::sampleRate() const
{
    // sampleRate is called during AudioNode construction to initialize the
    // scheduler, but DeviceNodes are not scheduled.
    // during construction of DeviceNodes, the device_callback will not yet be
    // ready, so bail out.
    if (!device_callback)
        return 0;

    return device_callback->getSamplingInfo().sampling_rate;
}

void AudioContext::startOfflineRendering()
{
    if (!m_isOfflineContext)
        throw std::runtime_error("context was not constructed for offline rendering");

    m_isInitialized = true;
    device_callback->start();
}

void AudioContext::suspend()
{
    device_callback->stop();
}

// if the context was suspended, resume the progression of time and processing in the audio context
void AudioContext::resume()
{
    device_callback->start();
}

void AudioContext::debugTraverse(
    AudioNode * required_inlet,
    AudioBus * src, AudioBus * dst,
    int frames,
    const SamplingInfo & info,
    AudioHardwareInput * optional_hardware_input)
{
    synchronizeConnections();
    auto renderBus = std::unique_ptr<AudioBus>(new AudioBus(2, frames));

    auto ctx = this;

    // bail if shutting down.
    auto ac = audioContextInterface().lock();
    if (!ac)
        return;

    ASSERT(required_inlet);

    ContextRenderLock renderLock(this, "lab::pull_graph");
    if (!renderLock.context()) 
        return;  // return if couldn't acquire lock

    if (!ctx->isInitialized())
    {
        dst->zero();
        return;
    }

    // Denormals can slow down audio processing.
    // Use an RAII object to protect all AudioNodes processed within this scope.

    /// @TODO under what circumstance do they arise?
    /// If they come from input data such as loaded WAV files, they should be corrected
    /// at source. If they can result from signal processing; again, where? The
    /// signal processing should not produce denormalized values.

    DenormalDisabler denormalDisabler;

    // Let the context take care of any business at the start of each render quantum.
    ctx->handlePreRenderTasks(renderLock);

/* pass two, implement hardware input

    // Prepare the local audio input provider for this render quantum.
    if (optional_hardware_input && src)
    {
        optional_hardware_input->set(src);
    }
*/

// process the graph by pulling the inputs, which will recurse the entire processing graph.

    enum WorkType
    {
        processNode,
        silenceOutputs,
        unsilenceOutputs,
        setOutputChannels,
        sumInputs,
        setZeroes,
        manageEnvelope,
    };

    struct Work
    {
        AudioNode * node = nullptr;
        WorkType type;
    };

    std::vector<Work> work;

    std::vector<AudioNode *> node_stack;
    node_stack.push_back(required_inlet);

    static int color = 1;
    required_inlet->color = color;
    ++color;

    static std::map<int, std::vector<AudioBus *>> summing_busses;
    std::vector<AudioBus *> in_use;


    AudioNode * current_node = nullptr;
    while (node_stack.size() > 0)
    {
        current_node = node_stack.back();

        if (current_node->color >= color
            || !current_node->isInitialized()
            || !current_node->_scheduler.update(renderLock, frames)) 
        {
            // if the node is not active, or the has already done, pop it and continue
            node_stack.pop_back();
            continue;
        }

        // currentEpoch true means the node has already been scheduled this epoch
        if (current_node->isScheduledNode()
            && (current_node->_scheduler._playbackState < SchedulingState::FADE_IN
                || current_node->_scheduler._playbackState == SchedulingState::FINISHED))
        {
            // current node is silent, no recursion necessary, continue
            work.push_back({current_node, silenceOutputs});
            node_stack.pop_back();
            continue;
        }

        auto sz = node_stack.size();
        for (auto & p : current_node->_params)
        {
            p->serviceQueue(renderLock);
            for (auto & i : p->inputs().sources)
            {
                if (!i || !(*i))
                    continue;
                auto output = (*i)->outputBus(renderLock);
                if (!output)
                    continue;

                if ((*i)->color >= color)
                    continue;

                auto sb = summing_busses.find(1);
                if (sb == summing_busses.end())
                {
                    summing_busses[1] = {};
                    sb = summing_busses.find(1);
                }
                if (sb->second.size())
                {
                    in_use.push_back(sb->second.back());
                    sb->second.pop_back();
                }
                else
                {
                    in_use.push_back(new AudioBus(1, frames));
                }
                p->_input.summingBus = in_use.back();

                node_stack.push_back(i->get());
            }
        }

        // if there were parameters with inputs, queue up their work next
        if (node_stack.size() > sz)
            continue;

        // Process all of the AudioNodes connected to our inputs.
        sz = node_stack.size();
        for (auto & in : current_node->_inputs)
        {
            if (!in)
                continue;

            for (auto & s : in->sources)
            {
                // only visit nodes that are not yet at the current epoch
                if (s && (*s)->color < color)
                {
                    node_stack.push_back(s->get());
                }
            }
        }

        // if there were inputs on the node current node, queue up their work next
        if (node_stack.size() > sz)
            continue;

        // should this just be "do the stuff"?
        //work.push_back({current_node, setOutputChannels});
        //work.push_back({current_node, sumInputs});
        work.push_back({current_node, processNode});
        //work.push_back({current_node, setZeroes});
        //work.push_back({current_node, manageEnvelope});
        //work.push_back({current_node, unsilenceOutputs});

        // work is done on the current node, retire it
        current_node->color = color;
        node_stack.pop_back();
    }


    for (auto& w : work) {
        // actually do the things in graph order
        if (w.type == processNode) {
            // Process all of the AudioNodes connected to our inputs.
            for (auto & in : w.node->_inputs)
            {
                if (!in)
                    continue;

                // compute the sum of all inputs if there's more than one
                if (in->sources.size() > 1)
                {
                    ASSERT(in->summingBus);

                    int requiredChannels = 1;
                    for (auto & s : in->sources)
                    {
                        if (!s)
                            continue;

                        auto bus = (*s)->outputBus(renderLock);
                        if (bus && bus->numberOfChannels() > requiredChannels)
                            requiredChannels = bus->numberOfChannels();
                    }

                    auto sb = summing_busses.find(requiredChannels);
                    if (sb == summing_busses.end()) {
                        summing_busses[requiredChannels] = {};
                        sb = summing_busses.find(requiredChannels);
                    }
                    if (sb->second.size())
                    {
                        in_use.push_back(sb->second.back());
                        sb->second.pop_back();
                    }
                    else
                    {
                        in_use.push_back(new AudioBus(requiredChannels, frames));
                    }

                    in->summingBus = in_use.back();
                    in->summingBus->zero();
                    for (auto & s : in->sources)
                    {
                        if (!s)
                            continue;
                        auto bus = (*s)->outputBus(renderLock);
                        if (bus)
                            in->summingBus->sumFrom(*bus);
                    }
                }
            }

            // signal processing for the node
            w.node->process(renderLock, frames);
            // there may need to be silence at the beginning or end of the current quantum.

            int start_zero_count = w.node->_scheduler._renderOffset;
            int final_zero_start = w.node->_scheduler._renderOffset + w.node->_scheduler._renderLength;
            int final_zero_count = frames - final_zero_start;

            //  initialize the busses with start and final zeroes.
            if (start_zero_count)
            {
                for (int i = 0; i < w.node->_output.bus->numberOfChannels(); ++i)
                    memset(w.node->_output.bus->channel(i)->mutableData(), 0,
                           sizeof(float) * start_zero_count);
            }

            if (final_zero_count)
            {
                for (int i = 0; i < w.node->_output.bus->numberOfChannels(); ++i)
                    memset(w.node->_output.bus->channel(i)->mutableData() + final_zero_start, 0,
                           sizeof(float) * final_zero_count);
            }

            // clean pops resulting from starting or stopping
            const int start_envelope = 64;
            const int end_envelope = 64;

            #define OOS(x) (float(x) / float(steps))
            if (start_zero_count > 0 || w.node->_scheduler._playbackState == SchedulingState::FADE_IN)
            {

                int steps = start_envelope;
                int damp_start = start_zero_count;
                int damp_end = start_zero_count + steps;
                if (damp_end > frames)
                {
                    damp_end = frames;
                    steps = frames - start_zero_count;
                }

                // damp out the first samples
                if (damp_end > 0)
                    for (int i = 0; i < w.node->_output.bus->numberOfChannels(); ++i)
                    {
                        float * data = w.node->_output.bus->channel(i)->mutableData();
                        for (int j = damp_start; j < damp_end; ++j)
                            data[j] *= OOS(j - damp_start);
                    }
            }

            if (final_zero_count > 0 || w.node->_scheduler._playbackState == SchedulingState::STOPPING)
            {
                int steps = end_envelope;
                int damp_end, damp_start;
                if (!final_zero_count)
                {
                    damp_end = frames - final_zero_count;
                    damp_start = damp_end - steps;
                }
                else
                {
                    damp_end = frames - final_zero_count;
                    damp_start = damp_end - steps;
                }

                if (damp_start < 0)
                {
                    damp_start = 0;
                    steps = damp_end;
                }

                // damp out the last samples
                if (steps > 0)
                {
                    //printf("out: %d %d\n", damp_start, damp_end);
                    for (int i = 0; i < w.node->_output.bus->numberOfChannels(); ++i)
                    {
                        float * data = w.node->_output.bus->channel(i)->mutableData();
                        for (int j = damp_start; j < damp_end; ++j)
                            data[j] *= OOS(damp_end - j);
                    }
                }
            }

            w.node->unsilenceOutputs(renderLock);
        }
        else if (w.type == silenceOutputs)
        {
            w.node->silenceOutputs(renderLock);
        }
    }

    // put the allocated busses back in the pool
    while (in_use.size()) {
        summing_busses[in_use.back()->numberOfChannels()].push_back(in_use.back());
        in_use.pop_back();
    }

    AudioBus * renderedBus = required_inlet->outputBus(renderLock);

    if (!renderedBus)
    {
        dst->zero();
    }
    else if (renderedBus != dst)
    {
        // in-place processing was not possible - so copy
        dst->copyFrom(*renderedBus);
    }

    // Let the context take care of any business at the end of each render quantum.
    ctx->handlePostRenderTasks(renderLock);
}


}  // End namespace lab
