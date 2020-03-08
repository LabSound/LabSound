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

namespace lab
{

AudioNodeScheduler::AudioNodeScheduler(float sampleRate) 
    : _epochWaveLength(1.f / sampleRate)
    , _epoch(0)
    , _startWhen(std::numeric_limits<uint64_t>::max())
    , _stopWhen(std::numeric_limits<uint64_t>::max())
{
    // During construction, DeviceNodes will not yet have a sampleRate, so use 1 in that case.
    _epochWaveLength = sampleRate > 0 ? 1.f / sampleRate : 1.f;
}

bool AudioNodeScheduler::update(ContextRenderLock & r, int epoch_length)
{
    uint32_t proposed_epoch = static_cast<uint32_t>(r.context()->currentSampleFrame());
    if (_epoch >= proposed_epoch)
        return false;

    _epoch = proposed_epoch;
    _renderOffset = 0;
    _renderLength = epoch_length;

    if (_playbackState == SchedulingState::SCHEDULED)
    {
        _startWhen += _epoch;
        _playbackState = SchedulingState::PLAY_PENDING;
    }
    if (_playbackState == SchedulingState::FADE_IN)
    {
        _playbackState = SchedulingState::PLAYING;
    }
    if (_playbackState == SchedulingState::PLAY_PENDING)
    {
        if (_epoch + epoch_length >= _startWhen)
        {
            if (_epoch >= _startWhen)
            {
                _renderOffset = 0;
                _renderLength = epoch_length;
            }
            else
            {
                _renderOffset = static_cast<int>(_startWhen - _epoch);
                _renderLength = epoch_length - _renderOffset;
            }

            _playbackState = SchedulingState::FADE_IN;
            _startWhen = std::numeric_limits<uint32_t>::max();
        }
    }
    if (_playbackState == SchedulingState::PLAYING)
    {
        if (_epoch + epoch_length >= _stopWhen)
        {
            if (_epoch >= _stopWhen)
                _renderLength = epoch_length - 1;
            else
                _renderLength = static_cast<int>(epoch_length - _stopWhen - _epoch - _renderOffset);

            // be sure to trigger the damp out
            if (_renderLength == epoch_length)
                _renderLength = epoch_length - 1;

            _playbackState = SchedulingState::STOPPING;
        }
    }
    // else is here because if the state was just switched to STOPPING, it needs to render one last time before becoming UNSCHEDULED
    else if (_playbackState == SchedulingState::STOPPING)
    {
        if (_epoch + epoch_length >= _stopWhen)
        {
            // scheduled stop has occured, so make sure it doesn't immediately trigger again
            _stopWhen = std::numeric_limits<uint32_t>::max();
            _playbackState = SchedulingState::UNSCHEDULED;
        }
    }

    ASSERT(_renderOffset < epoch_length);

    return true;
}

void AudioNodeScheduler::start(double when)
{
    // if already scheduled or playing, nothing to do
    if (_playbackState == SchedulingState::SCHEDULED ||
        _playbackState == SchedulingState::PLAYING)
        return;

    // treat non finite, or max values as a cancellation of stopping or resetting
    if (!std::isfinite(when) || when == std::numeric_limits<double>::max())
    {
        if (_playbackState == SchedulingState::STOPPING ||
            _playbackState == SchedulingState::RESETTING)
        {
            _playbackState = SchedulingState::PLAYING;
            _stopWhen = std::numeric_limits<uint32_t>::max();
        }
        return;
    }

    _startWhen = static_cast<uint32_t>(when * _epochWaveLength);
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
        _stopWhen = std::numeric_limits<uint32_t>::max(); // cancel stop
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
    _stopWhen = _epoch + static_cast<uint32_t>(when * _epochWaveLength);
}

void AudioNodeScheduler::reset()
{
    _startWhen = std::numeric_limits<uint32_t>::max();;
    _stopWhen = std::numeric_limits<uint32_t>::max();;
    if (_playbackState != SchedulingState::UNSCHEDULED)
        _playbackState = SchedulingState::RESETTING;
}

void AudioNodeScheduler::finish(ContextRenderLock & r)
{
    if (_playbackState < SchedulingState::PLAYING)
        _playbackState = SchedulingState::FINISHING;
    else if (_playbackState > SchedulingState::PLAYING &&
             _playbackState < SchedulingState::FINISHED)
        _playbackState = SchedulingState::FINISHING;

    r.context()->enqueueEvent(_onEnded);
}



AudioNode::AudioNode(AudioContext & ac)
    : _scheduler(ac.sampleRate())
{ }



AudioNode::~AudioNode()
{
    uninitialize();
}

void AudioNode::initialize()
{
    m_isInitialized = true;
}

void AudioNode::uninitialize()
{
    m_isInitialized = false;
}

void AudioNode::reset(ContextRenderLock& r)
{
    _scheduler.reset();
}

void AudioNode::addInput(std::unique_ptr<AudioNodeInput> input)
{
    m_inputs.emplace_back(std::move(input));
}

void AudioNode::addOutput(std::unique_ptr<AudioNodeOutput> output)
{
    m_outputs.emplace_back(std::move(output));
}

// safe without a Render lock because vector is immutable
std::shared_ptr<AudioNodeInput> AudioNode::input(int i)
{
    if (i < m_inputs.size()) return m_inputs[i];
    return nullptr;
}

// safe without a Render lock because vector is immutable
std::shared_ptr<AudioNodeOutput> AudioNode::output(int i)
{
    if (i < m_outputs.size()) return m_outputs[i];
    return nullptr;
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

    if (m_channelCount != channelCount)
    {
        if (m_channelCount != channelCount)
        {
            m_channelCount = channelCount;
            if (m_channelCountMode != ChannelCountMode::Max)
            {
                updateChannelsForInputs(g);
            }
        }
    }
}

void AudioNode::setChannelCountMode(ContextGraphLock & g, ChannelCountMode mode)
{
    if (mode >= ChannelCountMode::End || !g.context())
    {
        throw std::invalid_argument("No context specified");
    }

    if (m_channelCountMode != mode)
    {
        m_channelCountMode = mode;
        updateChannelsForInputs(g);
    }
}

void AudioNode::updateChannelsForInputs(ContextGraphLock & g)
{
    for (auto & input : m_inputs)
        input->changedOutputs(g);
}

void AudioNode::processIfNecessary(ContextRenderLock & r, int bufferSize, int offset, int count)
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

    if (_scheduler._playbackState < SchedulingState::FADE_IN ||
        _scheduler._playbackState == SchedulingState::FINISHED)
    {
        silenceOutputs(r);
        return;
    }

    // there may need to be silence at the beginning or end of the current quantum.

    int start_zeroes = _scheduler._renderOffset;
    int final_zeroes = bufferSize - start_zeroes - _scheduler._renderLength;

    // get inputs in preparation for processing

    pullInputs(r, bufferSize, _scheduler._renderOffset, _scheduler._renderLength);

    //  initialize the busses with start and final zeroes.

    if (start_zeroes)
    {
        for (auto & out : m_outputs)
            for (int i = 0; i < out->numberOfChannels(); ++i)
                memset(out->bus(r)->channel(i)->mutableData(), 0, sizeof(float) * start_zeroes);
    }

    if (final_zeroes)
    {
        for (auto & out : m_outputs)
            for (int i = 0; i < out->numberOfChannels(); ++i)
                memset(out->bus(r)->channel(i)->mutableData() + bufferSize - final_zeroes,
                    0, sizeof(float) * final_zeroes);
    }

    // do the signal processing

    process(r, bufferSize, start_zeroes, bufferSize - start_zeroes - final_zeroes);

    // clean pops resulting from starting or stopping

#define OOS(x) (float(x) / float(steps))
    if (_scheduler._playbackState == SchedulingState::FADE_IN)
    {
        int steps = 64;

        // damp out the first samples
        int damp = std::min(start_zeroes + steps, bufferSize) - steps;
        for (auto & out : m_outputs)
            for (int i = 0; i < out->bus(r)->numberOfChannels(); ++i)
            {
                float* data = out->bus(r)->channel(i)->mutableData() + damp;
                for (int j = 0; j < steps; ++j)
                    data[j] *= OOS(j);
            }
    }

    if (final_zeroes)
    {
        int steps = 64;

        // damp out the last samples
        int damp = std::min(bufferSize - final_zeroes + steps, bufferSize) - steps;
        for (auto & out : m_outputs)
            for (int i = 0; i < out->numberOfChannels(); ++i)
            {
                float* data = out->bus(r)->channel(i)->mutableData() + damp;
                for (int j = 0; j < steps; ++j)
                    data[j] *= OOS(steps - j);
            }
    }

    unsilenceOutputs(r);
}

void AudioNode::checkNumberOfChannelsForInput(ContextRenderLock & r, AudioNodeInput * input)
{
    ASSERT(r.context());
    for (auto & in : m_inputs)
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
    return _scheduler._playbackState < SchedulingState::FADE_IN ||
        _scheduler._playbackState == SchedulingState::FINISHED;
}

void AudioNode::pullInputs(ContextRenderLock & r, int bufferSize, int offset, int count)
{
    ASSERT(r.context());

    // Process all of the AudioNodes connected to our inputs.
    for (auto & in : m_inputs)
    {
        in->pull(r, 0, bufferSize, offset, count);
    }
}

bool AudioNode::inputsAreSilent(ContextRenderLock & r)
{
    for (auto & in : m_inputs)
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
    for (auto out : m_outputs)
    {
        out->bus(r)->zero();
    }
}

void AudioNode::unsilenceOutputs(ContextRenderLock & r)
{
    for (auto out : m_outputs)
    {
        out->bus(r)->clearSilentFlag();
    }
}

std::shared_ptr<AudioParam> AudioNode::param(char const * const str)
{
    for (auto & p : m_params)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

std::shared_ptr<AudioSetting> AudioNode::setting(char const * const str)
{
    for (auto & p : m_settings)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

std::vector<std::string> AudioNode::paramNames() const
{
    std::vector<std::string> ret;
    for (auto & p : m_params)
    {
        ret.push_back(p->name());
    }
    return ret;
}
std::vector<std::string> AudioNode::paramShortNames() const
{
    std::vector<std::string> ret;
    for (auto & p : m_params)
    {
        ret.push_back(p->shortName());
    }
    return ret;
}

std::vector<std::string> AudioNode::settingNames() const
{
    std::vector<std::string> ret;
    for (auto & p : m_settings)
    {
        ret.push_back(p->name());
    }
    return ret;
}
std::vector<std::string> AudioNode::settingShortNames() const
{
    std::vector<std::string> ret;
    for (auto & p : m_settings)
    {
        ret.push_back(p->shortName());
    }
    return ret;
}

}  // namespace lab
