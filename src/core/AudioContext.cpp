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

#include "concurrentqueue/concurrentqueue.h"
#include "libnyquist/Encoders.h"

#include <assert.h>
#include <queue>
#include <stdio.h>

namespace lab
{
enum class ConnectionOperationKind : int
{
    DisconnectNode = 0,
    DisconnectInput,
    DisconnectParam,
    ConnectNode,
    ConnectParam,
    FinishDisconnectNode,
    FinishDisconnectInput,
};

struct PendingNodeConnection
{
    ConnectionOperationKind type;
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
    ConnectionOperationKind type;
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
                        node_connection.source, node_connection.srcIndex);

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


void AudioContext::connect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, int destIdx, int srcIdx)
{
    if (!destination)
        throw std::runtime_error("Cannot connect to null destination");
    if (!source)
        throw std::runtime_error("Cannot connect from null source");
    if (srcIdx > source->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs");
    if (destIdx > destination->numberOfInputs())
        throw std::out_of_range("Input index greater than available inputs");
    m_internal->pendingNodeConnections.enqueue(
        {ConnectionOperationKind::ConnectNode, destination, source, destIdx, srcIdx});
}



// disconnect source from destination, at index. -1 means disconnect the
// source node from all inputs it is connected to.
void AudioContext::disconnectNode(std::shared_ptr<AudioNode> destination,
                                  std::shared_ptr<AudioNode> source,
                                  int inletIdx, int outletIdx)
{
    if (!destination && !source)
        return;
    if (source && outletIdx > source->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs");
    if (destination && inletIdx > destination->numberOfInputs())
        throw std::out_of_range("Input index greater than available inputs");
    m_internal->pendingNodeConnections.enqueue(
        {ConnectionOperationKind::DisconnectNode, destination, source, outletIdx, inletIdx});
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


void AudioContext::connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index)
{
    if (!param)
        throw std::invalid_argument("No parameter specified");
    if (!driver)
        throw std::invalid_argument("No driving node supplied");
    if (index >= driver->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs on the driver");
    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::ConnectParam, param, driver, index});
}


// connect a named parameter on a node to receive the indexed output of a node
void AudioContext::connectParam(std::shared_ptr<AudioNode> destinationNode, char const*const parameterName,
                                std::shared_ptr<AudioNode> driver, int index)
{
    if (!parameterName)
        throw std::invalid_argument("No parameter specified");

    std::shared_ptr<AudioParam> param = destinationNode->param(parameterName);
    if (!param)
        throw std::invalid_argument("Parameter not found on node");

    if (!driver)
        throw std::invalid_argument("No driving node supplied");

    if (index >= driver->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs on the driver");

    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::ConnectParam, param, driver, index});
}


void AudioContext::disconnectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index)
{
    if (!param)
        throw std::invalid_argument("No parameter specified");

    if (index >= driver->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs on the driver");

    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::DisconnectParam, param, driver, index});
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


}  // End namespace lab
