// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef BiquadProcessor_h
#define BiquadProcessor_h

#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioBus.h"

#include "internal/Biquad.h"
#include "internal/AudioDSPKernel.h"
#include "internal/AudioDSPKernelProcessor.h"

namespace lab {

// BiquadProcessor is an AudioDSPKernelProcessor which uses Biquad objects to implement several common filters.

class BiquadProcessor : public AudioDSPKernelProcessor {
public:

    BiquadProcessor(size_t numberOfChannels, bool autoInitialize);

    virtual ~BiquadProcessor();
    
    virtual AudioDSPKernel* createKernel();
        
    virtual void process(ContextRenderLock&, const AudioBus* source, AudioBus* destination, size_t framesToProcess);

    // Get the magnitude and phase response of the filter at the given
    // set of frequencies (in Hz). The phase response is in radians.
    void getFrequencyResponse(ContextRenderLock&,
                              size_t nFrequencies,
                              const float* frequencyHz,
                              float* magResponse,
                              float* phaseResponse);

    void checkForDirtyCoefficients(ContextRenderLock&);
    
    bool filterCoefficientsDirty() const { return m_filterCoefficientsDirty; }
    bool hasSampleAccurateValues() const { return m_hasSampleAccurateValues; }

    std::shared_ptr<AudioParam> parameter1() { return m_parameter1; }
    std::shared_ptr<AudioParam> parameter2() { return m_parameter2; }
    std::shared_ptr<AudioParam> parameter3() { return m_parameter3; }
    std::shared_ptr<AudioParam> parameter4() { return m_parameter4; }

    FilterType type() const { return m_type; }
    void setType(FilterType);

private:
    FilterType m_type;

    std::shared_ptr<AudioParam> m_parameter1;
    std::shared_ptr<AudioParam> m_parameter2;
    std::shared_ptr<AudioParam> m_parameter3;
    std::shared_ptr<AudioParam> m_parameter4;

    // so DSP kernels know when to re-compute coefficients
    bool m_filterCoefficientsDirty;

    // Set to true if any of the filter parameters are sample-accurate.
    bool m_hasSampleAccurateValues;
};

} // namespace lab

#endif // BiquadProcessor_h
