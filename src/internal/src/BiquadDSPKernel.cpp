// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/BiquadDSPKernel.h"
#include "internal/BiquadProcessor.h"
#include "internal/Assertions.h"

#include <limits.h>
#include <vector>

namespace lab {

// FIXME: As a recursive linear filter, depending on its parameters, a biquad filter can have
// an infinite tailTime. In practice, Biquad filters do not usually (except for very high resonance values) 
// have a tailTime of longer than approx. 200ms. This value could possibly be calculated based on the
// settings of the Biquad.
static const double MaxBiquadDelayTime = 0.2;

void BiquadDSPKernel::updateCoefficientsIfNecessary(ContextRenderLock& r, bool useSmoothing, bool forceUpdate)
{
    if (forceUpdate || biquadProcessor()->filterCoefficientsDirty()) {
        double value1;
        double value2;
        double gain;
        double detune; // in Cents

        if (biquadProcessor()->hasSampleAccurateValues()) {
            value1 = biquadProcessor()->parameter1()->finalValue(r);
            value2 = biquadProcessor()->parameter2()->finalValue(r);
            gain = biquadProcessor()->parameter3()->finalValue(r);
            detune = biquadProcessor()->parameter4()->finalValue(r);
        } else if (useSmoothing) {
            value1 = biquadProcessor()->parameter1()->smoothedValue();
            value2 = biquadProcessor()->parameter2()->smoothedValue();
            gain = biquadProcessor()->parameter3()->smoothedValue();
            detune = biquadProcessor()->parameter4()->smoothedValue();
        } else {
            value1 = biquadProcessor()->parameter1()->value(r);
            value2 = biquadProcessor()->parameter2()->value(r);
            gain = biquadProcessor()->parameter3()->value(r);
            detune = biquadProcessor()->parameter4()->value(r);
        }

        // Convert from Hertz to normalized frequency 0 -> 1.
        double nyquist = r.context()->sampleRate() * 0.5f;
        double normalizedFrequency = value1 / nyquist;

        // Offset frequency by detune.
        if (detune)
            normalizedFrequency *= pow(2, detune / 1200);

        // Configure the biquad with the new filter parameters for the appropriate type of filter.
        switch (biquadProcessor()->type()) {
        case FilterType::LOWPASS:
            m_biquad.setLowpassParams(normalizedFrequency, value2);
            break;

        case FilterType::HIGHPASS:
            m_biquad.setHighpassParams(normalizedFrequency, value2);
            break;

        case FilterType::BANDPASS:
            m_biquad.setBandpassParams(normalizedFrequency, value2);
            break;

        case FilterType::LOWSHELF:
            m_biquad.setLowShelfParams(normalizedFrequency, gain);
            break;

        case FilterType::HIGHSHELF:
            m_biquad.setHighShelfParams(normalizedFrequency, gain);
            break;

        case FilterType::PEAKING:
            m_biquad.setPeakingParams(normalizedFrequency, value2, gain);
            break;

        case FilterType::NOTCH:
            m_biquad.setNotchParams(normalizedFrequency, value2);
            break;

        case FilterType::ALLPASS:
            m_biquad.setAllpassParams(normalizedFrequency, value2);
            break;
        }
    }
}

void BiquadDSPKernel::process(ContextRenderLock& r, const float* source, float* destination, size_t framesToProcess)
{
    ASSERT(source && destination && biquadProcessor());
    
    // Recompute filter coefficients if any of the parameters have changed.
    // FIXME: as an optimization, implement a way that a Biquad object can simply copy its internal filter coefficients from another Biquad object.
    // Then re-factor this code to only run for the first BiquadDSPKernel of each BiquadProcessor.

    updateCoefficientsIfNecessary(r, true, false);

    m_biquad.process(source, destination, framesToProcess);
}

void BiquadDSPKernel::getFrequencyResponse(ContextRenderLock& r,
                                           size_t nFrequencies,
                                           const float* frequencyHz,
                                           float* magResponse,
                                           float* phaseResponse)
{
    bool isGood = nFrequencies > 0 && frequencyHz && magResponse && phaseResponse;
    ASSERT(isGood);
    if (!isGood)
        return;

    std::vector<float> frequency(nFrequencies);

    float nyquist = r.context()->sampleRate() * 0.5f;

    // Convert from frequency in Hz to normalized frequency (0 -> 1),
    // with 1 equal to the Nyquist frequency.
    for (size_t k = 0; k < nFrequencies; ++k)
        frequency[k] = static_cast<float>(frequencyHz[k] / nyquist);

    // We want to get the final values of the coefficients and compute
    // the response from that instead of some intermediate smoothed
    // set. Forcefully update the coefficients even if they are not
    // dirty.

    updateCoefficientsIfNecessary(r, false, true);

    m_biquad.getFrequencyResponse(nFrequencies, &frequency[0], magResponse, phaseResponse);
}

double BiquadDSPKernel::tailTime(ContextRenderLock & r) const
{
    return MaxBiquadDelayTime;
}

double BiquadDSPKernel::latencyTime(ContextRenderLock & r) const
{
    return 0;
}

} // namespace lab
