// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioHardwareInputNode.h"
#include "LabSound/core/AudioListener.h"
#include "LabSound/core/OscillatorNode.h"
#include "internal/HRTFDatabase.h"

#include "LabSound/extended/AudioContextLock.h"

//// for debugging only
#include "LabSound/extended/ADSRNode.h"
//// for debugging only


#include "internal/Assertions.h"

#include "concurrentqueue/concurrentqueue.h"

#include <assert.h>
#include <map>
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

    void flushDebugBuffer(char const* const rawFilePath)
    {
        if (!debugBufferIndex || !rawFilePath) {
            debugBufferIndex = 0;
            return;
        }

        FILE* f = fopen(rawFilePath, "wb");
        if (!f) {
            debugBufferIndex = 0;
            return;
        }
        fwrite(debugBuffer.data(), sizeof(float) * debugBufferIndex, 1, f);
        fclose(f);
        debugBufferIndex = 0;
    }
};

void AudioContext::appendDebugBuffer(AudioBus* bus, int channel, int count)
{
    m_internal->appendDebugBuffer(bus, channel, count);
}

void AudioContext::flushDebugBuffer(char const* const rawFilePath)
{
    m_internal->flushDebugBuffer(rawFilePath);
}


AudioContext::AudioContext(std::shared_ptr<AudioDevice> device, bool isOffline)
    : m_isOfflineContext(isOffline)
{
    static std::atomic<int> id {1};
    m_internal.reset(new AudioContext::Internals(true));
    m_listener.reset(new AudioListener());
    m_audioContextInterface = std::make_shared<AudioContextInterface>(device, this, id);
    ++id;

    if (isOffline)
    {
        updateThreadShouldRun = 1;
        graphKeepAlive = 0;
    }
}

AudioContext::AudioContext(std::shared_ptr<AudioDevice> device, bool isOffline, bool autoDispatchEvents)
    : m_isOfflineContext(isOffline)
{
    static std::atomic<int> id {1};
    m_internal.reset(new AudioContext::Internals(autoDispatchEvents));
    m_listener.reset(new AudioListener());
    m_audioContextInterface = std::make_shared<AudioContextInterface>(device, this, id);
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

    ASSERT(!m_isInitialized);
    ASSERT(!m_automaticPullNodes.size());
    ASSERT(!m_renderingAutomaticPullNodes.size());

    LOG_INFO("Finish AudioContext::~AudioContext()");
}

bool AudioContext::loadHrtfDatabase(const std::string & searchPath)
{
    auto db = new HRTFDatabaseLoader(sampleRate(), searchPath);
    m_internal->hrtfDatabaseLoader.reset(db);
    db->loadAsynchronously();
    db->waitForLoaderThreadCompletion();
    return db->database()->numberOfElevations() > 0 && db->database()->numberOfAzimuths() > 0;
}

std::shared_ptr<HRTFDatabaseLoader> AudioContext::hrtfDatabaseLoader() const {
    return m_internal->hrtfDatabaseLoader;
}

void AudioContext::lazyInitialize()
{
    if (m_isInitialized)
        return;

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    ASSERT(!m_isAudioThreadFinished);
    if (m_isAudioThreadFinished)
        return;

    if (auto d = m_audioContextInterface->_destinationNode.get())
    {
        if (!isOfflineContext())
        {
            // This starts the audio thread and all audio rendering.
            // The destination node's provideInput() method will now be called repeatedly to render audio.
            // Each time provideInput() is called, a portion of the audio stream is rendered.

            graphKeepAlive = 0.25f;  // pump the graph for the first 0.25 seconds
            graphUpdateThread = std::thread(&AudioContext::update, this);
            d->device()->start();
        }

        cv.notify_all();
        m_isInitialized = true;
    }
    else
    {
        LOG_ERROR("m_device not specified");
        ASSERT(m_audioContextInterface->_destinationNode);
    }
}

void AudioContext::backendReinitialize()
{
    this->m_audioContextInterface->_device->backendReinitialize();
    this->m_audioContextInterface->resetSamplingInfo();
}

void AudioContext::uninitialize()
{
    LOG_TRACE("AudioContext::uninitialize()");

    if (!m_isInitialized)
        return;

    // for the case where an Offline AudioDestinationNode needs to update the graph
    // before shutting done
    updateAutomaticPullNodes();

    // This stops the audio thread and all audio rendering.
    if (m_audioContextInterface->_destinationNode && m_audioContextInterface->_destinationNode->device())
        m_audioContextInterface->_destinationNode->device()->stop();

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
    // these may be null during application exit.
    if (!r.context() || !m_audioContextInterface)
        return;

    // At the beginning of every render quantum, update the graph.

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
                                    param_connection.source,
                                    param_connection.destIndex);

                // if unscheduled, the source should start to play as soon as possible
                if (!param_connection.source->isScheduledNode())
                    param_connection.source->_self->scheduler.start(0);
            }
            else
                AudioParam::disconnect(gLock,
                                       param_connection.destination,
                                       param_connection.source);
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
                    // render lock is held, so this is safe
                    AudioNode::connect(node_connection.destination,
                                       node_connection.source,
                                       node_connection.srcIndex, node_connection.destIndex);

                    if (!node_connection.source->isScheduledNode())
                        node_connection.source->_self->scheduler.start(0);
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
                        node_connection.destination->disconnect(node_connection.source,
                                                                node_connection.srcIndex, node_connection.destIndex);
                    }
                    else if (node_connection.destination)
                    {
                        node_connection.destination->disconnect({}, -1, -1);
                    }
                    else if (node_connection.source)
                    {
                        // iterate all nodes an disconnect this from any it feeds into
                        auto output = node_connection.source->output();
                        if (output) {
                            node_connection.destination->disconnectReceivers();
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

    updateAutomaticPullNodes();
}

void AudioContext::handlePostRenderTasks(ContextRenderLock & r)
{
    ASSERT(r.context());
    updateAutomaticPullNodes();
}

void AudioContext::synchronizeConnections(int timeOut_ms)
{
    cv.notify_all();
    if (!m_audioContextInterface->_destinationNode || !m_audioContextInterface->_destinationNode->device())
        return;

    // don't synch a suspended context as that will simply max out the timeout
    if (!m_audioContextInterface->_destinationNode->device()->isRunning())
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
    m_internal->pendingNodeConnections.enqueue({ConnectionOperationKind::Connect, destination, source, destIdx, srcIdx});
}

void AudioContext::disconnect(std::shared_ptr<AudioNode> destination, std::shared_ptr<AudioNode> source, int destIdx, int srcIdx)
{
    if (!destination && !source)
        return;
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
    
    return destination->isConnected(source);
}


void AudioContext::connectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index)
{
    if (!param)
        throw std::invalid_argument("No parameter specified");
    if (!driver)
        throw std::invalid_argument("No driving node supplied");
    if (index >= driver->output()->numberOfChannels())
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

    m_internal->pendingParamConnections.enqueue({ConnectionOperationKind::Connect, param, driver, index});
}


void AudioContext::disconnectParam(std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> driver, int index)
{
    if (param && driver)
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
            node->_self->scheduler.start(0);
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
    m_audioContextInterface->_destinationNode = device;
    lazyInitialize();
}

std::shared_ptr<AudioDestinationNode> AudioContext::destinationNode()
{
    return m_audioContextInterface->_destinationNode;
}

bool AudioContext::isOfflineContext() const
{
    return m_isOfflineContext;
}

std::shared_ptr<AudioListener> AudioContext::listener()
{
    return m_listener;
}

AudioContext::AudioContextInterface::AudioContextInterface(
    std::shared_ptr<AudioDevice> device, AudioContext * ac, int id)
: _id(id)
, _ac(ac)
, _device(device)
{
    _last_info.sampling_rate = device->getOutputConfig().desired_samplerate;
    resetSamplingInfo();
}

void AudioContext::AudioContextInterface::resetSamplingInfo()
{
    _last_info.current_sample_frame = 0;
    _last_info.current_time = 0.f;
    _last_info.epoch[0] = std::chrono::high_resolution_clock::time_point();
    _last_info.epoch[1] = std::chrono::high_resolution_clock::time_point();
}


double AudioContext::AudioContextInterface::currentTime() const
{
    return _last_info.current_time;
}

uint64_t AudioContext::AudioContextInterface::currentSampleFrame() const
{
    return _last_info.current_sample_frame & ~1;
}

double AudioContext::AudioContextInterface::predictedCurrentTime() const
{
    uint64_t t = _last_info.current_sample_frame;
    double val = t / _last_info.sampling_rate;
    auto t2 = std::chrono::high_resolution_clock::now();
    int index = t & 1;

    if (!_last_info.epoch[index].time_since_epoch().count())
        return val;

    std::chrono::duration<double> elapsed =
        std::chrono::high_resolution_clock::now() - _last_info.epoch[index];
    return val + elapsed.count();
}

float AudioContext::AudioContextInterface::sampleRate() const
{
    // sampleRate is called during AudioNode construction to initialize the
    // scheduler, but DeviceNodes are not scheduled.
    // during construction of DeviceNodes, the device_callback will not yet be
    // ready, so bail out.
    if (!_destinationNode)
        return 0.f;

    return _last_info.sampling_rate;
}

void AudioContext::AudioContextInterface::updateSamplingInfo() {
    auto samplingInfo = _last_info;
    const int32_t index = 1 - (samplingInfo.current_sample_frame & 1);
    const uint64_t t = samplingInfo.current_sample_frame & ~1;
    samplingInfo.current_sample_frame = t + index + AudioNode::ProcessingSizeInFrames;
    samplingInfo.current_time = samplingInfo.current_sample_frame / static_cast<double>(samplingInfo.sampling_rate);
    samplingInfo.epoch[index] = std::chrono::high_resolution_clock::now();
    _last_info = samplingInfo;
}

void AudioContext::startOfflineRendering()
{
    if (!m_isOfflineContext)
        throw std::runtime_error("context was not constructed for offline rendering");

    m_isInitialized = true;
    
    if (!m_audioContextInterface->_destinationNode && !m_audioContextInterface->_destinationNode->device())
        return;
    
    m_audioContextInterface->_destinationNode->device()->start();
}

void AudioContext::suspend()
{
    if (!m_audioContextInterface->_destinationNode && !m_audioContextInterface->_destinationNode->device())
        return;

    m_audioContextInterface->_destinationNode->device()->stop();
}

// if the context was suspended, resume the progression of time and processing in the audio context
void AudioContext::resume()
{
    if (!m_audioContextInterface->_destinationNode && !m_audioContextInterface->_destinationNode->device())
        return;

    m_audioContextInterface->_destinationNode->device()->start();
}

void AudioContext::close()
{
    suspend();
    m_audioContextInterface->_destinationNode.reset();
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
        for (auto & p : current_node->_self->params)
        {
#if 0
            /// @TODO rewrite once rendering is back on line
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
#endif
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

#if 0
            /// @TODO rewrite once rendering is back on line
            int connectionCount = in->numberOfConnections();
            for (int j = 0; j < connectionCount; ++j) {
                if (auto destNode = in->connection(renderLock, i)) {
                    if (destNode->sourceNode()->_self->color < color)
                    {
                        node_stack.push_back(destNode->sourceNode());
                    }
                }
            }
#endif

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
/*            if (auto adsr = dynamic_cast<ADSRNode*>(w.node)) {
                printf("%s %s %s Inputs silent: %s\n",
                       w.node->name(),
                       schedulingStateName(w.node->_self->scheduler.playbackState()),
                       adsr->finished(renderLock)? "Finished": "Playing",
                       w.node->inputsAreSilent(renderLock) ? "yes" : "no");
            }
            else {*/
                printf("%s %s Inputs silent: %s\n",
                       w.node->name(),
                       schedulingStateName(w.node->_self->scheduler.playbackState()),
                       w.node->inputsAreSilent(renderLock) ? "yes" : "no");
//            }
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
