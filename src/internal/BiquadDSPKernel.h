// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef BiquadDSPKernel_h
#define BiquadDSPKernel_h

#include "internal/AudioDSPKernel.h"
#include "internal/Biquad.h"
#include "internal/BiquadProcessor.h"

namespace lab {

class BiquadProcessor;

// BiquadDSPKernel is an AudioDSPKernel and is responsible for filtering one channel of a BiquadProcessor using a Biquad object.

class BiquadDSPKernel : public AudioDSPKernel 
{
public:  

    explicit BiquadDSPKernel(BiquadProcessor* processor) : AudioDSPKernel(processor)
    {
    }
    
    // AudioDSPKernel
    virtual void process(ContextRenderLock& r, const float* source, float* dest, size_t framesToProcess) override;
    virtual void reset() override { m_biquad.reset(); }

    // Get the magnitude and phase response of the filter at the given
    // set of frequencies (in Hz). The phase response is in radians.
    void getFrequencyResponse(ContextRenderLock& r,
                              int nFrequencies,
                              const float* frequencyHz,
                              float* magResponse,
                              float* phaseResponse);

    virtual double tailTime() const override;
    virtual double latencyTime() const override;

protected:
    Biquad m_biquad;
    BiquadProcessor* biquadProcessor() { return static_cast<BiquadProcessor*>(processor()); }

    // To prevent audio glitches when parameters are changed,
    // dezippering is used to slowly change the parameters.
    // |useSmoothing| implies that we want to update using the
    // smoothed values. Otherwise the final target values are
    // used. If |forceUpdate| is true, we update the coefficients even
    // if they are not dirty. (Used when computing the frequency
    // response.)
    void updateCoefficientsIfNecessary(ContextRenderLock& r, bool useSmoothing, bool forceUpdate);
};

} // namespace lab

#endif // BiquadDSPKernel_h
