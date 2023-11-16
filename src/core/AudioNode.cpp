// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"

using namespace std;

#define LOG_PLAYBACK_STATE_TRANSITION(node_name, old_state, new_state) printf("Scheduler: %s ⮕ %s (%s)\n", (schedulingStateName(old_state)), (schedulingStateName(new_state)), (node_name))

namespace lab
{

AudioNode::Internal::Internal(AudioContext & ac)
:  _scheduler(ac.sampleRate())
{}

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
/*        int input_count = params[i]->inputs();
        if (inputs.sources.size() > 0) {
            str = spaces + "   [Param] " + names[i];
            prnln(str.c_str());
            for (const auto & n : inputs.sources)
                _printGraph(n->get(), prnln, indent + 3);
        }*/
    }

    if (root->_self->m_inputs.size() > 0)
    {
        prnln(str.c_str());
        for (auto & i : root->_self->m_inputs)
        {
            if (!i)
                continue;

            str = spaces + "   [Input] ";
            prnln(str.c_str());
//            for (auto & n : i->sources)
//                _printGraph(n->get(), prnln, indent + 3);
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

/// @TODO when the ls2 merge is done, move this to its own source file
///
const char * schedulingStateName(SchedulingState s)
{
    switch (s)
    {
    case SchedulingState::UNSCHEDULED: return "UNSCHEDULED";
    case SchedulingState::SCHEDULED: return "SCHEDULED";
    case SchedulingState::FADE_IN: return "FADE_IN";
    case SchedulingState::PLAYING: return "PLAYING";
    case SchedulingState::STOPPING: return "STOPPING";
    case SchedulingState::RESETTING: return "RESETTING";
    case SchedulingState::FINISHING: return "FINISHING";
    case SchedulingState::FINISHED: return "FINISHED";
    }
    return "Unknown";
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

bool AudioNodeScheduler::update(ContextRenderLock & r, int epoch_length, const char* node_name)
{
    assert(node_name != nullptr);
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
                LOG_PLAYBACK_STATE_TRANSITION(node_name, _playbackState, SchedulingState::FADE_IN);
                _playbackState = SchedulingState::FADE_IN;
            }
            else if (_startWhen < _epoch + epoch_length)
            {
                // start falls within the frame
                _renderOffset = static_cast<int>(_startWhen - _epoch);
                _renderLength = epoch_length - _renderOffset;
                LOG_PLAYBACK_STATE_TRANSITION(node_name, _playbackState, SchedulingState::FADE_IN);
                _playbackState = SchedulingState::FADE_IN;
            }

            /// @TODO the case of a start and stop within one epoch needs to be special
            /// cased, to fit this current architecture, there'd be a FADE_IN_OUT scheduling
            /// state so that the envelope can be correctly applied.
            // FADE_IN_OUT would transition to UNSCHEDULED.
            break;

        case SchedulingState::FADE_IN:
            // start time has been achieved, there'll be one quantum with fade in applied.
            _renderOffset = 0;
            LOG_PLAYBACK_STATE_TRANSITION(node_name, _playbackState, SchedulingState::PLAYING);
            _playbackState = SchedulingState::PLAYING;
            // fall through to PLAYING to allow render length to be adjusted if stop-start is less than one quantum length

        case SchedulingState::PLAYING:
            /// @TODO include the end envelope in the stop check so that a scheduled stop that
            /// spans this quantum and the next ends in the current quantum

            if (_stopWhen <= _epoch)
            {
                // exactly on start, or late, stop straight away, render a whole frame of fade out
                _renderLength = epoch_length - _renderOffset;
                LOG_PLAYBACK_STATE_TRANSITION(node_name, _playbackState, SchedulingState::STOPPING);
                _playbackState = SchedulingState::STOPPING;
            }
            else if (_stopWhen < _epoch + epoch_length)
            {
                // stop falls within the frame
                _renderOffset = 0;
                _renderLength = static_cast<int>(_stopWhen - _epoch);
                LOG_PLAYBACK_STATE_TRANSITION(node_name, _playbackState, SchedulingState::STOPPING);
                _playbackState = SchedulingState::STOPPING;
            }

            // do not fall through to STOPPING because one quantum must render the fade out effect
            break;

        case SchedulingState::STOPPING:
            if (_epoch + epoch_length >= _stopWhen)
            {
                // scheduled stop has occured, so make sure stop doesn't immediately trigger again
                _stopWhen = std::numeric_limits<uint64_t>::max();
                LOG_PLAYBACK_STATE_TRANSITION(node_name, _playbackState, SchedulingState::UNSCHEDULED);
                _playbackState = SchedulingState::UNSCHEDULED;
                if (_onEnded)
                    r.context()->enqueueEvent(_onEnded);
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
    : _self(new Internal(ac))
{
    if (desc.params) {
        AudioParamDescriptor const * i = desc.params;
        while (i->name)
        {
            _self->_params.push_back(std::make_shared<AudioParam>(i));
            ++i;
        }
    }
    if (desc.settings) {
        AudioSettingDescriptor const * i = desc.settings;
        while (i->name)
        {
            _self->_settings.push_back(std::make_shared<AudioSetting>(i));
            ++i;
        }
    }
    
    _self->m_channelCount = desc.initialChannelCount;
    if (_self->m_channelCount > 0) {
        addOutput(std::unique_ptr<AudioNodeOutput>(
                    new AudioNodeOutput(this, _self->m_channelCount)));
    }
}

AudioNode::~AudioNode()
{
    uninitialize();
}

void AudioNode::initialize()
{
    _self->m_isInitialized = true;
}

void AudioNode::uninitialize()
{
    _self->m_isInitialized = false;
}

void AudioNode::reset(ContextRenderLock & r)
{
    _self->_scheduler.reset();
}

void AudioNode::addInput(ContextGraphLock&, std::unique_ptr<AudioNodeInput> input)
{
    _self->m_inputs.emplace_back(std::move(input));
}

void AudioNode::addOutput(ContextGraphLock&, std::unique_ptr<AudioNodeOutput> output)
{
    _self->m_outputs.emplace_back(std::move(output));
}

void AudioNode::addInput(std::unique_ptr<AudioNodeInput> input)
{
    input->setName("in" + to_string((int) _self->m_inputs.size()));
    _self->m_inputs.emplace_back(std::move(input));
}

void AudioNode::addOutput(std::unique_ptr<AudioNodeOutput> output)
{
    _self->m_outputs.emplace_back(std::move(output));
}

// safe without a Render lock because vector is immutable
std::shared_ptr<AudioNodeInput> AudioNode::input(int i)
{
    if (i < _self->m_inputs.size())
        return _self->m_inputs[i];
    return {};
}

std::shared_ptr<AudioNodeInput> AudioNode::input(char const* const str)
{
    for (auto & i : _self->m_inputs)
    {
        if (i->name() == str)
            return i;
    }
    return {};
}

// safe without a Render lock because vector is immutable
std::shared_ptr<AudioNodeOutput> AudioNode::output(int i)
{
    if (i < _self->m_outputs.size())
        return _self->m_outputs[i];
    return {};
}

std::shared_ptr<AudioNodeOutput> AudioNode::output(char const* const str)
{
    for (auto & i : _self->m_outputs)
    {
        if (i->name() == str)
            return i;
    }
    return {};
}

int AudioNode::channelCount()
{
    ASSERT(_self->m_channelCount != 0);
    return _self->m_channelCount;
}

void AudioNode::setChannelCount(ContextGraphLock & g, int channelCount)
{
    if (!g.context())
    {
        throw std::invalid_argument("No context specified");
    }

    if (_self->m_channelCount != channelCount)
    {
        _self->m_channelCount = channelCount;
        if (_self->m_channelCountMode != ChannelCountMode::Max)
        {
            for (auto& input : _self->m_inputs)
                input->changedOutputs(g);
        }
    }
}

void AudioNode::setChannelCountMode(ContextGraphLock & g, ChannelCountMode mode)
{
    if (mode >= ChannelCountMode::End || !g.context())
    {
        throw std::invalid_argument("No context specified");
    }

    if (_self->m_channelCountMode != mode)
    {
        _self->m_channelCountMode = mode;
        for (auto& input : _self->m_inputs)
            input->changedOutputs(g);
    }
}

void AudioNode::processIfNecessary(ContextRenderLock & r, int bufferSize)
{
    auto ac = r.context();
    if (!ac)
        return;

    bool diagnosing_silence = ac->diagnosing().get() == this;

    if (diagnosing_silence) {
        if (_self->_scheduler._playbackState < SchedulingState::FADE_IN ||
            _self->_scheduler._playbackState > SchedulingState::PLAYING)
        {
            ac->diagnosed_silence("Scheduled node not playing");
        }
    }

    if (!isInitialized()) {
        if (diagnosing_silence)
            ac->diagnosed_silence("Not initialized");
        return;
    }

    // outputs cache results in their busses.
    // if the scheduler's recorded epoch is the same as the context's, the node
    // shall bail out as it has been processed once already this epoch.

    if (!_self->_scheduler.update(r, bufferSize, name())) {
        if (diagnosing_silence)
            ac->diagnosed_silence("Already processed");
        return;
    }

    ProfileScope selfScope(_self->totalTime);
    _self->graphTime.zero();

    if (isScheduledNode() && 
        (_self->_scheduler._playbackState < SchedulingState::FADE_IN ||
         _self->_scheduler._playbackState == SchedulingState::FINISHED))
    {
        silenceOutputs(r);
        if (diagnosing_silence)
            ac->diagnosed_silence("Unscheduled");
        return;
    }


    // there may need to be silence at the beginning or end of the current quantum.

    int start_zero_count = _self->_scheduler._renderOffset;
    int final_zero_start = _self->_scheduler._renderOffset + _self->_scheduler._renderLength;
    int final_zero_count = bufferSize - final_zero_start;

    // if the input counts need to match the output counts,
    // do it here before pulling inputs
    conformChannelCounts();

    // get inputs in preparation for processing
    {
        ProfileScope scope(_self->graphTime);
        pullInputs(r, bufferSize);
        scope.finalize();   // ensure the scope is not prematurely destructed
    }

    // ensure all requested channel count updates have been resolved
    for (auto& out : _self->m_outputs)
        out->updateRenderingState(r);

    //  initialize the busses with start and final zeroes.
    if (start_zero_count)
    {
        for (auto & out : _self->m_outputs)
            for (int i = 0; i < out->numberOfChannels(); ++i)
                memset(out->bus(r)->channel(i)->mutableData(), 0, sizeof(float) * start_zero_count);
    }

    if (final_zero_count)
    {
        for (auto & out : _self->m_outputs)
            for (int i = 0; i < out->numberOfChannels(); ++i)
                memset(out->bus(r)->channel(i)->mutableData() + final_zero_start, 0, sizeof(float) * final_zero_count);
    }

    // do the signal processing

    process(r, bufferSize);

    // clean pops resulting from starting or stopping

    #define OOS(x) (float(x) / float(steps))
    if (start_zero_count > 0 || _self->_scheduler._playbackState == SchedulingState::FADE_IN)
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
            for (auto & out : _self->m_outputs)
                for (int i = 0; i < out->bus(r)->numberOfChannels(); ++i)
                {
                    float* data = out->bus(r)->channel(i)->mutableData();
                    for (int j = damp_start; j < damp_end; ++j)
                        data[j] *= OOS(j - damp_start);
                }
    }

    if (final_zero_count > 0 || _self->_scheduler._playbackState == SchedulingState::STOPPING)
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
            for (auto& out : _self->m_outputs)
                for (int i = 0; i < out->numberOfChannels(); ++i)
                {
                    float* data = out->bus(r)->channel(i)->mutableData();
                    for (int j = damp_start; j < damp_end; ++j)
                        data[j] *= OOS(damp_end - j);
                }
        }
    }

    unsilenceOutputs(r);
    selfScope.finalize(); // ensure profile is not prematurely destructed
}

void AudioNode::conformChannelCounts()
{
    return;

    // no generic count conforming. nodes are responsible if they are going to do it at all
/*
    int inputChannelCount = input->numberOfChannels(r);

    bool channelCountChanged = false;
    for (int i = 0; i < numberOfOutputs() && !channelCountChanged; ++i)
    {
        channelCountChanged = isInitialized() && inputChannelCount != output(i)->numberOfChannels();
    }

    if (channelCountChanged)
    {
        uninitialize();
        for (int i = 0; i < numberOfOutputs(); ++i)
        {
            // This will propagate the channel count to any nodes connected further down the chain...
            output(i)->setNumberOfChannels(r, inputChannelCount);
        }
        initialize();
    }
    */
}

void AudioNode::checkNumberOfChannelsForInput(ContextRenderLock & r, AudioNodeInput * input)
{
    ASSERT(r.context());

    if (!input || input != this->input(0).get())
        return;

    for (auto & in : _self->m_inputs)
    {
        if (in.get() == input)
        {
            input->updateInternalBus(r);
            break;
        }
    }
}

bool AudioNode::propagatesSilence(ContextRenderLock& r) const
{
    return _self->_scheduler._playbackState < SchedulingState::FADE_IN ||
           _self->_scheduler._playbackState == SchedulingState::FINISHED;
}

void AudioNode::pullInputs(ContextRenderLock & r, int bufferSize)
{
    ASSERT(r.context());

    // Process all of the AudioNodes connected to our inputs.
    for (auto & in : _self->m_inputs)
    {
        in->pull(r, 0, bufferSize);
    }
}

bool AudioNode::inputsAreSilent(ContextRenderLock & r)
{    
    for (auto & in : _self->m_inputs)
    {
        if (!in->bus(r)->isSilent())
        {
            return false;
        }
    }
    return true;
}

void AudioNode::silenceOutputs(ContextRenderLock & r)
{
    for (auto out : _self->m_outputs)
    {
        out->bus(r)->zero();
    }
}

void AudioNode::unsilenceOutputs(ContextRenderLock & r)
{
    for (auto out : _self->m_outputs)
    {
        out->bus(r)->clearSilentFlag();
    }
}

std::shared_ptr<AudioParam> AudioNode::param(char const * const str)
{
    for (auto & p : _self->_params)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

int AudioNode::param_index(char const * const str)
{
    int count = (int) _self->_params.size();
    for (int i = 0; i < count; ++i)
    {
        if (!strcmp(str, _self->_params[i]->name().c_str()))
            return i;
    }
    return -1;
}

std::shared_ptr<AudioParam> AudioNode::param(int index)
{
    if (index >= _self->_params.size())
        return {};

    return _self->_params[index];
}

std::shared_ptr<AudioSetting> AudioNode::setting(char const * const str)
{
    for (auto & p : _self->_settings)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

int AudioNode::setting_index(char const * const str)
{
    int count = (int) _self->_settings.size();
    for (int i = 0; i < count; ++i)
    {
        if (!strcmp(str, _self->_settings[i]->name().c_str()))
            return i;
    }
    return -1;
}

std::shared_ptr<AudioSetting> AudioNode::setting(int index)
{
    if (index >= _self->_settings.size())
        return {};

    return _self->_settings[index];
}

std::vector<std::string> AudioNode::paramNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _self->_params)
    {
        ret.push_back(p->name());
    }
    return ret;
}
std::vector<std::string> AudioNode::paramShortNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _self->_params)
    {
        ret.push_back(p->shortName());
    }
    return ret;
}

std::vector<std::string> AudioNode::settingNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _self->_settings)
    {
        ret.push_back(p->name());
    }
    return ret;
}
std::vector<std::string> AudioNode::settingShortNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _self->_settings)
    {
        ret.push_back(p->shortName());
    }
    return ret;
}

}  // namespace lab
