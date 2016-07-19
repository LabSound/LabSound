// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioUtilities.h"
#include "internal/FloatConversion.h"
#include "internal/AudioBus.h"

#include <WTF/MathExtras.h>
#include <algorithm>

using namespace lab;

// AudioBus is not exposed in the public API
struct AudioParam::Data
{
    std::unique_ptr<AudioBus> m_internalSummingBus;
};

const double AudioParam::DefaultSmoothingConstant = 0.05;
const double AudioParam::SnapThreshold = 0.001;

AudioParam::AudioParam(const std::string& name, double defaultValue, double minValue, double maxValue, unsigned units)
: AudioSummingJunction()
, m_name(name)
, m_value(defaultValue)
, m_defaultValue(defaultValue)
, m_minValue(minValue)
, m_maxValue(maxValue)
, m_units(units)
, m_smoothedValue(defaultValue)
, m_smoothingConstant(DefaultSmoothingConstant)
, m_data(new Data())
{

}

AudioParam::~AudioParam() {}

float AudioParam::value(ContextRenderLock& r)
{
    // Update value for timeline.
    if (r.context()) {
        bool hasValue;
        float timelineValue = m_timeline.valueForContextTime(r, narrowPrecisionToFloat(m_value), hasValue);
        
        if (hasValue)
            m_value = timelineValue;
    }

    return narrowPrecisionToFloat(m_value);
}

void AudioParam::setValue(float value)
{
    if (!std::isnan(value) && !std::isinf(value))
        m_value = value;
}

float AudioParam::smoothedValue()
{
    return narrowPrecisionToFloat(m_smoothedValue);
}

bool AudioParam::smooth(ContextRenderLock& r)
{
    // If values have been explicitly scheduled on the timeline, then use the exact value.
    // Smoothing effectively is performed by the timeline.
    bool useTimelineValue = false;
    if (r.context())
        m_value = m_timeline.valueForContextTime(r, narrowPrecisionToFloat(m_value), useTimelineValue);
    
    if (m_smoothedValue == m_value) {
        // Smoothed value has already approached and snapped to value.
        return true;
    }
    
    if (useTimelineValue)
        m_smoothedValue = m_value;
    else {
        // Dezipper - exponential approach.
        m_smoothedValue += (m_value - m_smoothedValue) * m_smoothingConstant;

        // If we get close enough then snap to actual value.
        if (fabs(m_smoothedValue - m_value) < SnapThreshold) // FIXME: the threshold needs to be adjustable depending on range - but this is OK general purpose value.
            m_smoothedValue = m_value;
    }

    return false;
}

float AudioParam::finalValue(ContextRenderLock& r)
{
    float value;
    calculateFinalValues(r, &value, 1, false);
    return value;
}

void AudioParam::calculateSampleAccurateValues(ContextRenderLock& r, float* values, unsigned numberOfValues)
{
    bool isSafe = r.context() && values && numberOfValues;
    if (!isSafe)
        return;

    calculateFinalValues(r, values, numberOfValues, true);
}

void AudioParam::calculateFinalValues(ContextRenderLock& r, float* values, unsigned numberOfValues, bool sampleAccurate)
{
    bool isSafe = r.context() && values && numberOfValues;
    if (!isSafe)
        return;

    // The calculated result will be the "intrinsic" value summed with all audio-rate connections.

    if (sampleAccurate) {
        // Calculate sample-accurate (a-rate) intrinsic values.
        calculateTimelineValues(r, values, numberOfValues);
    }
    else {
        // Calculate control-rate (k-rate) intrinsic value.
        bool hasValue;
        float timelineValue = m_timeline.valueForContextTime(r, narrowPrecisionToFloat(m_value), hasValue);

        if (hasValue)
            m_value = timelineValue;

        values[0] = narrowPrecisionToFloat(m_value);
    }
    
    // if there are rendering connections, be sure they are ready
    updateRenderingState(r);
    
    size_t connectionCount = numberOfRenderingConnections(r);
    if (!connectionCount)
        return;

    // Now sum all of the audio-rate connections together (unity-gain summing junction).
    // Note that parameter connections would normally be mono, so mix down to mono if necessary.
    
    // LabSound: For some reason a bus was temporarily created here and the results discarded.
    // Bug still exists in WebKit top of tree.
    if (m_data->m_internalSummingBus && m_data->m_internalSummingBus->length() < numberOfValues)
        m_data->m_internalSummingBus.reset();
    
    if (!m_data->m_internalSummingBus)
        m_data->m_internalSummingBus.reset(new AudioBus(1, numberOfValues));

    // point the summing bus at the values array
    m_data->m_internalSummingBus->setChannelMemory(0, values, numberOfValues);

    for (size_t i = 0; i < connectionCount; ++i)
    {
        auto output = renderingOutput(r, i);
        
        ASSERT(output);
        
        // Render audio from this output.
        AudioBus* connectionBus = output->pull(r, 0, AudioNode::ProcessingSizeInFrames);

        // Sum, with unity-gain.
        m_data->m_internalSummingBus->sumFrom(*connectionBus);
    }
}

void AudioParam::calculateTimelineValues(ContextRenderLock& r, float* values, unsigned numberOfValues)
{
    // Calculate values for this render quantum.
    // Normally numberOfValues will equal AudioNode::ProcessingSizeInFrames (the render quantum size).
    double sampleRate = r.context()->sampleRate();
    double startTime = r.context()->currentTime();
    double endTime = startTime + numberOfValues / sampleRate;

    // Note we're running control rate at the sample-rate.
    // Pass in the current value as default value.
    m_value = m_timeline.valuesForTimeRange(startTime, endTime, narrowPrecisionToFloat(m_value), values, numberOfValues, sampleRate, sampleRate);
}
    
void AudioParam::connect(ContextGraphLock& g, std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNodeOutput> output)
{
    if (!output)
        return;
    
    if (param->isConnected(output))
        return;
    
    param->junctionConnectOutput(output);
    output->addParam(g, param);
}

void AudioParam::disconnect(ContextGraphLock& g, std::shared_ptr<AudioParam> param, std::shared_ptr<AudioNodeOutput> output)
{
    if (!output)
        return;
    
    if (param->isConnected(output)) {
        param->junctionDisconnectOutput(output);
    }
    output->removeParam(g, param);
}
