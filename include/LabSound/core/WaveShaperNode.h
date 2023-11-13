
// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef WaveShaperNode_h
#define WaveShaperNode_h

#include "LabSound/core/AudioNode.h"
#include <atomic>
#include <mutex>

namespace lab {
enum OverSampleType
{
    NONE = 0,
    _2X = 1,
    _4X = 2,
    _OverSampleTypeCount
};

class WaveShaperNode : public AudioNode
{
public:
    WaveShaperNode(AudioContext & ac);
    WaveShaperNode(AudioContext & ac, AudioNodeDescriptor const&); // for subclasses
    virtual ~WaveShaperNode();

    static const char* static_name() { return "WaveShaper"; }
    virtual const char* name() const override { return static_name(); }
    static AudioNodeDescriptor * desc();

    // copies the curve
    void setCurve(std::vector<float> & curve);
    void setOversample(OverSampleType oversample) { m_oversample = oversample; }
    OverSampleType oversample() const { return m_oversample; }

    // AudioNode
    virtual void process(ContextRenderLock &, int bufferSize) override;
    virtual void reset(ContextRenderLock &) override {}

protected:
    void processCurve(const float * source, float * destination, int framesToProcess);
    virtual double tailTime(ContextRenderLock& r) const override { return 0.; }
    virtual double latencyTime(ContextRenderLock& r) const override { return 0.; }

    std::mutex _curveMutex;
    
    std::vector<float> m_curve;
    std::vector<float> m_newCurve;
    std::atomic<int> _newCurveReady{0};

    // Oversampling.
    void * m_oversamplingArrays = nullptr;
    std::atomic<OverSampleType> m_oversample{OverSampleType::NONE};

    // Use up-sampling, process at the higher sample-rate, then down-sample.
    void processCurve2x(const float * source, float * dest, int framesToProcess);
    void processCurve4x(const float * source, float * dest, int framesToProcess);
};

}  // namespace lab

#endif
