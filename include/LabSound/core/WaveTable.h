// License: BSD 3 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef WaveTable_h
#define WaveTable_h

#include "LabSound/core/AudioArray.h"
#include "LabSound/core/Synthesis.h"

#include <vector>
#include <memory>

namespace lab
{

class WaveTable 
{

public:

    WaveTable(float sampleRate, OscillatorType basicWaveform);
    WaveTable(float sampleRate, OscillatorType basicWaveform, std::vector<float> & real, std::vector<float> & imag);

    ~WaveTable();

    // Returns pointers to the lower and higher wavetable data for the pitch range containing
    // the given fundamental frequency. These two tables are in adjacent "pitch" ranges
    // where the higher table will have the maximum number of partials which won't alias when played back
    // at this fundamental frequency. The lower wavetable is the next range containing fewer partials than the higher wavetable.
    // Interpolation between these two tables can be made according to tableInterpolationFactor.
    // Where values from 0 -> 1 interpolate between lower -> higher.
    void waveDataForFundamentalFrequency(float, float* &lowerWaveData, float* &higherWaveData, float& tableInterpolationFactor);

    // Returns the scalar multiplier to the oscillator frequency to calculate wave table phase increment.
    float rateScale() const { return m_rateScale; }

    unsigned periodicWaveSize() const;
    
    float sampleRate() const { return m_sampleRate; }

private:

    void generateBasicWaveform(OscillatorType);

    float m_sampleRate;
    unsigned m_numberOfRanges;
    float m_centsPerRange;

    // The lowest frequency (in Hertz) where playback will include all of the partials.
    // Playing back lower than this frequency will gradually lose more high-frequency information.
    // This frequency is quite low (~10Hz @ 44.1KHz)
    float m_lowestFundamentalFrequency;

    float m_rateScale;

    unsigned numberOfRanges() const { return m_numberOfRanges; }

    // Maximum possible number of partials (before culling).
    unsigned maxNumberOfPartials() const;

    unsigned numberOfPartialsForRange(unsigned rangeIndex) const;

    // Creates tables based on numberOfComponents Fourier coefficients.
    void createBandLimitedTables(const float* real, const float* imag, unsigned numberOfComponents);

    std::vector<std::unique_ptr<AudioFloatArray> > m_bandLimitedTables;
};

} // namespace lab

#endif // WaveTable_h
