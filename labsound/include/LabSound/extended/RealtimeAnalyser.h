// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef REALTIME_ANALYSER_H
#define REALTIME_ANALYSER_H

#include "LabSound/core/AudioArray.h"
#include "LabSound/extended/AudioContextLock.h"
#include <vector>

namespace lab 
{

class AudioBus;
class FFTFrame;

class RealtimeAnalyser
{

    RealtimeAnalyser(const RealtimeAnalyser&); // noncopyable

public:
    RealtimeAnalyser(uint32_t fftSize);
    virtual ~RealtimeAnalyser();
    
    void reset();

    uint32_t fftSize() const { return m_fftSize; }

    uint32_t frequencyBinCount() const { return m_fftSize / 2; }

    void setMinDecibels(double k) { m_minDecibels = k; }
    double minDecibels() const { return m_minDecibels; }

    void setMaxDecibels(double k) { m_maxDecibels = k; }
    double maxDecibels() const { return m_maxDecibels; }

    void setSmoothingTimeConstant(double k) { m_smoothingTimeConstant = k; }
    double smoothingTimeConstant() const { return m_smoothingTimeConstant; }

    void getFloatFrequencyData(std::vector<float>&);
    void getByteFrequencyData(std::vector<uint8_t>&);
    void getFloatTimeDomainData(std::vector<float>&); // LabSound
    void getByteTimeDomainData(std::vector<uint8_t>&);

    void writeInput(ContextRenderLock& r, AudioBus*, size_t framesToProcess);

    static const double DefaultSmoothingTimeConstant;
    static const double DefaultMinDecibels;
    static const double DefaultMaxDecibels;

    static const uint32_t DefaultFFTSize;
    static const uint32_t MinFFTSize;
    static const uint32_t MaxFFTSize;
    static const uint32_t InputBufferSize;

private:

    // The audio thread writes the input audio here.
    AudioFloatArray m_inputBuffer;
    unsigned m_writeIndex;
    
    uint32_t m_fftSize;
    std::unique_ptr<FFTFrame> m_analysisFrame;
    void doFFTAnalysis();
    
    // doFFTAnalysis() stores the floating-point magnitude analysis data here.
    AudioFloatArray m_magnitudeBuffer;
    AudioFloatArray& magnitudeBuffer() { return m_magnitudeBuffer; }

    // A value between 0 and 1 which averages the previous version of m_magnitudeBuffer with the current analysis magnitude data.
    double m_smoothingTimeConstant;    

    // The range used when converting when using getByteFrequencyData(). 
    double m_minDecibels;
    double m_maxDecibels;
};

} // namespace lab

#endif // RealtimeAnalyser_h
