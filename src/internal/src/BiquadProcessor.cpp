// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/BiquadProcessor.h"
#include "internal/BiquadDSPKernel.h"

namespace lab {
    
BiquadProcessor::BiquadProcessor(float sampleRate, size_t numberOfChannels, bool autoInitialize)
    : AudioDSPKernelProcessor(sampleRate, numberOfChannels)
    , m_type(LowPass)
    , m_filterCoefficientsDirty(true)
    , m_hasSampleAccurateValues(false)
{
    double nyquist = 0.5 * this->sampleRate();

    // Create parameters for BiquadFilterNode.
    m_parameter1 = std::make_shared<AudioParam>("frequency", 350.0, 10.0, nyquist);
    m_parameter2 = std::make_shared<AudioParam>("Q", 1, 0.0001, 1000.0);
    m_parameter3 = std::make_shared<AudioParam>("gain", 0.0, -40, 40);
    m_parameter4 = std::make_shared<AudioParam>("detune", 0.0, -4800, 4800);

    if (autoInitialize)
        initialize();
}

BiquadProcessor::~BiquadProcessor()
{
    if (isInitialized())
        uninitialize();
}

AudioDSPKernel* BiquadProcessor::createKernel()
{
    return new BiquadDSPKernel(this);
}

void BiquadProcessor::checkForDirtyCoefficients(ContextRenderLock& r)
{
    // Deal with smoothing / de-zippering. Start out assuming filter parameters are not changing.

    // The BiquadDSPKernel objects rely on this value to see if they need to re-compute their internal filter coefficients.
    m_filterCoefficientsDirty = false;
    m_hasSampleAccurateValues = false;
    
    if (m_parameter1->hasSampleAccurateValues() || m_parameter2->hasSampleAccurateValues() || m_parameter3->hasSampleAccurateValues() || m_parameter4->hasSampleAccurateValues()) {
        m_filterCoefficientsDirty = true;
        m_hasSampleAccurateValues = true;
    } else {
        if (m_hasJustReset) {
            // Snap to exact values first time after reset, then smooth for subsequent changes.
            m_parameter1->resetSmoothedValue();
            m_parameter2->resetSmoothedValue();
            m_parameter3->resetSmoothedValue();
            m_parameter4->resetSmoothedValue();
            m_filterCoefficientsDirty = true;
            m_hasJustReset = false;
        } else {
            // Smooth all of the filter parameters. If they haven't yet converged to their target value then mark coefficients as dirty.
            bool isStable1 = m_parameter1->smooth(r);
            bool isStable2 = m_parameter2->smooth(r);
            bool isStable3 = m_parameter3->smooth(r);
            bool isStable4 = m_parameter4->smooth(r);
            if (!(isStable1 && isStable2 && isStable3 && isStable4))
                m_filterCoefficientsDirty = true;
        }
    }
}

void BiquadProcessor::process(ContextRenderLock& r, const AudioBus* source, AudioBus* destination, size_t framesToProcess)
{
    if (!isInitialized()) {
        destination->zero();
        return;
    }
        
    checkForDirtyCoefficients(r);
            
    // For each channel of our input, process using the corresponding BiquadDSPKernel into the output channel.
    for (unsigned i = 0; i < m_kernels.size(); ++i) {
        m_kernels[i]->process(r, source->channel(i)->data(), destination->channel(i)->mutableData(), framesToProcess);
    }
}

void BiquadProcessor::setType(FilterType type)
{
    if (type != m_type) {
        m_type = type;
        reset(); // The filter state must be reset only if the type has changed.
    }
}

void BiquadProcessor::getFrequencyResponse(ContextRenderLock& r,
                                           int nFrequencies,
                                           const float* frequencyHz,
                                           float* magResponse,
                                           float* phaseResponse)
{
    // Compute the frequency response on a separate temporary kernel
    // to avoid interfering with the processing running in the audio
    // thread on the main kernels.
    
    std::unique_ptr<BiquadDSPKernel> responseKernel(new BiquadDSPKernel(this));

    responseKernel->getFrequencyResponse(r, nFrequencies, frequencyHz, magResponse, phaseResponse);
}

} // namespace lab
