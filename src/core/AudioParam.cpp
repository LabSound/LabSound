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

AudioBus const* const AudioParam::bus() const
{
    return m_internalSummingBus.get();
}


const double AudioParam::DefaultSmoothingConstant = 0.05;
const double AudioParam::SnapThreshold = 0.001;

AudioParam::AudioParam(AudioParamDescriptor const * const desc) noexcept
    : _desc(desc)
    , m_value(desc->defaultValue)
    , m_smoothedValue(desc->defaultValue)
    , m_smoothingConstant(DefaultSmoothingConstant)
{
}

AudioParam::~AudioParam() {}

float AudioParam::value() const
{
    return static_cast<float>(m_value);
}

void AudioParam::setValue(float value)
{
    if (!std::isnan(value) && !std::isinf(value))
        m_value = value;
}

float AudioParam::smoothedValue()
{
    return static_cast<float>(m_smoothedValue);
}

bool AudioParam::smooth(ContextRenderLock & r)
{
    // If values have been explicitly scheduled on the timeline, then use the exact value.
    // Smoothing effectively is performed by the timeline.
    bool useTimelineValue = false;
    if (r.context())
        m_value = m_timeline.valueForContextTime(r, static_cast<float>(m_value), useTimelineValue);

    if (m_smoothedValue == m_value)
    {
        // Smoothed value has already approached and snapped to value.
        return true;
    }

    if (useTimelineValue)
        m_smoothedValue = m_value;
    else
    {
        // Dezipper - exponential approach.
        m_smoothedValue += (m_value - m_smoothedValue) * m_smoothingConstant;

        // If we get close enough then snap to actual value.
        if (fabs(m_smoothedValue - m_value) < SnapThreshold)  // @fixme: the threshold needs to be adjustable depending on range - but this is OK general purpose value.
            m_smoothedValue = m_value;
    }

    return false;
}

float AudioParam::finalValue(ContextRenderLock & r)
{
    float value;
    calculateFinalValues(r, &value, 1, false);
    return value;
}

void AudioParam::calculateSampleAccurateValues(ContextRenderLock & r, float * values, int numberOfValues)
{
    bool isSafe = r.context() && values && numberOfValues;
    if (!isSafe)
        return;

    calculateFinalValues(r, values, numberOfValues, true);
}

void AudioParam::calculateFinalValues(ContextRenderLock & r, float * values, int numberOfValues, bool sampleAccurate)
{
    bool isSafe = r.context() && values && numberOfValues;
    if (!isSafe)
        return;

    // The calculated result will be the "intrinsic" value summed with all audio-rate connections.

    if (sampleAccurate)
    {
        // Calculate sample-accurate (a-rate) intrinsic values.
        calculateTimelineValues(r, values, numberOfValues);
    }
    else
    {
        // Calculate control-rate (k-rate) intrinsic value.
        bool hasValue;
        float timelineValue = m_timeline.valueForContextTime(r, static_cast<float>(m_value), hasValue);

        if (hasValue)
            m_value = timelineValue;

        values[0] = static_cast<float>(m_value);
    }

    // Now sum all of the audio-rate connections together (unity-gain summing junction).
    // Note that parameter connections would normally be mono, so mix down to mono if necessary.

    // LabSound: For some reason a bus was temporarily created here and the results discarded.
    // Bug still exists in WebKit top of tree.
    if (m_internalSummingBus && m_internalSummingBus->length() < numberOfValues)
        m_internalSummingBus.reset();

    if (!m_internalSummingBus)
        m_internalSummingBus.reset(new AudioBus(1, numberOfValues));

    // point the summing bus at the values array
    m_internalSummingBus->setChannelMemory(0, values, numberOfValues);

    if (_overridingInput) {
        auto output = _overridingInput->renderedOutputCurrentQuantum(r);
        m_internalSummingBus->sumFrom(*output.get(), _overridingInputChannel, 0);
    }
}

void AudioParam::calculateTimelineValues(ContextRenderLock & r, 
    float * values, int numberOfValues)
{
    // Calculate values for this render quantum.
    // Normally numberOfValues will equal AudioNode::ProcessingSizeInFrames 
    // (the render quantum size).
    double sampleRate = r.context()->sampleRate();
    double startTime = r.context()->currentTime();
    double endTime = startTime + numberOfValues / sampleRate;

    // Note we're running control rate at the sample-rate.
    // Pass in the current value as default value.
    m_value = m_timeline.valuesForTimeRange(startTime, endTime, static_cast<float>(m_value), values, numberOfValues, sampleRate, sampleRate);
}

void AudioParam::connect(ContextGraphLock & g, std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> output, int channel)
{
    if (!param || !output)
        return;

    param->_overridingInput = output;
    param->_overridingInputChannel = channel;
}

void AudioParam::disconnect(ContextGraphLock & g, std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNode> output)
{
    if (!param || !output)
        return;

    if (param->_overridingInput == output)
        param->_overridingInput.reset();
}

void AudioParam::disconnectAll(ContextGraphLock & g, std::shared_ptr<AudioParam> param)
{
    param->_overridingInput.reset();
}
