// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"
#include "LabSound/core/AudioHardwareInputNode.h"
#include "LabSound/core/AudioListener.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/NullDeviceNode.h"
#include "LabSound/core/OscillatorNode.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"

#include "readerwriterqueue/readerwriterqueue.h"

#include <assert.h>
#include <queue>
#include <stdio.h>

namespace lab
{
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
    int destIndex;
    int srcIndex;
    float duration = 0.1f;

    PendingConnection(
        std::shared_ptr<AudioNode> destination,
        std::shared_ptr<AudioNode> source,
        ConnectionType t,
        int destIndex = 0,
        int srcIndex = 0)
        : type(t)
        , destination(destination)
        , source(source)
        , destIndex(destIndex)
        , srcIndex(srcIndex)
    {
    }
};

struct AudioContext::Internals
{
    Internals(bool a)
        : autoDispatchEvents(a)
    {
    }
    ~Internals() = default;

    moodycamel::ReaderWriterQueue<std::function<void()>> enqueuedEvents;
    bool autoDispatchEvents;

    struct CompareScheduledTime
    {
        bool operator()(const PendingConnection & p1, const PendingConnection & p2)
        {
            if (!p1.destination || !p2.destination) return false;
            if (!p2.destination->isScheduledNode()) return false;  // src cannot be compared
            if (!p1.destination->isScheduledNode()) return false;  // dest cannot be compared
            AudioScheduledSourceNode * ap2 = static_cast<AudioScheduledSourceNode *>(p2.destination.get());
            AudioScheduledSourceNode * ap1 = static_cast<AudioScheduledSourceNode *>(p1.destination.get());
            return ap2->startWhen() < ap1->startWhen();
        }
    };

    std::priority_queue<PendingConnection, std::deque<PendingConnection>, CompareScheduledTime> pendingNodeConnections;
    std::queue<std::tuple<std::shared_ptr<AudioParam>, std::shared_ptr<AudioNode>, ConnectionType, int>> pendingParamConnections;
};

// Constructor for realtime rendering
AudioContext::AudioContext(bool isOffline, bool autoDispatchEvents)
    : m_isOfflineContext(isOffline)
{
    m_internal.reset(new AudioContext::Internals(autoDispatchEvents));
    m_listener.reset(new AudioListener());

    if (isOffline)
    {
        updateThreadShouldRun = 1;
        graphKeepAlive = 0;
    }
}

AudioContext::~AudioContext()
{
    // LOG can block.
    // LOG("Begin AudioContext::~AudioContext()");

    if (!isOfflineContext())
        graphKeepAlive = 0.25f;

    updateThreadShouldRun = 0;
    cv.notify_all();

    if (graphUpdateThread.joinable())
        graphUpdateThread.join();

    uninitialize();

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
    LOG("AudioContext::uninitialize()");

    if (!m_isInitialized)
        return;

    // for the case where an OfflineAudioDestinationNode needs to update the graph:
    updateAutomaticPullNodes();

    // This stops the audio thread and all audio rendering.
    device_callback->stop();

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    m_isAudioThreadFinished = true;

    updateAutomaticPullNodes();  // added for the case where an NullDeviceNode needs to update the graph

    m_isInitialized = false;
}

bool AudioContext::isInitialized() const
{
    return m_isInitialized;
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

    /// @TODO is a repeat of handlePreRenderTasks actually correct here?
    AudioSummingJunction::handleDirtyAudioSummingJunctions(r);
    updateAutomaticPullNodes();
}

void AudioContext::connect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, int destIdx, int srcIdx)
{
    if (!destination) throw std::runtime_error("Cannot connect to null destination");
    if (!source) throw std::runtime_error("Cannot connect from null source");
    std::lock_guard<std::mutex> lock(m_updateMutex);
    if (srcIdx > source->numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs");
    if (destIdx > destination->numberOfInputs()) throw std::out_of_range("Input index greater than available inputs");
    m_internal->pendingNodeConnections.emplace(destination, source, ConnectionType::Connect, destIdx, srcIdx);
    cv.notify_all();
}

void AudioContext::disconnect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, int destIdx, int srcIdx)
{
    if (!destination && !source)
        return;

    std::lock_guard<std::mutex> lock(m_updateMutex);
    if (source && srcIdx > source->numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs");
    if (destination && destIdx > destination->numberOfInputs()) throw std::out_of_range("Input index greater than available inputs");
    m_internal->pendingNodeConnections.emplace(destination, source, ConnectionType::Disconnect, destIdx, srcIdx);
    cv.notify_all();
}

void AudioContext::disconnect(std::shared_ptr<AudioNode> node, int index)
{
    if (!node)
        return;

    std::lock_guard<std::mutex> lock(m_updateMutex);
    m_internal->pendingNodeConnections.emplace(node, std::shared_ptr<AudioNode>(), ConnectionType::Disconnect, index, 0);
    cv.notify_all();
}

void AudioContext::connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index)
{
    if (!param)
        throw std::invalid_argument("No parameter specified");

    if (index >= driver->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs on the driver");

    m_internal->pendingParamConnections.push(std::make_tuple(param, driver, ConnectionType::Connect, index));
    cv.notify_all();
}

void AudioContext::disconnectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index)
{
    if (!param)
        throw std::invalid_argument("No parameter specified");

    if (index >= driver->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs on the driver");

    m_internal->pendingParamConnections.push(std::make_tuple(param, driver, ConnectionType::Disconnect, index));
    cv.notify_all();
}

void AudioContext::update()
{
    if (!m_isOfflineContext) { LOG("Begin UpdateGraphThread"); }

    const float frameLengthInMilliseconds = (sampleRate() / (float) AudioNode::ProcessingSizeInFrames) / 1000.f;  // = ~0.345ms @ 44.1k/128
    const float graphTickDurationMs = frameLengthInMilliseconds * 16;  // = ~5.5ms
    const uint32_t graphTickDurationUs = static_cast<uint32_t>(graphTickDurationMs * 1000.f);  // = ~5550us

    ASSERT(frameLengthInMilliseconds);
    ASSERT(graphTickDurationMs);
    ASSERT(graphTickDurationUs);

    // graphKeepAlive keeps the thread alive momentarily (letting tail tasks
    // finish) even updateThreadShouldRun has been signaled.
    while (!m_internal->pendingNodeConnections.empty() || !m_internal->pendingParamConnections.empty() ||
           updateThreadShouldRun != 0 || graphKeepAlive > 0)
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

            // A condition variable is used to notify this thread that a graph update is pendingin one of the queues.

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
            while (!m_internal->pendingParamConnections.empty())
            {
                auto connection = m_internal->pendingParamConnections.front();
                m_internal->pendingParamConnections.pop();
                if (std::get<2>(connection) == ConnectionType::Connect)
                    AudioParam::connect(gLock, std::get<0>(connection), std::get<1>(connection)->output(std::get<3>(connection)));
                else
                    AudioParam::disconnect(gLock, std::get<0>(connection), std::get<1>(connection)->output(std::get<3>(connection)));
            }

            std::vector<PendingConnection> skippedConnections;

            // Satisfy node connections
            while (!m_internal->pendingNodeConnections.empty())
            {
                auto connection = m_internal->pendingNodeConnections.top();
                m_internal->pendingNodeConnections.pop();

                switch (connection.type)
                {
                    case ConnectionType::Connect:
                    {
                        // requeue this node if the scheduled time is > 100ms away
                        if (connection.destination && connection.destination->isScheduledNode())
                        {
                            AudioScheduledSourceNode * node = dynamic_cast<AudioScheduledSourceNode *>(connection.destination.get());
                            if (node->startWhen() > now + 0.1)
                            {
                                m_internal->pendingNodeConnections.pop();  // pop from current queue
                                skippedConnections.push_back(connection);  // save for later
                                continue;
                            }
                        }

                        // if unscheduled it should start to play as soon as possible
                        if (!connection.source->isScheduledNode())
                        {
                            connection.source->_scheduler.start(0);
                        }

                        AudioNodeInput::connect(gLock, connection.destination->input(connection.destIndex), connection.source->output(connection.srcIndex));
                    }
                    break;

                    case ConnectionType::Disconnect:
                    {
                        connection.type = ConnectionType::FinishDisconnect;
                        skippedConnections.push_back(connection);  // save for later
                        if (connection.source)
                        {
                            // if source and destination are specified, then we don't ramp out the destination
                            // source all by itself will be completely disconnected
                            connection.source->scheduleDisconnect();
                        }
                        else if (connection.destination)
                        {
                            // destination all by itself will be completely disconnected
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
                            for (int out = 0; out < connection.destination->numberOfOutputs(); ++out)
                            {
                                auto output = connection.destination->output(out);
                                if (!output) continue;

                                AudioNodeOutput::disconnectAll(gLock, output);
                            }
                        }
                        else if (connection.source)
                        {
                            for (int out = 0; out < connection.source->numberOfOutputs(); ++out)
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
                m_internal->pendingNodeConnections.push(sc);
            }
        }

        if (lk.owns_lock())
            lk.unlock();
    }

    if (!m_isOfflineContext) { LOG("End UpdateGraphThread"); }
}

void AudioContext::addAutomaticPullNode(std::shared_ptr<AudioNode> node)
{
    std::lock_guard<std::mutex> lock(m_updateMutex);
    if (m_automaticPullNodes.find(node) == m_automaticPullNodes.end())
    {
        m_automaticPullNodes.insert(node);
        m_automaticPullNodesNeedUpdating = true;
        if (!node->isScheduledNode())
        {
            node->_scheduler.start(0);
        }
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

void AudioContext::processAutomaticPullNodes(ContextRenderLock & r, int framesToProcess)
{
    for (unsigned i = 0; i < m_renderingAutomaticPullNodes.size(); ++i)
    {
        m_renderingAutomaticPullNodes[i]->processIfNecessary(r, framesToProcess, 0, framesToProcess);
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

float AudioContext::sampleRate() const
{
    // during construction of DeviceNodes, the device_callback will not yet be ready.
    // sampleRate is called during AudioNode construction to initialize the scheduler, but DeviceNodes are not scheduled.
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

}  // End namespace lab
