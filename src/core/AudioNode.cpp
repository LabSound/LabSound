// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"

using namespace std;

//#define ASN_PRINT(...)
#define ASN_PRINT(a) printf("Scheduler: %s", (a))

namespace lab
{

AudioNode::Internal::Internal(AudioContext & ac)
: scheduler(ac.sampleRate())
, output(new AudioBus(0, AudioNode::ProcessingSizeInFrames, true))
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

    for (auto & i : root->_self->inputs)
    {
        str = spaces + "   [Input] " + i.node->name();
        prnln(str.c_str());
        _printGraph(i.node.get(), prnln, indent + 3);
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
    case SchedulingState::SCHEDULED:   return "SCHEDULED";
    case SchedulingState::FADE_IN:     return "FADE_IN";
    case SchedulingState::PLAYING:     return "PLAYING";
    case SchedulingState::STOPPING:    return "STOPPING";
    case SchedulingState::RESETTING:   return "RESETTING";
    case SchedulingState::FINISHING:   return "FINISHING";
    case SchedulingState::FINISHED:    return "FINISHED";
    }
    return "Unknown";
}


AudioNodeScheduler::AudioNodeScheduler(float sampleRate)
    : _epoch(0)
    , _startWhen(std::numeric_limits<uint64_t>::max())
    , _stopWhen(std::numeric_limits<uint64_t>::max())
    , _sampleRate(sampleRate)
{
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
    if (desc.params)
    {
        AudioParamDescriptor const * i = desc.params;
        while (i->name)
        {
            _self->params.push_back(std::make_shared<AudioParam>(i));
            ++i;
        }
    }
    if (desc.settings)
    {
        AudioSettingDescriptor const * i = desc.settings;
        while (i->name)
        {
            _self->settings.push_back(std::make_shared<AudioSetting>(i));
            ++i;
        }
    }
}

AudioNode::~AudioNode()
{
    // the destructor needs to remove "this" from all the receivers in order
    // to eliminate retain cycles.
    for (auto rcv : _self->receivers) {
        // iterate rcv->inputs and remove this from each
        for (auto it = rcv->_self->inputs.begin(); it != rcv->_self->inputs.end(); ) {
            if ((*it).node.get() == this) {
                it = rcv->_self->inputs.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    uninitialize();
}

void AudioNode::initialize()
{
    _self->isInitialized = true;
}

void AudioNode::uninitialize()
{
    _self->isInitialized = false;
}

void AudioNode::reset(ContextRenderLock & r)
{
    _self->scheduler.reset();
}

// safe without a Render lock because vector is immutable
std::shared_ptr<AudioNode> AudioNode::input(int i)
{
    if (i < _self->inputs.size())
        return _self->inputs[i].node;
    return {};
}

std::shared_ptr<AudioBus> AudioNode::summedInput()
{
    if (_self->inputs.size() == 0)
        return {};
    if (_self->inputs.size() == 1)
        return _self->inputs[0].node->output();
    if (!_self->summingBus)
        _self->summingBus = std::move(AudioBus::createByCloning(_self->inputs[0].node->output().get()));
    for (int i = 1; i < _self->inputs.size(); ++i)
        _self->summingBus->sumFrom(*_self->inputs[i].node->output().get());
    return _self->summingBus;
}


int AudioNode::channelCount()
{
    if (_self->output)
        return _self->output->numberOfChannels();
    return 0;
}

void AudioNode::setChannelCount(int channelCount)
{
    _self->desiredChannelCount = channelCount;
}

bool AudioNode::isConnected(std::shared_ptr<AudioNode> n) const
{
    for (int i = 0; i < _self->inputs.size(); ++i)
        if (_self->inputs[i].node == n)
            return true;
    
    return false;
}

// static
void AudioNode::connect(std::shared_ptr<AudioNode> rcv, std::shared_ptr<AudioNode> send, int inputSrcChannel, int dstChannel)
{
    // ignore if this pathway is already explicitly set up
    for (auto in : rcv->_self->inputs) {
        if (in.node == send) {
            if (in.inputChannel == inputSrcChannel && in.outputChannel == dstChannel)
                return;
        }
    }

    rcv->_self->inputs.push_back((Internal::InputMapping) { send, inputSrcChannel, dstChannel});

    // as many times as there are mappings, push back this
    send->_self->receivers.push_back(rcv);
}

void AudioNode::disconnect(std::shared_ptr<AudioNode> inputSrc, int inputSrcChannel, int dstChannel)
{
    if (!inputSrc) {
        _self->inputs.clear();
        return;
    }

    bool repeat = true;
    do {
        for (auto it = _self->inputs.begin(); it != _self->inputs.end(); ) {
            // test all the values ~ -1 signals "don't care"
            if ((*it).node == inputSrc && (*it).inputChannel == inputSrcChannel && (*it).outputChannel == dstChannel) {
                it = _self->inputs.erase(it);
                
                for (auto sit = inputSrc->_self->receivers.begin(); sit != inputSrc->_self->receivers.end(); ++sit) {
                    // knock out one subscriber to this
                    if (*sit == inputSrc) {
                        inputSrc->_self->receivers.erase(sit);
                        break;
                    }
                }
                break;
            }
            else
                ++it;
        }
    } while(repeat);
}

void AudioNode::disconnectReceivers()
{
    for (auto sit = _self->receivers.begin(); sit != _self->receivers.end(); ++sit) {
        if ((*sit).get() == this)
            sit = _self->receivers.erase(sit);
        else
            ++sit;
    }
}


std::shared_ptr<AudioBus> AudioNode::renderedOutputCurrentQuantum(ContextRenderLock& r)
{
    auto ac = r.context();
    ASSERT(ac);
    
    if (ac->currentSampleFrame() <_self->scheduler._epoch) {
        processIfNecessary(r, _self->output->length());
    }
    return _self->output;
}

bool AudioNode::updateSchedule(ContextRenderLock& r, int bufferSize)
{
    auto ac = r.context();
    ASSERT(ac);
    bool diagnosing_silence = ac->diagnosing().get() == this;

    runWorkBeforeScheduleUpdate(r);

    if (!isInitialized()) {
        if (diagnosing_silence)
            ac->diagnosed_silence("Not initialized");
        return false;
    }

    // outputs cache results in their busses.
    // if the scheduler's recorded epoch is the same as the context's, the node
    // shall bail out as it has been processed once already this epoch.

    if (!_self->scheduler.update(r, bufferSize)) {
        //if (diagnosing_silence)
        //    ac->diagnosed_silence("Already processed");
        return false;
    }

    if (diagnosing_silence) {
        if (_self->scheduler._playbackState < SchedulingState::FADE_IN ||
            _self->scheduler._playbackState > SchedulingState::PLAYING)
        {
            ac->diagnosed_silence("Scheduled node not playing");
        }
    }

    if (isScheduledNode() && 
        (_self->scheduler._playbackState < SchedulingState::FADE_IN ||
         _self->scheduler._playbackState == SchedulingState::FINISHED))
    {
        silenceOutputs(r);
        if (diagnosing_silence)
            ac->diagnosed_silence("Unscheduled");
        return false;
    }
    return true;
}

void AudioNode::processIfNecessary(ContextRenderLock& r, int bufferSize)
{
    auto ac = r.context();
    ASSERT(ac);
    bool diagnosing_silence = ac->diagnosing().get() == this;

    ProfileScope selfScope(_self->totalTime);
    _self->graphTime.zero();

    // there may need to be silence at the beginning or end of the current quantum.

    int start_zero_count = _self->scheduler._renderOffset;
    int final_zero_start = _self->scheduler._renderOffset + _self->scheduler._renderLength;
    int final_zero_count = bufferSize - final_zero_start;

    // get inputs in preparation for processing
    {
        ProfileScope scope(_self->graphTime);
        pullInputs(r, bufferSize);
        scope.finalize();   // ensure the scope is not prematurely destructed
    }
    
    //  initialize the busses with start and final zeroes.
    if (start_zero_count)
    {
        for (int i = 0; i < _self->output->numberOfChannels(); ++i)
            memset(_self->output->channel(i)->mutableData(), 0, sizeof(float) * start_zero_count);
    }

    if (final_zero_count)
    {
        for (int i = 0; i < _self->output->numberOfChannels(); ++i)
            memset(_self->output->channel(i)->mutableData() + final_zero_start, 0, sizeof(float) * final_zero_count);
    }

    // do the signal processing

    process(r, bufferSize);

    // clean pops resulting from starting or stopping

    
    const int start_envelope = ProcessingSizeInFrames / 2;
    const int end_envelope = ProcessingSizeInFrames / 2;

    
    #define OOS(x) (float(x) / float(steps))
    if (start_zero_count > 0 || _self->scheduler._playbackState == SchedulingState::FADE_IN)
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
            for (int i = 0; i < _self->output->numberOfChannels(); ++i)
            {
                float* data = _self->output->channel(i)->mutableData();
                for (int j = damp_start; j < damp_end; ++j)
                    data[j] *= OOS(j - damp_start);
            }
    }

    if (final_zero_count > 0 || _self->scheduler._playbackState == SchedulingState::STOPPING)
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
            for (int i = 0; i < _self->output->numberOfChannels(); ++i)
            {
                float* data = _self->output->channel(i)->mutableData();
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
    return _self->scheduler._playbackState < SchedulingState::FADE_IN ||
           _self->scheduler._playbackState == SchedulingState::FINISHED;
}

void AudioNode::pullInputs(ContextRenderLock & r, int bufferSize)
{
    ASSERT(r.context());

    // Process all of the AudioNodes connected to our inputs.
    for (auto & in : _self->inputs)
    {
        in.node->pullInputs(r, bufferSize);
    }
}

bool AudioNode::inputsAreSilent(ContextRenderLock & r)
{    
    for (auto & in : _self->inputs)
    {
        if (!in.node->inputsAreSilent(r))
        {
            return false;
        }
    }
    return true;
}

void AudioNode::silenceOutputs(ContextRenderLock & r)
{
    if (_self->output)
        _self->output->zero();
}

void AudioNode::unsilenceOutputs(ContextRenderLock & r)
{
    if (_self->output)
        _self->output->clearSilentFlag();
}

std::shared_ptr<AudioParam> AudioNode::param(char const * const str)
{
    for (auto & p : _self->params)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

int AudioNode::param_index(char const * const str)
{
    int count = (int) _self->params.size();
    for (int i = 0; i < count; ++i)
    {
        if (!strcmp(str, _self->params[i]->name().c_str()))
            return i;
    }
    return -1;
}

std::shared_ptr<AudioParam> AudioNode::param(int index)
{
    if (index >= _self->params.size())
        return {};

    return _self->params[index];
}

std::shared_ptr<AudioSetting> AudioNode::setting(char const * const str)
{
    for (auto & p : _self->settings)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

int AudioNode::setting_index(char const * const str)
{
    int count = (int) _self->settings.size();
    for (int i = 0; i < count; ++i)
    {
        if (!strcmp(str, _self->settings[i]->name().c_str()))
            return i;
    }
    return -1;
}

std::shared_ptr<AudioSetting> AudioNode::setting(int index)
{
    if (index >= _self->settings.size())
        return {};

    return _self->settings[index];
}

std::vector<std::string> AudioNode::paramNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _self->params)
    {
        ret.push_back(p->name());
    }
    return ret;
}
std::vector<std::string> AudioNode::paramShortNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _self->params)
    {
        ret.push_back(p->shortName());
    }
    return ret;
}

std::vector<std::string> AudioNode::settingNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _self->settings)
    {
        ret.push_back(p->name());
    }
    return ret;
}
std::vector<std::string> AudioNode::settingShortNames() const
{
    std::vector<std::string> ret;
    for (auto & p : _self->settings)
    {
        ret.push_back(p->shortName());
    }
    return ret;
}

}  // namespace lab
