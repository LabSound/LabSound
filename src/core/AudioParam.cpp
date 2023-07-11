// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/Macros.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"

#include <algorithm>

using namespace lab;

std::shared_ptr<AudioBus> AudioParam::bus() const
{
    if (_overridingInput)
        return _overridingInput->output();
    return _bus;
}

AudioParam::AudioParam(AudioParamDescriptor const * const desc) noexcept
: _descriptor(desc)
, _intrinsicValue(desc->defaultValue)
, _overridingChannel(0)
{
    _bus = std::shared_ptr<AudioBus>(new AudioBus(1, AudioNode::ProcessingSizeInFrames));
}

bool AudioParam::isConstant(float* value) const {
    auto b = bus();
    if (!b) {
        if (value) *value = 0.f;
        return true;
    }
    AudioChannel* c = b->channel(_overridingChannel);
    if (!c) {
        if (value) *value = 0.f;
        return true;
    }
    const float* d = c->data();
    if (!d) {
        if (value) *value = 0.f;
        return true;
    }
    if (value)
        *value = d[0];
    
    // if there's an overriding input, the parameter is not constant.
    // if there's no knots, the _intrinsincValue is used
    // if there's one knot, it must be Held
    // if there's more than one, the param is not constant.
    return !_overridingInput || _knots.size() > 1;
}

void AudioParam::process(ContextRenderLock &, int bufferSize) {
    if (_overridingInput) {
        // nothing to do
        return;
    }
    if (_bus->length() < bufferSize) {
        _bus = std::shared_ptr<AudioBus>(new AudioBus(1, bufferSize));
    }
    AudioChannel* c = _bus->channel(0);
    float* d = c->mutableData();
    for (int i = 0; i < bufferSize; ++i)
        d[i] = _intrinsicValue;
}

void AudioParam::connect(ContextGraphLock & g,
                         std::shared_ptr<AudioParam> param,
                         std::shared_ptr<AudioNode> output, int channel)
{
    if (!param || !output)
        return;

    auto b = output->output();
    if (!b || b->numberOfChannels() <= channel)
        return;
    
    param->_overridingInput = output;
    param->_overridingChannel = channel;
}

void AudioParam::disconnect(ContextGraphLock & g,
                            std::shared_ptr<AudioParam> param,
                            std::shared_ptr<AudioNode> output)
{
    if (!param || !output)
        return;

    if (param->_overridingInput == output) {
        param->_overridingInput.reset();
        param->_overridingChannel = 0;
    }
}

void AudioParam::disconnectAll(ContextGraphLock & g, std::shared_ptr<AudioParam> param)
{
    param->_overridingInput.reset();
    param->_overridingChannel = 0;
}

void AudioParam::setValueAtTime(float value, float t) {
    if (value <= 0.f) {
        _knots.clear();
        _intrinsicValue = value;
    }
    else {
        ASSERT(false); /// @TODO
    }
}
