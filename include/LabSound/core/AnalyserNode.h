// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AnalyserNode_h
#define AnalyserNode_h

#include "LabSound/core/AudioBasicInspectorNode.h"

namespace lab {

class AudioSetting;

// params:
// settings: fftSize, minDecibels, maxDecibels, smoothingTimeConstant
//
class AnalyserNode : public AudioBasicInspectorNode 
{
public:

    AnalyserNode(); // defaults to 1024
    AnalyserNode(size_t fftSize);
    virtual ~AnalyserNode();
    
    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;

    void setFftSize(ContextRenderLock&, size_t fftSize);
    size_t fftSize() const;

    size_t frequencyBinCount() const;

    void setMinDecibels(double k);
    double minDecibels() const;

    void setMaxDecibels(double k);
    double maxDecibels() const;

    void setSmoothingTimeConstant(double k);
    double smoothingTimeConstant() const;

    // ffi: user facing functions
    void getFloatFrequencyData(std::vector<float>& array);
    void getByteFrequencyData(std::vector<uint8_t>& array);
    void getFloatTimeDomainData(std::vector<float>& array);
    void getByteTimeDomainData(std::vector<uint8_t>& array);

private:
    void shared_construction(size_t fftSize);

    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    struct Detail;
    Detail* _detail = nullptr;
};

} // namespace lab

#endif // AnalyserNode_h
