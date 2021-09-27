// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"
#include "concurrentqueue/concurrentqueue.h"

using namespace std;

//#define ASN_PRINT(...)
#define ASN_PRINT(a) printf(a)


/*


AudioNode
    [Params]
    [Settings]
    [Inputs]
    [(name, AudioBus)]

Inputs
    [Outputs]


*/





namespace lab
{

const char * schedulingStateName(SchedulingState s)
{
    switch (s)
    {
        case SchedulingState::UNSCHEDULED:
            return "UNSCHEDULED";
        case SchedulingState::SCHEDULED:
            return "SCHEDULED";
        case SchedulingState::FADE_IN:
            return "FADE_IN";
        case SchedulingState::PLAYING:
            return "PLAYING";
        case SchedulingState::STOPPING:
            return "STOPPING";
        case SchedulingState::RESETTING:
            return "RESETTING";
        case SchedulingState::FINISHING:
            return "FINISHING";
        case SchedulingState::FINISHED:
            return "FINISHED";
    }
    return "Unknown";
}

AudioNode::Input::Input() noexcept
{
    summingBus = nullptr;
}
AudioNode::Input::~Input()
{
    if (summingBus)
        delete summingBus;
}

AudioNode::Output::~Output()
{
    if (bus)
        delete bus;
}

const int start_envelope = 64;
const int end_envelope = 64;

AudioNodeScheduler::AudioNodeScheduler(float sampleRate)
    : _epoch(0)
    , _startWhen(std::numeric_limits<uint64_t>::max())
    , _stopWhen(std::numeric_limits<uint64_t>::max())
    , _sampleRate(sampleRate)
{
}

bool AudioNodeScheduler::isCurrentEpoch(ContextRenderLock & r) const
{
    return _epoch >= r.context()->currentSampleFrame();
}


bool AudioNodeScheduler::update(ContextRenderLock & r, int epoch_length)
{
    uint64_t proposed_epoch = r.context()->currentSampleFrame();
    if (_epoch >= proposed_epoch)
        return false;

    _epoch = proposed_epoch;
    _renderOffset = 0;
    _renderLength = epoch_length;

    switch (_playbackState)
    {
        case SchedulingState::UNSCHEDULED:
            break;

        case SchedulingState::SCHEDULED:
            // start has been called, start looking for the start time
            if (_startWhen <= _epoch)
            {
                // exactly on start, or late, get going straight away
                _renderOffset = 0;
                _renderLength = epoch_length;
                _playbackState = SchedulingState::FADE_IN;
                ASN_PRINT("fade in\n");
            }
            else if (_startWhen < _epoch + epoch_length)
            {
                // start falls within the frame
                _renderOffset = static_cast<int>(_startWhen - _epoch);
                _renderLength = epoch_length - _renderOffset;
                _playbackState = SchedulingState::FADE_IN;
                ASN_PRINT("fade in\n");
            }

            /// @TODO the case of a start and stop within one epoch needs to be special
            /// cased, to fit this current architecture, there'd be a FADE_IN_OUT scheduling
            /// state so that the envelope can be correctly applied.
            // FADE_IN_OUT would transition to UNSCHEDULED.
            break;

        case SchedulingState::FADE_IN:
            // start time has been achieved, there'll be one quantum with fade in applied.
            _renderOffset = 0;
            _playbackState = SchedulingState::PLAYING;
            ASN_PRINT("playing\n");
            // fall through to PLAYING to allow render length to be adjusted if stop-start is less than one quantum length

        case SchedulingState::PLAYING:
            /// @TODO include the end envelope in the stop check so that a scheduled stop that
            /// spans this quantum and the next ends in the current quantum

            if (_stopWhen <= _epoch)
            {
                // exactly on start, or late, stop straight away, render a whole frame of fade out
                _renderLength = epoch_length - _renderOffset;
                _playbackState = SchedulingState::STOPPING;
                ASN_PRINT("stopping\n");
            }
            else if (_stopWhen < _epoch + epoch_length)
            {
                // stop falls within the frame
                _renderOffset = 0;
                _renderLength = static_cast<int>(_stopWhen - _epoch);
                _playbackState = SchedulingState::STOPPING;
                ASN_PRINT("stopping\n");
            }

            // do not fall through to STOPPING because one quantum must render the fade out effect
            break;

        case SchedulingState::STOPPING:
            if (_epoch + epoch_length >= _stopWhen)
            {
                // scheduled stop has occured, so make sure stop doesn't immediately trigger again
                _stopWhen = std::numeric_limits<uint64_t>::max();
                _playbackState = SchedulingState::UNSCHEDULED;
                if (_onEnded)
                    r.context()->enqueueEvent(_onEnded);
                ASN_PRINT("unscheduled\n");
            }
            break;

        case SchedulingState::RESETTING:
            break;
        case SchedulingState::FINISHING:
            break;
        case SchedulingState::FINISHED:
            break;
    }

    ASSERT(_renderOffset < epoch_length);
    return true;
}

void AudioNodeScheduler::start(double when)
{
    // irrespective of start state, onStart will be called if possible
    // to allow a node to use subsequent starts as a hint
    if (_onStart)
        _onStart(when);

    // if already scheduled or playing, nothing to do
    if (_playbackState == SchedulingState::SCHEDULED || _playbackState == SchedulingState::PLAYING)
        return;

    // start cancels stop
    _stopWhen = std::numeric_limits<uint64_t>::max();

    // treat non finite, or max values as a cancellation of stopping or resetting
    if (!std::isfinite(when) || when == std::numeric_limits<double>::max())
    {
        if (_playbackState == SchedulingState::STOPPING || _playbackState == SchedulingState::RESETTING)
        {
            _playbackState = SchedulingState::PLAYING;
        }
        return;
    }

    _startWhen = _epoch + static_cast<uint64_t>(when * _sampleRate);
    _playbackState = SchedulingState::SCHEDULED;
}

void AudioNodeScheduler::stop(double when)
{
    // if the node is on the way to a terminal state do nothing
    if (_playbackState >= SchedulingState::STOPPING)
        return;

    // treat non-finite, and FLT_MAX as stop cancellation, if already playing
    if (!std::isfinite(when) || when == std::numeric_limits<double>::max())
    {
        // stop at a non-finite time means don't stop.
        _stopWhen = std::numeric_limits<uint64_t>::max();  // cancel stop
        if (_playbackState == SchedulingState::STOPPING)
        {
            // if in the process of stopping, set it back to scheduling to start immediately
            _playbackState = SchedulingState::SCHEDULED;
            _startWhen = 0;
        }

        return;
    }

    // clamp timing to now or the future
    if (when < 0)
        when = 0;

    // let the scheduler know when to activate the STOPPING state
    _stopWhen = _epoch + static_cast<uint64_t>(when * _sampleRate);
}

void AudioNodeScheduler::reset()
{
    _startWhen = std::numeric_limits<uint64_t>::max();
    _stopWhen = std::numeric_limits<uint64_t>::max();
    if (_playbackState != SchedulingState::UNSCHEDULED)
        _playbackState = SchedulingState::RESETTING;
}

void AudioNodeScheduler::finish(ContextRenderLock & r)
{
    if (_playbackState < SchedulingState::PLAYING)
        _playbackState = SchedulingState::FINISHING;
    else if (_playbackState >= SchedulingState::PLAYING && _playbackState < SchedulingState::FINISHED)
        _playbackState = SchedulingState::FINISHING;

    r.context()->enqueueEvent(_onEnded);
}

AudioParamDescriptor const * const AudioNodeDescriptor::param(char const * const p) const
{
    if (!params)
        return nullptr;

    AudioParamDescriptor const * i = params;
    while (i->name)
    {
        if (!strcmp(p, i->name))
            return i;

        ++i;
    }

    return nullptr;
}

AudioSettingDescriptor const * const AudioNodeDescriptor::setting(char const * const s) const
{
    if (!settings)
        return nullptr;

    AudioSettingDescriptor const * i = settings;
    while (i->name)
    {
        if (!strcmp(s, i->name))
            return i;

        ++i;
    }

    return nullptr;
}

AudioNode::AudioNode(AudioContext & ac, AudioNodeDescriptor const & desc)
    : _scheduler(ac.sampleRate())
    , _enqueudWork((ConcurrentQueue*) new moodycamel::ConcurrentQueue<Work>())
{
    if (desc.params)
    {
        AudioParamDescriptor const * i = desc.params;
        while (i->name)
        {
            _params.push_back(std::make_shared<AudioParam>(i));
            ++i;
        }
    }
    if (desc.settings)
    {
        AudioSettingDescriptor const * i = desc.settings;
        while (i->name)
        {
            _settings.push_back(std::make_shared<AudioSetting>(i));
            ++i;
        }
    }
}

AudioNode::~AudioNode()
{
    uninitialize();
    delete (moodycamel::ConcurrentQueue<Work> *) _enqueudWork;
}

void AudioNode::initialize()
{
    m_isInitialized = true;

    // service the queue once so that all inputs and outputs are registered
    // initialize() is only called from constructors, so the node is not in a graph.
    // there is no data race possible.
    ContextRenderLock r(nullptr, "initialize");
    serviceQueue(r);
}

void AudioNode::uninitialize()
{
    m_isInitialized = false;
}

void AudioNode::reset(ContextRenderLock & r)
{
    _scheduler.reset();
}

void AudioNode::addInput(const std::string & name)
{
    ((moodycamel::ConcurrentQueue<Work> *) _enqueudWork)->enqueue(Work {Work::OpAddInput, {}, 0, 0, name});
}

void AudioNode::addOutput(const std::string & name, int channelCount, int size)
{
    ((moodycamel::ConcurrentQueue<Work> *) _enqueudWork)->enqueue(Work {Work::OpAddOutput, {}, 0, 0, name, channelCount, size});
}

void AudioNode::connect(int input_index, std::shared_ptr<AudioNode> n, int node_outputIndex)
{
    ((moodycamel::ConcurrentQueue<Work> *) _enqueudWork)->enqueue(Work {Work::OpConnectInput, n, node_outputIndex, input_index});
}

void AudioNode::connect(ContextRenderLock & r, int input_index, std::shared_ptr<AudioNode> n, int node_outputIndex)
{
    while (_inputs.size() <= input_index)
        _inputs.emplace_back(Input {});
    _inputs[input_index].sources.push_back(Input::Source {n, node_outputIndex});
    if (_inputs[input_index].sources.size() > 1 && !_inputs[input_index].summingBus)
    {
        _inputs[input_index].summingBus = new AudioBus(n->outputBus(r, 0)->numberOfChannels(), AudioNode::ProcessingSizeInFrames);
    }
}

void AudioNode::disconnect(int inputIndex)
{
    ((moodycamel::ConcurrentQueue<Work> *) _enqueudWork)->enqueue(Work {Work::OpDisconnectIndex, {}, 0, inputIndex});
}

void AudioNode::disconnect(std::shared_ptr<AudioNode> n)
{
    ((moodycamel::ConcurrentQueue<Work> *) _enqueudWork)->enqueue(Work {Work::OpDisconnectInput, n, 0});
}

void AudioNode::disconnect(ContextRenderLock &, std::shared_ptr<AudioNode> node)
{
    if (!node)
    {
        for (auto & i : _inputs)
            for (auto & j : i.sources)
                j.node.reset();
    }
    else
    {
        for (auto & i : _inputs)
            for (std::vector<AudioNode::Input::Source>::iterator j = i.sources.begin(); j != i.sources.end(); ++j)
            {
                if (j->node.get() == node.get())
                {
                    j = i.sources.erase(j);
                    if (j == i.sources.end())
                        break;
                }
            }
    }
}

void AudioNode::disconnectAll()
{
    ((moodycamel::ConcurrentQueue<Work> *) _enqueudWork)->enqueue(Work {Work::OpDisconnectInput, {}, 0});
}

bool AudioNode::isConnected(std::shared_ptr<AudioNode> n)
{
    for (auto& i : _inputs)
        for (auto & j : i.sources)
            if (j.node.get() == n.get())
                return true;
    return false;
}


void AudioNode::serviceQueue(ContextRenderLock & r)
{
    moodycamel::ConcurrentQueue<Work> * work_queue = (moodycamel::ConcurrentQueue<Work> *) _enqueudWork;
    if (work_queue->size_approx() > 0)
    {
        Work work;
        while (work_queue->try_dequeue(work))
        {
            switch (work.op)
            {
                case Work::OpAddInput:
                    _inputs.emplace_back(Input {});
                    break;
                case Work::OpAddOutput:
                    _outputs.emplace_back(Output {work.name, new AudioBus(work.channelCount, work.size)});
                    break;
                case Work::OpConnectInput:
                    connect(r, work.inputIndex, work.node, work.node_outputIndex);
                    break;
                case Work::OpDisconnectIndex:
                    if (_inputs.size() > work.inputIndex)
                        _inputs[work.inputIndex].sources.clear();
                    break;
                case Work::OpDisconnectInput:
                    disconnect(r, work.node);
                    break;
            }
        }
    }
}

AudioBus* AudioNode::outputBus(ContextRenderLock & r, char const * const str)
{
    serviceQueue(r);
    for (auto & i : _outputs)
    {
        if (i.name == str)
            return i.bus;
    }
    return nullptr;
}

std::string AudioNode::outputBusName(ContextRenderLock & r, int i)
{
    serviceQueue(r);
    if (i >= _outputs.size())
        return "";

    return _outputs[i].name;
}


AudioBus* AudioNode::outputBus(ContextRenderLock & r, int i)
{
    serviceQueue(r);
    if (i >= _outputs.size())
        return nullptr;

    return _outputs[i].bus;
}

AudioBus * AudioNode::inputBus(ContextRenderLock & r, int i)
{
    serviceQueue(r);
    if (i >= _inputs.size())
        return nullptr;

    auto & sources = _inputs[i].sources;
    if (!sources.size())
        return nullptr;

    if (sources.size() == 1)
        return sources[0].node->outputBus(r, sources[0].out);

    // the summing bus should have been created when multiple inputs were added
    // the summing bus should have been evaluated when pullInputs was called
    ASSERT(_inputs[i].summingBus);
    return _inputs[i].summingBus;
}

int AudioNode::channelCount()
{
    ASSERT(m_channelCount != 0);
    return m_channelCount;
}

void AudioNode::setChannelCount(ContextGraphLock & g, int channelCount)
{
    if (!g.context())
    {
        throw std::invalid_argument("No context specified");
    }

    m_channelCount = channelCount;
}

void AudioNode::setChannelCountMode(ContextGraphLock & g, ChannelCountMode mode)
{
    if (mode >= ChannelCountMode::End || !g.context())
    {
        throw std::invalid_argument("No context specified");
    }

    m_channelCountMode = mode;
}

void AudioNode::processIfNecessary(ContextRenderLock & r, int bufferSize)
{
    if (!isInitialized())
        return;

    auto ac = r.context();
    if (!ac)
        return;

    // outputs cache results in their busses.
    // if the scheduler's recorded epoch is the same as the context's, the node
    // shall bail out as it has been processed once already this epoch.

    if (!_scheduler.update(r, bufferSize))
        return;

    ProfileScope selfScope(totalTime);
    graphTime.zero();

    if (isScheduledNode() && 
        (_scheduler._playbackState < SchedulingState::FADE_IN ||
         _scheduler._playbackState == SchedulingState::FINISHED))
    {
        silenceOutputs(r);
        return;
    }


    // there may need to be silence at the beginning or end of the current quantum.

    int start_zero_count = _scheduler._renderOffset;
    int final_zero_start = _scheduler._renderOffset + _scheduler._renderLength;
    int final_zero_count = bufferSize - final_zero_start;

    // get inputs in preparation for processing
    {
        ProfileScope scope(graphTime);
        pullInputs(r, bufferSize);
        scope.finalize();   // ensure the scope is not prematurely destructed
    }

    //  initialize the busses with start and final zeroes.
    if (start_zero_count)
    {
        for (auto & out : _outputs)
            for (int i = 0; i < out.bus->numberOfChannels(); ++i)
                memset(out.bus->channel(i)->mutableData(), 0, sizeof(float) * start_zero_count);
    }

    if (final_zero_count)
    {
        for (auto & out : _outputs)
            for (int i = 0; i < out.bus->numberOfChannels(); ++i)
                memset(out.bus->channel(i)->mutableData() + final_zero_start, 0, sizeof(float) * final_zero_count);
    }

    // do the signal processing

    process(r, bufferSize);

    // clean pops resulting from starting or stopping

#define OOS(x) (float(x) / float(steps))
    if (start_zero_count > 0 || _scheduler._playbackState == SchedulingState::FADE_IN)
    {
        int steps = start_envelope;
        int damp_start = start_zero_count;
        int damp_end = start_zero_count + steps;
        if (damp_end > bufferSize)
        {
            damp_end = bufferSize;
            steps = bufferSize - start_zero_count;
        }

        // damp out the first samples
        if (damp_end > 0)
            for (auto & out : _outputs)
                for (int i = 0; i < out.bus->numberOfChannels(); ++i)
                {
                    float* data = out.bus->channel(i)->mutableData();
                    for (int j = damp_start; j < damp_end; ++j)
                        data[j] *= OOS(j - damp_start);
                }
    }

    if (final_zero_count > 0 || _scheduler._playbackState == SchedulingState::STOPPING)
    {
        int steps = end_envelope;
        int damp_end, damp_start;
        if (!final_zero_count)
        {
            damp_end = bufferSize - final_zero_count;
            damp_start = damp_end - steps;
        }
        else
        {
            damp_end = bufferSize - final_zero_count;
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
            for (auto& out : _outputs)
                for (int i = 0; i < out.bus->numberOfChannels(); ++i)
                {
                    float* data = out.bus->channel(i)->mutableData();
                    for (int j = damp_start; j < damp_end; ++j)
                        data[j] *= OOS(damp_end - j);
                }
        }
    }

    unsilenceOutputs(r);
    selfScope.finalize(); // ensure profile is not prematurely destructed
}



bool AudioNode::propagatesSilence(ContextRenderLock& r) const
{
    return _scheduler._playbackState < SchedulingState::FADE_IN ||
        _scheduler._playbackState == SchedulingState::FINISHED;
}

void AudioNode::pullInputs(ContextRenderLock & r, int bufferSize)
{
    ASSERT(r.context());
    // exit early if already processed
    if (_scheduler.isCurrentEpoch(r))
        return;

    // Process all of the AudioNodes connected to our inputs.
    for (auto & in : _inputs)
    {
        for (auto & s : in.sources)
        {
            // only visit nodes that are not yet at the current epoch
            if (s.node && !s.node->_scheduler.isCurrentEpoch(r))
                s.node->pullInputs(r, bufferSize);
        }

        // compute the sum of all inputs if there's more than one
        if (in.sources.size() > 1)
        {
            ASSERT(in.summingBus);
            in.summingBus->zero();
            for (auto & s : in.sources)
            {
                in.summingBus->sumFrom(*s.node->outputBus(r, s.out));
            }
        }
    }

    processIfNecessary(r, bufferSize);
}

bool AudioNode::inputsAreSilent(ContextRenderLock & r)
{
    int i = 0;
    for (auto & in : _inputs)
    {
        for (auto & s : in.sources)
        {
            AudioBus * inBus = s.node ? s.node->inputBus(r, i) : nullptr;
            if (inBus && !inBus->isSilent())
                return false;
        }
    }
    return true;
}

void AudioNode::silenceOutputs(ContextRenderLock & r)
{
    for (auto & out : _outputs)
    {
        out.bus->zero();
    }
}

void AudioNode::unsilenceOutputs(ContextRenderLock & r)
{
    for (auto & out : _outputs)
    {
        out.bus->clearSilentFlag();
    }
}

std::shared_ptr<AudioParam> AudioNode::param(char const * const str)
{
    for (auto & p : _params)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

std::shared_ptr<AudioSetting> AudioNode::setting(char const * const str)
{
    for (auto & p : _settings)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

std::vector<std::string> AudioNode::paramNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _params)
    {
        ret.push_back(p->name());
    }
    return ret;
}
std::vector<std::string> AudioNode::paramShortNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _params)
    {
        ret.push_back(p->shortName());
    }
    return ret;
}

std::vector<std::string> AudioNode::settingNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _settings)
    {
        ret.push_back(p->name());
    }
    return ret;
}
std::vector<std::string> AudioNode::settingShortNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _settings)
    {
        ret.push_back(p->shortName());
    }
    return ret;
}

}  // namespace lab
