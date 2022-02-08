// License: BSD 2 Clause
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

#include <functional>
#include <string>
#include <math.h>

using namespace std;

//#define ASN_PRINT(...)
#define ASN_PRINT(a) printf(a)




namespace lab
{

// static
void AudioNode::_printGraph(const AudioNode * root, std::function<void(const char *)> prnln, int indent)
{
    const int maxIndent = 40;
    static char * spacesBuff = nullptr;
    if (!spacesBuff) {
        spacesBuff = (char *) malloc(maxIndent + 1);
        memset(spacesBuff, ' ', maxIndent);
        spacesBuff[maxIndent] = '\0';
    }
    int spaceTerminator = indent < maxIndent ? indent : maxIndent;
    spacesBuff[spaceTerminator] = '\0';
    std::string spaces = spacesBuff;
    spacesBuff[spaceTerminator] = ' ';

    std::string str = spaces;
    str += std::string(root->name());
    prnln(str.c_str());

    const auto params = root->params();
    auto names = root->paramNames();
    for (int i = 0; i < params.size(); ++i)
    {
        auto & inputs = params[i]->inputs();
        if (inputs.sources.size() > 0) {
            str = spaces + "   [Param] " + names[i];
            prnln(str.c_str());
            for (const auto & n : inputs.sources)
                _printGraph(n->get(), prnln, indent + 3);
        }
    }

    if (root->_inputs.size() > 0)
    {
        prnln(str.c_str());
        for (auto & i : root->_inputs)
        {
            if (!i)
                continue;

            str = spaces + "   [Input] ";
            prnln(str.c_str());
            for (auto & n : i->sources)
                _printGraph(n->get(), prnln, indent + 3);
        }
    }
}


// static
void AudioNode::printGraph(const AudioNode * root, std::function<void(const char *)> prnln)
{
    if (!root || !prnln)
        return;

    _printGraph(root, prnln, 0);
}

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

AudioNodeSummingInput::AudioNodeSummingInput(const std::string name) noexcept
: name(name)
{
    summingBus = nullptr;
}

AudioNodeSummingInput::~AudioNodeSummingInput()
{
}

void AudioNodeSummingInput::clear() noexcept
{
    if (summingBus)
    {
        summingBus = nullptr;
    }
    sources.clear();
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
    // @dp suppress epoch check while the main loop is being rewritten
    uint64_t proposed_epoch = r.context()->currentSampleFrame();
    //if (_epoch >= proposed_epoch)
    //    return false;

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
    if (!isfinite(when) || when == std::numeric_limits<double>::max())
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
    if (!isfinite(when) || when == std::numeric_limits<double>::max())
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
    , _enqueudWork((ConcurrentQueue *) new moodycamel::ConcurrentQueue<Work>())
    , _output(nullptr)
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
    ((moodycamel::ConcurrentQueue<Work> *) 
        _enqueudWork)->enqueue(Work {Work::OpAddInput, {}, 0, 0, name});
}

const AudioNodeSummingInput* AudioNode::input(int index) const
{
    return _inputs[index];
}

int AudioNode::input_index(const char* name) const
{
    std::string cmp(name);
    int count = (int) _inputs.size();
    for (int i = 0; i < count; ++i) {
        if (_inputs[i] && _inputs[i]->name == cmp)
            return i;
    }
    return -1;
}

void AudioNode::connect(int input_index, std::shared_ptr<AudioNode> n)
{
    ((moodycamel::ConcurrentQueue<Work> *) 
        _enqueudWork)->enqueue(Work {Work::OpConnectInput, n, 0, input_index});
}

void AudioNode::connect(ContextRenderLock & r, 
    int input_index, std::shared_ptr<AudioNode> n)
{
    if (!_inputs[input_index]) {
        // _inputs will autoallocate a slot, so fill it in
        std::string name = "in" + std::to_string(input_index);
        _inputs[input_index] = new AudioNodeSummingInput(name);
    }

    std::shared_ptr<AudioNode> * copy = new std::shared_ptr<AudioNode>();
    *copy = n;
    _inputs[input_index]->sources.emplace_back(copy);
}

void AudioNode::disconnect(int inputIndex)
{
    ((moodycamel::ConcurrentQueue<Work> *) 
        _enqueudWork)->enqueue(Work {Work::OpDisconnectIndex, {}, 0, inputIndex});
}

void AudioNode::disconnect(std::shared_ptr<AudioNode> n)
{
    ((moodycamel::ConcurrentQueue<Work> *) 
        _enqueudWork)->enqueue(Work {Work::OpDisconnectInput, n, 0});
}

void AudioNode::disconnect(ContextRenderLock &, std::shared_ptr<AudioNode> node)
{
    if (!node)
    {
        for (auto i : _inputs)
            if (i)
                i->sources.clear();
    }
    else
    {
        for (auto input : _inputs)
        {
            if (!input)
                continue;

            int src_count = (int) input->sources.size();
            for (int j = 0; j < src_count; ++j)
            {
                if (input->sources[j] && input->sources[j]->get() == node.get())
                {
                    delete input->sources[j];
                    input->sources.swap_pop(j);
                    --src_count;
                }
            }
        }
    }
}

void AudioNode::disconnectAll()
{
    ((moodycamel::ConcurrentQueue<Work> *) _enqueudWork)->enqueue(Work {Work::OpDisconnectInput, {}, -1});
}

bool AudioNode::isConnected(std::shared_ptr<AudioNode> n)
{
    for (auto & i : _inputs)
        if (i)
            for (auto & j : i->sources)
                if (j->get() == n.get())
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
                {
                    std::string name = work.name;
                    if (name.length() == 0)
                    {
                        name = "in" + std::to_string(_inputs.size() + 1);
                    }
                    _inputs.emplace_back(AudioNodeSummingInput(name));
                    break;
                }
                case Work::OpConnectInput:
                    connect(r, work.inputIndex, work.node);
                    break;
                case Work::OpDisconnectIndex:
                    if (work.inputIndex == -1)
                    {
                        for (auto & i : _inputs)
                            if (i)
                                i->sources.clear();
                    }
                    else if (_inputs[work.inputIndex])
                        _inputs[work.inputIndex]->sources.clear();
                    break;
                case Work::OpDisconnectInput:
                    disconnect(r, work.node);
                    break;
            }
        }
    }
}

std::string AudioNode::inputBusName(ContextRenderLock & r, int i)
{
    serviceQueue(r);
    if (!_inputs[i])
        return "";

    return _inputs[i]->name;
}


AudioBus* AudioNode::outputBus(ContextRenderLock & r)
{
    return _output;
}

AudioBus * AudioNode::inputBus(ContextRenderLock & r, int i)
{
    serviceQueue(r);
    if (!_inputs[i])
        return nullptr;

    auto & sources = _inputs[i]->sources;
    if (!sources.size())
        return nullptr;

    if (sources.size() == 1)
    {
        if (!sources[0]->get())
            return nullptr;

        return (*sources[0])->outputBus(r);
    }

    // the summing bus should have been created when multiple inputs were added
    // the summing bus should have been evaluated when pullInputs was called
    ASSERT(_inputs[i]->summingBus);
    return _inputs[i]->summingBus;
}

void AudioNode::pullInputs(ContextRenderLock & r, int bufferSize)
{
    /// @TODO remove pullInputs
    #if 0
    // exit early if already processed
    if (_scheduler.isCurrentEpoch(r))
        return;
    
    if (!isInitialized())
        return;

    auto ac = r.context();
    if (!ac)
        return;

    // the scheduler says the node isn't running, bail out
    if (!_scheduler.update(r, bufferSize))
        return;

    if (isScheduledNode() && (_scheduler._playbackState < SchedulingState::FADE_IN || _scheduler._playbackState == SchedulingState::FINISHED))
    {
        silenceOutputs(r);
        return;
    }

    ProfileScope selfScope(totalTime);
    {
        // process all the node's inputs

        graphTime.zero();
        ProfileScope scope(graphTime);

        for (auto & p : _params)
        {
            p->serviceQueue(r);
            for (auto & i : p->inputs().sources)
            {
                if (!i)
                    continue;
                auto output = (*i)->outputBus(r);
                if (!output)
                    continue;

                // Render audio feeding the parameter
                (*i)->pullInputs(r, bufferSize);
            }
        }

        // Process all of the AudioNodes connected to our inputs.
        for (auto & in : _inputs)
        {
            if (!in)
                continue;

            for (auto & s : in->sources)
            {
                // only visit nodes that are not yet at the current epoch
                if (s &&  !(*s)->_scheduler.isCurrentEpoch(r))
                    (*s)->pullInputs(r, bufferSize);
            }

            // compute the sum of all inputs if there's more than one
            if (in->sources.size() > 1)
            {
                ASSERT(in->summingBus);

                int requiredChannels = 1;
                for (auto & s : in->sources)
                {
                    if (!s)
                        continue;

                    auto bus = (*s)->outputBus(r);
                    if (bus && bus->numberOfChannels() > requiredChannels)
                        requiredChannels = bus->numberOfChannels();
                }
                if (requiredChannels != in->summingBus->numberOfChannels())
                {
                    in->summingBus->setNumberOfChannels(r, requiredChannels);
                }

                in->summingBus->zero();
                for (auto & s : in->sources)
                {
                    if (!s)
                        continue;
                    auto bus = (*s)->outputBus(r);
                    if (bus)
                        in->summingBus->sumFrom(*bus);
                }
            }
        }
        scope.finalize();  // ensure the scope is not prematurely destructed
    }


    // do the signal processing for this node

    process(r, bufferSize);

    // there may need to be silence at the beginning or end of the current quantum.

    int start_zero_count = _scheduler._renderOffset;
    int final_zero_start = _scheduler._renderOffset + _scheduler._renderLength;
    int final_zero_count = bufferSize - final_zero_start;

    //  initialize the busses with start and final zeroes.
    if (start_zero_count)
    {
        for (int i = 0; i < _output.bus->numberOfChannels(); ++i)
            memset(_output.bus->channel(i)->mutableData(), 0, 
                    sizeof(float) * start_zero_count);
    }

    if (final_zero_count)
    {
        for (int i = 0; i < _output.bus->numberOfChannels(); ++i)
            memset(_output.bus->channel(i)->mutableData() + final_zero_start, 0, 
                    sizeof(float) * final_zero_count);
    }

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
            for (int i = 0; i < _output.bus->numberOfChannels(); ++i)
            {
                float* data = _output.bus->channel(i)->mutableData();
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
            for (int i = 0; i < _output.bus->numberOfChannels(); ++i)
            {
                float* data = _output.bus->channel(i)->mutableData();
                for (int j = damp_start; j < damp_end; ++j)
                    data[j] *= OOS(damp_end - j);
            }
        }
    }

    unsilenceOutputs(r);
    selfScope.finalize(); // ensure profile is not prematurely destructed
    #endif
}



bool AudioNode::propagatesSilence(ContextRenderLock& r) const
{
    return _scheduler._playbackState < SchedulingState::FADE_IN ||
        _scheduler._playbackState == SchedulingState::FINISHED;
}

bool AudioNode::inputsAreSilent(ContextRenderLock & r)
{
    int i = 0;
    for (auto & in : _inputs)
    {
        if (!in)
            continue;

        for (auto & node : in->sources)
        {
            AudioBus * inBus = node ? (*node)->inputBus(r, i) : nullptr;
            if (inBus && !inBus->isSilent())
                return false;
        }
    }
    return true;
}

void AudioNode::silenceOutputs(ContextRenderLock & r)
{
    _output->zero();
}

void AudioNode::unsilenceOutputs(ContextRenderLock & r)
{
    _output->clearSilentFlag();
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

int AudioNode::param_index(char const * const str)
{
    int count = (int) _params.size();
    for (int i = 0; i < count; ++i)
    {
        if (!strcmp(str, _params[i]->name().c_str()))
            return i;
    }
    return -1;
}

std::shared_ptr<AudioParam> AudioNode::param(int index)
{
    if (index >= _params.size())
        return {};

    return _params[index];
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

int AudioNode::setting_index(char const * const str)
{
    int count = (int) _settings.size();
    for (int i = 0; i < count; ++i)
    {
        if (!strcmp(str, _settings[i]->name().c_str()))
            return i;
    }
    return -1;
}

std::shared_ptr<AudioSetting> AudioNode::setting(int index)
{
    if (index >= _settings.size())
        return {};

    return _settings[index];
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
