// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioParam_h
#define AudioParam_h

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioParamTimeline.h"
#include "LabSound/core/AudioSummingJunction.h"

#include <sys/types.h>
#include <string>

namespace lab {

class AudioNodeOutput;

class AudioParam : public AudioSummingJunction {
public:
    static const double DefaultSmoothingConstant;
    static const double SnapThreshold;

    AudioParam(const std::string& name, double defaultValue, double minValue, double maxValue, unsigned units = 0);
    virtual ~AudioParam();
    
    // AudioSummingJunction
    virtual bool canUpdateState() override { return true; }
    virtual void didUpdate(ContextRenderLock&) override { }

    // Intrinsic value.
    float value(ContextRenderLock&);
    void setValue(float);

    // Final value for k-rate parameters, otherwise use calculateSampleAccurateValues() for a-rate.
    float finalValue(ContextRenderLock&);

    std::string name() const { return m_name; }

    float minValue() const { return static_cast<float>(m_minValue); }
    float maxValue() const { return static_cast<float>(m_maxValue); }
    float defaultValue() const { return static_cast<float>(m_defaultValue); }
    unsigned units() const { return m_units; }

    // Value smoothing:

    // When a new value is set with setValue(), in our internal use of the parameter we don't immediately jump to it.
    // Instead we smoothly approach this value to avoid glitching.
    float smoothedValue();

    // Smoothly exponentially approaches to (de-zippers) the desired value.
    // Returns true if smoothed value has already snapped exactly to value.
    bool smooth(ContextRenderLock&);

    void resetSmoothedValue() { m_smoothedValue = m_value; }
    void setSmoothingConstant(double k) { m_smoothingConstant = k; }

    // Parameter automation.    
    void setValueAtTime(float value, float time) { m_timeline.setValueAtTime(value, time); }
    void linearRampToValueAtTime(float value, float time) { m_timeline.linearRampToValueAtTime(value, time); }
    void exponentialRampToValueAtTime(float value, float time) { m_timeline.exponentialRampToValueAtTime(value, time); }
    void setTargetAtTime(float target, float time, float timeConstant) { m_timeline.setTargetAtTime(target, time, timeConstant); }
    void setValueCurveAtTime(std::shared_ptr<std::vector<float>> curve, float time, float duration) { m_timeline.setValueCurveAtTime(curve, time, duration); }
    void cancelScheduledValues(float startTime) { m_timeline.cancelScheduledValues(startTime); }

    bool hasSampleAccurateValues() { return m_timeline.hasValues() || numberOfConnections(); }
    
    // Calculates numberOfValues parameter values starting at the context's current time.
    // Must be called in the context's render thread.
    void calculateSampleAccurateValues(ContextRenderLock&, float* values, unsigned numberOfValues);

    // Connect an audio-rate signal to control this parameter.
    static void connect(ContextGraphLock& g, std::shared_ptr<AudioParam>, std::shared_ptr<AudioNodeOutput>);
    static void disconnect(ContextGraphLock& g, std::shared_ptr<AudioParam>, std::shared_ptr<AudioNodeOutput>);

private:
    // sampleAccurate corresponds to a-rate (audio rate) vs. k-rate in the Web Audio specification.
    void calculateFinalValues(ContextRenderLock& r, float* values, unsigned numberOfValues, bool sampleAccurate);
    void calculateTimelineValues(ContextRenderLock& r, float* values, unsigned numberOfValues);

    std::string m_name;
    double m_value;
    double m_defaultValue;
    double m_minValue;
    double m_maxValue;
    unsigned m_units;

    // Smoothing (de-zippering)
    double m_smoothedValue;
    double m_smoothingConstant;
    
    AudioParamTimeline m_timeline;
    
    struct Data;
    std::unique_ptr<Data> m_data;
};

} // namespace lab

#endif // AudioParam_h
