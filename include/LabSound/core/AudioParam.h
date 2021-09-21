// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioParam_h
#define AudioParam_h

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioParamTimeline.h"
#include "LabSound/core/AudioSummingJunction.h"

#include <string>
#include <sys/types.h>

namespace lab
{

class AudioNodeOutput;

struct AudioParamDescriptor
{
    char const * const name;
    char const * const shortName;
    double defaultValue, minValue, maxValue;
};


class AudioParam : public AudioSummingJunction
{
public:
    static const double DefaultSmoothingConstant;
    static const double SnapThreshold;

    AudioParam(AudioParamDescriptor const*const);
    virtual ~AudioParam();

    // Intrinsic value.
    float value() const;
    void setValue(float);

    // Final value for k-rate parameters, otherwise use calculateSampleAccurateValues() for a-rate.
    float finalValue(ContextRenderLock &);

    std::string name() const { return _desc->name; }
    std::string shortName() const { return _desc->shortName; }

    double minValue() const { return _desc->minValue; }
    double maxValue() const { return _desc->maxValue; }
    double defaultValue() const { return _desc->defaultValue; }

    // Value smoothing:

    // When a new value is set with setValue(), in our internal use of the parameter we don't immediately jump to it.
    // Instead we smoothly approach this value to avoid glitching.
    float smoothedValue();

    // Smoothly exponentially approaches to (de-zippers) the desired value.
    // Returns true if smoothed value has already snapped exactly to value.
    bool smooth(ContextRenderLock &);

    void resetSmoothedValue() { m_smoothedValue = m_value; }
    void setSmoothingConstant(double k) { m_smoothingConstant = k; }

    // Parameter automation. 
    // Time is a double representing the time (in seconds) after the AudioContext was first created that the change in value will happen
    // Returns a reference for chaining calls.
    AudioParam & setValueAtTime(float value, float time) { m_timeline.setValueAtTime(value, time); return *this; }
    AudioParam & linearRampToValueAtTime(float value, float time) { m_timeline.linearRampToValueAtTime(value, time); return *this; }
    AudioParam & exponentialRampToValueAtTime(float value, float time) { m_timeline.exponentialRampToValueAtTime(value, time); return *this; }
    AudioParam & setTargetAtTime(float target, float time, float timeConstant) { m_timeline.setTargetAtTime(target, time, timeConstant); return *this; }
    AudioParam & setValueCurveAtTime(std::vector<float> curve, float time, float duration) { m_timeline.setValueCurveAtTime(curve, time, duration); return *this; }
    AudioParam & cancelScheduledValues(float startTime) { m_timeline.cancelScheduledValues(startTime); return *this; }

    bool hasSampleAccurateValues() { return m_timeline.hasValues() || numberOfConnections(); }

    // Calculates numberOfValues parameter values starting at the context's current time.
    // Must be called in the context's render thread.
    void calculateSampleAccurateValues(ContextRenderLock &, float * values, int numberOfValues);

    AudioBus const* const bus() const;

    // Connect an audio-rate signal to control this parameter.
    static void connect(ContextGraphLock & g, std::shared_ptr<AudioParam>, std::shared_ptr<AudioNodeOutput>);
    static void disconnect(ContextGraphLock & g, std::shared_ptr<AudioParam>, std::shared_ptr<AudioNodeOutput>);
    static void disconnectAll(ContextGraphLock & g, std::shared_ptr<AudioParam>);

private:
    // sampleAccurate corresponds to a-rate (audio rate) vs. k-rate in the Web Audio specification.
    void calculateFinalValues(ContextRenderLock & r, float * values, int numberOfValues, bool sampleAccurate);
    void calculateTimelineValues(ContextRenderLock & r, float * values, int numberOfValues);

    // Smoothing (de-zippering)
    double m_smoothedValue;
    double m_smoothingConstant;
    double m_value;

    AudioParamTimeline m_timeline;

    std::unique_ptr<AudioBus> m_internalSummingBus;

    AudioParamDescriptor const*const _desc;
};

}  // namespace lab

#endif  // AudioParam_h
