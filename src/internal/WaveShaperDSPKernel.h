// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef WaveShaperDSPKernel_h
#define WaveShaperDSPKernel_h

#include "internal/AudioDSPKernel.h"
#include "internal/WaveShaperProcessor.h"

namespace lab {

class WaveShaperProcessor;

// WaveShaperDSPKernel is an AudioDSPKernel and is responsible for non-linear distortion on one channel.

class WaveShaperDSPKernel : public AudioDSPKernel {
public:  
    explicit WaveShaperDSPKernel(WaveShaperProcessor* processor)
    : AudioDSPKernel(processor)
    {
    }
    
    // AudioDSPKernel
    virtual void process(ContextRenderLock&,
                         const float* source, float* dest, size_t framesToProcess) override;
    virtual void reset() override { }
    virtual double tailTime() const override { return 0; }
    virtual double latencyTime() const override { return 0; }
    
protected:
    WaveShaperProcessor* waveShaperProcessor() { return static_cast<WaveShaperProcessor*>(processor()); }
};

} // namespace lab

#endif // WaveShaperDSPKernel_h
