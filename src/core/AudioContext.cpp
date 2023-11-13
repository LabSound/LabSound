// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioHardwareInputNode.h"
#include "LabSound/core/AudioListener.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/OscillatorNode.h"
#include "internal/HRTFDatabase.h"

#include "LabSound/extended/AudioContextLock.h"

//// for debugging only
#include "LabSound/extended/ADSRNode.h"
//// for debugging only


#include "internal/Assertions.h"

#include "concurrentqueue/concurrentqueue.h"
#include "libnyquist/Encoders.h"

#include <assert.h>
#include <queue>
#include <stdio.h>

namespace lab {

enum class ConnectionOperationKind : int
{
    Disconnect = 0,
    Connect,
    FinishDisconnect
};

struct PendingNodeConnection
{
    ConnectionOperationKind type;
    std::shared_ptr<AudioNode> destination;
    std::shared_ptr<AudioNode> source;
    int destIndex = 0;
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
    int destIndex = 0;

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

    std::shared_ptr<HRTFDatabaseLoader> hrtfDatabaseLoader;

    
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


AudioContext::AudioContext(bool isOffline)
    : m_isOfflineContext(isOffline)
{
    static std::atomic<int> id {1};
    m_internal.reset(new AudioContext::Internals(true));
    m_listener.reset(new AudioListener());
    m_audioContextInterface = std::make_shared<AudioContextInterface>(this, id);
    ++id;

    if (isOffline)
    {
        updateThreadShouldRun = 1;
        graphKeepAlive = 0;
    }
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

bool AudioContext::isAutodispatchingEvents() const
{
    return m_internal->autoDispatchEvents;
}


AudioContext::~AudioContext()
{
    LOG_TRACE("Begin AudioContext::~AudioContext()");

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

    m_audioContextInterface.reset();

    ASSERT(!_contextIsInitialized);
    ASSERT(!m_automaticPullNodes.size());
    ASSERT(!m_renderingAutomaticPullNodes.size());

    LOG_INFO("Finish AudioContext::~AudioContext()");
}

bool AudioContext::loadHrtfDatabase(const std::string & searchPath)
{
    //auto db = new HRTFDatabaseLoader(sampleRate(), searchPath);
    //m_internal->hrtfDatabaseLoader.reset(db);
    //db->loadAsynchronously();
    //db->waitForLoaderThreadCompletion();
    //return db->database()->numberOfElevations() > 0 && db->database()->numberOfAzimuths() > 0;
    bool loaded = true;
    auto db = HRTFDatabaseLoader::MakeHRTFLoaderSingleton(sampleRate(), searchPath);
    // auto db = new HRTFDatabaseLoader(sampleRate(), searchPath);
    if (!db->database() || !db->database()->files_found_and_loaded())
    {
        m_internal->hrtfDatabaseLoader.reset(db);
        db->loadAsynchronously();
        db->waitForLoaderThreadCompletion();
        printf("db files found and loaded %d", db->database()->files_found_and_loaded() ? 1 : 0);
        printf("num elevs %d num az %d\n", db->database()->numberOfElevations(), db->database()->numberOfAzimuths());
        loaded = db->database()->files_found_and_loaded();
        loaded = loaded && db->database()->numberOfElevations() > 0 && db->database()->numberOfAzimuths() > 0;
    }
    return loaded;
}

std::shared_ptr<HRTFDatabaseLoader> AudioContext::hrtfDatabaseLoader() const {
    return m_internal->hrtfDatabaseLoader;
}

void AudioContext::lazyInitialize()
{
    if (_contextIsInitialized)
        return;

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    ASSERT(!m_isAudioThreadFinished);
    if (m_isAudioThreadFinished)
        return;

    if (auto d = _destinationNode.get())
    {
        if (!isOfflineContext())
        {
            // start the device
            d->device()->start();

            // start the audio thread and all audio rendering.
            // The destination node's provideInput() method will now be called repeatedly to render audio.
            // Each time provideInput() is called, a portion of the audio stream is rendered.

            graphKeepAlive = 0.25f;  // pump the graph for the first 0.25 seconds
            graphUpdateThread = std::thread(&AudioContext::update, this);
        }

        _contextIsInitialized = 1;
        cv.notify_all();
    }
    else
    {
        LOG_ERROR("m_device not specified");
        ASSERT(_destinationNode);
    }
}

void AudioContext::uninitialize()
{
    LOG_TRACE("AudioContext::uninitialize()");

    if (!_contextIsInitialized)
        return;

    // for the case where an Offline AudioDestinationNode needs to update the graph
    // before shutting done
    updateAutomaticPullNodes();

    // This stops the audio thread and all audio rendering.
    if (_destinationNode && _destinationNode->device())
        _destinationNode->device()->stop();

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    m_isAudioThreadFinished = true;

    updateAutomaticPullNodes();  // added for the case where an NullDeviceNode needs to update the graph

    _contextIsInitialized = 0;
}

bool AudioContext::isInitialized() const
{
    return _contextIsInitialized != 0;
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
        // take a graph lock until the queues are cleared
        ContextGraphLock gLock(this, "AudioContext::handlePreRenderTasks()");

        // resolve parameter connections
        PendingParamConnection param_connection;
        while (m_internal->pendingParamConnections.try_dequeue(param_connection))
        {
            if (param_connection.type == ConnectionOperationKind::Connect)
            {
                AudioParam::connect(gLock,
                                    param_connection.destination,
                                    param_connection.source->output(param_connection.destIndex));

                // if unscheduled, the source should start to play as soon as possible
                if (!param_connection.source->isScheduledNode())
                    param_connection.source->_self->_scheduler.start(0);
            }
            else
                AudioParam::disconnect(gLock,
                                       param_connection.destination,
                                       param_connection.source->output(param_connection.destIndex));
        }

        // resolve node connections
        PendingNodeConnection node_connection;
        std::vector<PendingNodeConnection> requeued_connections;
        while (m_internal->pendingNodeConnections.try_dequeue(node_connection))
        {
            switch (node_connection.type)
            {
                case ConnectionOperationKind::Connect:
                {
                    AudioNodeInput::connect(gLock,
                                            node_connection.destination->input(node_connection.destIndex),
                                            node_connection.source->output(node_connection.srcIndex));

                    if (!node_connection.source->isScheduledNode())
                        node_connection.source->_self->_scheduler.start(0);
                }
                break;

                case ConnectionOperationKind::Disconnect:
                {
                    node_connection.type = ConnectionOperationKind::FinishDisconnect;
                    requeued_connections.push_back(node_connection);  // save for later
                    if (node_connection.source)
                    {
                        // if source and destination are specified, then don't ramp out the destination
                        // source will be completely disconnected
                        node_connection.source->scheduleDisconnect();
                    }
                    else if (node_connection.destination)
                    {
                        // destination will be completely disconnected
                        node_connection.destination->scheduleDisconnect();
                    }
                }
                break;

                case ConnectionOperationKind::FinishDisconnect:
                {
                    if (node_connection.duration > 0)
                    {
                        node_connection.duration -= AudioNode::ProcessingSizeInFrames / sampleRate();
                        requeued_connections.push_back(node_connection);
                        continue;
                    }

                    if (node_connection.source && node_connection.destination)
                    {
                        //if (!node_connection.destination->disconnectionReady() || !node_connection.source->disconnectionReady())
                        //    requeued_connections.push_back(node_connection);
                        //else
                            AudioNodeInput::disconnect(gLock, node_connection.destination->input(node_connection.destIndex), node_connection.source->output(node_connection.srcIndex));
                    }
                    else if (node_connection.destination)
                    {
                        //if (!node_connection.destination->disconnectionReady())
                        //    requeued_connections.push_back(node_connection);
                        //else
                            for (int in = 0; in < node_connection.destination->numberOfInputs(); ++in)
                            {
                                auto input= node_connection.destination->input(in);
                                if (input)
                                    AudioNodeInput::disconnectAll(gLock, input);
                            }
                    }
                    else if (node_connection.source)
                    {
                        //if (!node_connection.destination->disconnectionReady())
                        //    requeued_connections.push_back(node_connection);
                        //else
                            for (int out = 0; out < node_connection.source->numberOfOutputs(); ++out)
                            {
                                auto output = node_connection.source->output(out);
                                if (output)
                                    AudioNodeOutput::disconnectAll(gLock, output);
                            }
                    }
                }
                break;
            }
        }

        // We have incompletely connected nodes, so next time the thread ticks we can re-check them
        for (auto & sc : requeued_connections)
            m_internal->pendingNodeConnections.enqueue(sc);
    }

    AudioSummingJunction::handleDirtyAudioSummingJunctions(r);
    updateAutomaticPullNodes();
}

void AudioContext::handlePostRenderTasks(ContextRenderLock & r)
{
    ASSERT(r.context());
    AudioSummingJunction::handleDirtyAudioSummingJunctions(r);
    updateAutomaticPullNodes();
}

void AudioContext::synchronizeConnections(int timeOut_ms)
{
    cv.notify_all();
    if (!_destinationNode || !_destinationNode->device())
        return;

    // don't synch a suspended context as that will simply max out the timeout
    if (!_destinationNode->device()->isRunning())
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
    m_internal->pendingNodeConnections.enqueue({ConnectionOperationKind::Connect, destination, source, destIdx, srcIdx});
}

void AudioContext::disconnect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, int destIdx, int srcIdx)
{
    if (!destination && !source)
        return;
    if (source && srcIdx > source->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs");
    if (destination && destIdx > destination->numberOfInputs())
        throw std::out_of_range("Input index greater than available inputs");
    m_internal->pendingNodeConnections.enqueue({ConnectionOperationKind::Disconnect, destination, source, destIdx, srcIdx});
}

void AudioContext::disconnect(std::shared_ptr<AudioNode> node, int index)
{
    if (!node)
        return;
    m_internal->pendingNodeConnections.enqueue({ConnectionOperationKind::Disconnect, node, std::shared_ptr<AudioNode>(), index, 0});
}

bool AudioContext::isConnected(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source)
{
    if (!destination || !source)
        return false;

    AudioNode* n = source.get();
    for (int i = 0; i < destination->numberOfInputs(); ++i)
    {
        auto c = destination->input(i);
        if (c->destinationNode() == n)
            return true;
    }

    return false;
}


void AudioContext::connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index)
{
    if (!param)
        throw std::invalid_argument("No parameter specified");
    if (!driver)
        throw std::invalid_argument("No driving node supplied");
    if (index >= driver->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs on the driver");
    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::Connect, param, driver, index});
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

    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::Connect, param, driver, index});
}


void AudioContext::disconnectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index)
{
    if (!param)
        throw std::invalid_argument("No parameter specified");

    if (index >= driver->numberOfOutputs())
        throw std::out_of_range("Output index greater than available outputs on the driver");

    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::Disconnect, param, driver, index});
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
            if (delta <= 0.f) // no need to keep running if the graph is no longer updating
                break;
            lastGraphUpdateTime = static_cast<float>(now);
            graphKeepAlive -= delta;
        }

        if (lk.owns_lock())
            lk.unlock();

        if (!updateThreadShouldRun)
            break;
    }

    if (!m_isOfflineContext)
    {
        LOG_TRACE("End UpdateGraphThread");
    }
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
            node->_self->_scheduler.start(0);
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
        m_renderingAutomaticPullNodes[i]->processIfNecessary(r, framesToProcess);
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

void AudioContext::setDestinationNode(std::shared_ptr<AudioDestinationNode> device)
{
    _destinationNode = device;
    lazyInitialize();
}

std::shared_ptr<AudioDestinationNode> AudioContext::destinationNode()
{
    return _destinationNode;
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
    auto dn = _destinationNode;
    if (dn)
        return dn->getSamplingInfo().current_time;
    return 0.f;
}

uint64_t AudioContext::currentSampleFrame() const
{
    return _destinationNode->getSamplingInfo().current_sample_frame;
}

double AudioContext::predictedCurrentTime() const
{
    auto info = _destinationNode->getSamplingInfo();
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
    if (!_destinationNode)
        return 0.f;

    return _destinationNode->getSamplingInfo().sampling_rate;
}

void AudioContext::startOfflineRendering()
{
    if (!m_isOfflineContext)
        throw std::runtime_error("context was not constructed for offline rendering");
    
    if (!_destinationNode && !_destinationNode->device())
        return;
    
    _contextIsInitialized = 1;
    _destinationNode->device()->start();
}

void AudioContext::suspend()
{
    if (!_destinationNode && !_destinationNode->device())
        return;

    _destinationNode->device()->stop();
}

// if the context was suspended, resume the progression of time and processing in the audio context
void AudioContext::resume()
{
    if (!_destinationNode && !_destinationNode->device())
        return;

    _destinationNode->device()->start();
}

void AudioContext::close()
{
    suspend();
    _destinationNode.reset();
}

void AudioContext::debugTraverse(AudioNode * root)
{
    ASSERT(root);
    // bail if shutting down.
    auto ac = audioContextInterface().lock();
    if (!ac)
        return;

    auto ctx = this;

    ContextRenderLock renderLock(this, "lab::pull_graph");
    if (!renderLock.context())
        return;  // return if couldn't acquire lock

    if (!ctx->isInitialized())
    {
        return;
    }

    // make sure any pending connections are made.
    synchronizeConnections();

    // Let the context take care of any business at the start of each render quantum.
    ctx->handlePreRenderTasks(renderLock);

    /* pass two, implement hardware input
    // Prepare the local audio input provider for this render quantum.
    if (optional_hardware_input && src)
    {
        optional_hardware_input->set(src);
    }
     */

    // process the graph by pulling the inputs, which will recurse the
    // entire processing graph.

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
    static int color = 1;
    root->_self->color = color;
    ++color;

    // start the traversal at the root node
    node_stack.push_back(root);

    static std::map<int, std::vector<AudioBus *>> summing_busses;
    std::vector<AudioBus *> in_use;
    std::vector<std::pair<AudioNode *, const char*>> by_whom;

    AudioNode * current_node = nullptr;
    while (!node_stack.empty())
    {
        current_node = node_stack.back();

        auto sz = node_stack.size();
        for (auto & p : current_node->_self->_params)
        {
            // if there are rendering connections, be sure they are ready
            p->updateRenderingState(renderLock);

            int connectionCount = p->numberOfRenderingConnections(renderLock);
            if (!connectionCount)
                continue;

            for (int i = 0; i < connectionCount; ++i)
            {
                std::shared_ptr<AudioNodeOutput> output = p->renderingOutput(renderLock, i);
                if (!output)
                    continue;
                
                AudioNode* node = output->sourceNode();
                if (!node || node->_self->color >= color)
                    continue;

                by_whom.push_back({current_node, "param input"});
                node_stack.push_back(node);
            }
        }

        // if there were parameters with inputs, queue up their work next
        if (node_stack.size() > sz)
            continue;

        // Process all of the AudioNodes connected to our inputs.
        sz = node_stack.size();
        int inputCount = current_node->numberOfInputs();
        for (int i = 0; i < inputCount; ++i)
        {
            auto in = current_node->input(i);
            if (!in)
                continue;

            int connectionCount = in->numberOfConnections();
            for (int j = 0; j < connectionCount; ++j) {
                if (auto destNode = in->connection(renderLock, i)) {
                    if (destNode->sourceNode()->_self->color < color)
                    {
                        node_stack.push_back(destNode->sourceNode());
                    }
                }
            }
        }

        // if there were inputs on the node current node, queue up their work next
        if (node_stack.size() > sz)
            continue;

        // move current_node to the work stack and retire it from the node stack
        current_node->_self->color = color;
        work.push_back({current_node, processNode});
        node_stack.pop_back();
    }


    for (auto& w : work) {
        // actually do the things in graph order
        if (w.type == processNode) {
            if (auto adsr = dynamic_cast<ADSRNode*>(w.node)) {
                printf("%s %s %s Inputs silent: %s\n",
                       w.node->name(),
                       schedulingStateName(w.node->_self->_scheduler.playbackState()),
                       adsr->finished(renderLock)? "Finished": "Playing",
                       w.node->inputsAreSilent(renderLock) ? "yes" : "no");
            }
            else {
                printf("%s %s Inputs silent: %s\n",
                       w.node->name(),
                       schedulingStateName(w.node->_self->_scheduler.playbackState()),
                       w.node->inputsAreSilent(renderLock) ? "yes" : "no");
            }
        }
    }
}

void AudioContext::diagnose(std::shared_ptr<AudioNode> n)
{
    _diagnose = n;
}

void AudioContext::diagnosed_silence(const char* msg)
{
    if (_diagnose)
    {
        printf("*** ls::diagnosed silence: %s:%s\n", _diagnose->name(), msg);
    }
}

} // end namespace lab
