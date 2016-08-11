// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/RealtimeAnalyser.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioBus.h"
#include "internal/AudioUtilities.h"
#include "internal/FFTFrame.h"
#include "internal/VectorMath.h"

#include <WTF/MathExtras.h>

#include <algorithm>
#include <limits.h>
#include <complex>

using namespace std;

namespace lab {

const double RealtimeAnalyser::DefaultSmoothingTimeConstant  = 0.8;
const double RealtimeAnalyser::DefaultMinDecibels = -100;
const double RealtimeAnalyser::DefaultMaxDecibels = -30;

// All FFT implementations are expected to handle power-of-two sizes MinFFTSize <= size <= MaxFFTSize.
const uint32_t RealtimeAnalyser::DefaultFFTSize = 2048;
const uint32_t RealtimeAnalyser::MinFFTSize = 32;
const uint32_t RealtimeAnalyser::MaxFFTSize = 2048;
const uint32_t RealtimeAnalyser::InputBufferSize = RealtimeAnalyser::MaxFFTSize * 2;

inline uint32_t RoundNextPow2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
    
inline void ApplyBlackmanWindow(float * p, uint32_t n)
{
    // Blackman window
    double alpha = 0.16;
    double a0 = 0.5 * (1 - alpha);
    double a1 = 0.5;
    double a2 = 0.5 * alpha;
    
    for (uint32_t i = 0; i < n; ++i) 
    {
        double x = static_cast<double>(i) / static_cast<double>(n);
        double window = a0 - a1 * cos(2 * piDouble * x) + a2 * cos(4 * piDouble * x);
        p[i] *= float(window);
    }

}

RealtimeAnalyser::RealtimeAnalyser(uint32_t fftSize)
    : m_inputBuffer(InputBufferSize)
    , m_writeIndex(0)
    , m_smoothingTimeConstant(DefaultSmoothingTimeConstant)
    , m_minDecibels(DefaultMinDecibels)
    , m_maxDecibels(DefaultMaxDecibels)
{
    uint32_t size = max(min(RoundNextPow2(fftSize), MaxFFTSize), MinFFTSize);
    m_fftSize = size;
    
    m_analysisFrame = std::unique_ptr<FFTFrame>(new FFTFrame(size));
    
    // m_magnitudeBuffer has size = fftSize / 2 because it contains floats reduced from complex values in m_analysisFrame.
    m_magnitudeBuffer.allocate(size / 2);
}

RealtimeAnalyser::~RealtimeAnalyser()
{

}

void RealtimeAnalyser::reset()
{
    m_writeIndex = 0;
    m_inputBuffer.zero();
    m_magnitudeBuffer.zero();
}

void RealtimeAnalyser::writeInput(ContextRenderLock &r, AudioBus* bus, size_t framesToProcess)
{
    bool isBusGood = bus && bus->numberOfChannels() > 0 && bus->channel(0)->length() >= framesToProcess && r.context();
    if (!isBusGood)
        return;
        
    // FIXME : allow to work with non-FFTSize divisible chunking
    bool isDestinationGood = m_writeIndex < m_inputBuffer.size() && m_writeIndex + framesToProcess <= m_inputBuffer.size();
    ASSERT(isDestinationGood);
    if (!isDestinationGood)
        return;    
    
    // Perform real-time analysis
    const float* source = bus->channel(0)->data();
    float* dest = m_inputBuffer.data() + m_writeIndex;

    // The source has already been sanity checked with isBusGood above.
    memcpy(dest, source, sizeof(float) * framesToProcess);

    // Sum all channels in one if numberOfChannels > 1.
    unsigned numberOfChannels = bus->numberOfChannels();
    if (numberOfChannels > 1) {
        for (unsigned i = 1; i < numberOfChannels; i++) {
            source = bus->channel(i)->data();
            VectorMath::vadd(dest, 1, source, 1, dest, 1, framesToProcess);
        }
        const float scale =  1.0 / numberOfChannels;
        VectorMath::vsmul(dest, 1, &scale, dest, 1, framesToProcess);
    }

    m_writeIndex += framesToProcess;
    if (m_writeIndex >= InputBufferSize)
        m_writeIndex = 0;
}

void RealtimeAnalyser::doFFTAnalysis()
{    
    // Unroll the input buffer into a temporary buffer, where we'll apply an analysis window followed by an FFT.
    uint32_t fftSize = this->fftSize();
    
    AudioFloatArray temporaryBuffer(fftSize);
    float* inputBuffer = m_inputBuffer.data();
    float* tempP = temporaryBuffer.data();

    // Take the previous fftSize values from the input buffer and copy into the temporary buffer.
    unsigned writeIndex = m_writeIndex;
    if (writeIndex < fftSize) 
    {
        memcpy(tempP, inputBuffer + writeIndex - fftSize + InputBufferSize, sizeof(*tempP) * (fftSize - writeIndex));
        memcpy(tempP + fftSize - writeIndex, inputBuffer, sizeof(*tempP) * writeIndex);
    }
    else
    {
        memcpy(tempP, inputBuffer + writeIndex - fftSize, sizeof(*tempP) * fftSize);
    }

    // Window the input samples.
    ApplyBlackmanWindow(tempP, fftSize);
    
    // Do the analysis.
    m_analysisFrame->doFFT(tempP);

    float* realP = m_analysisFrame->realData();
    float* imagP = m_analysisFrame->imagData();

    // Blow away the packed nyquist component.
    imagP[0] = 0;
    
    // Normalize so than an input sine wave at 0dBfs registers as 0dBfs (undo FFT scaling factor).
    const double magnitudeScale = 1.0 / DefaultFFTSize;

    // A value of 0 does no averaging with the previous result.  Larger values produce slower, but smoother changes.
    double k = m_smoothingTimeConstant;
    k = max(0.0, k);
    k = min(1.0, k);    
    
    // Convert the analysis data from complex to magnitude and average with the previous result.
    float* destination = magnitudeBuffer().data();
    size_t n = magnitudeBuffer().size();
    for (size_t i = 0; i < n; ++i) {
        std::complex<double> c(realP[i], imagP[i]);
        double scalarMagnitude = abs(c) * magnitudeScale;        
        destination[i] = float(k * destination[i] + (1 - k) * scalarMagnitude);
    }
}

void RealtimeAnalyser::getFloatFrequencyData(std::vector<float>& destinationArray)
{
    if (!destinationArray.size())
        return;
        
    doFFTAnalysis();
    
    // Convert from linear magnitude to floating-point decibels.
    const double minDecibels = m_minDecibels;
    size_t sourceLength = magnitudeBuffer().size();
    size_t len = min(sourceLength, destinationArray.size());
    if (len > 0) {
        const float* source = magnitudeBuffer().data();
        for (unsigned i = 0; i < len; ++i) {
            float linearValue = source[i];
            double dbMag = !linearValue ? minDecibels : AudioUtilities::linearToDecibels(linearValue);
            destinationArray[i] = float(dbMag);
        }
    }
}

void RealtimeAnalyser::getByteFrequencyData(std::vector<uint8_t>& destinationArray)
{
    if (!destinationArray.size())
        return;
        
    doFFTAnalysis();
    
    // Convert from linear magnitude to unsigned-byte decibels.
    size_t sourceLength = magnitudeBuffer().size();
    size_t len = min(sourceLength, destinationArray.size());
    if (len > 0) {
        const double rangeScaleFactor = m_maxDecibels == m_minDecibels ? 1 : 1 / (m_maxDecibels - m_minDecibels);
        const double minDecibels = m_minDecibels;

        const float* source = magnitudeBuffer().data();
        for (unsigned i = 0; i < len; ++i) {
            float linearValue = source[i];
            double dbMag = !linearValue ? minDecibels : AudioUtilities::linearToDecibels(linearValue);
            
            // The range m_minDecibels to m_maxDecibels will be scaled to byte values from 0 to UCHAR_MAX.
            double scaledValue = UCHAR_MAX * (dbMag - minDecibels) * rangeScaleFactor;

            // Clip to valid range.
            if (scaledValue < 0)
                scaledValue = 0;
            if (scaledValue > UCHAR_MAX)
                scaledValue = UCHAR_MAX;
            
            destinationArray[i] = static_cast<unsigned char>(scaledValue);
        }
    }
}

// LabSound begin
void RealtimeAnalyser::getFloatTimeDomainData(std::vector<float>& destinationArray)
{
    if (!destinationArray.size())
        return;
    
    size_t fftSize = this->fftSize();
    size_t len = min(fftSize, destinationArray.size());
    if (len > 0) {
        bool isInputBufferGood = m_inputBuffer.size() == InputBufferSize && m_inputBuffer.size() > fftSize;
        ASSERT(isInputBufferGood);
        if (!isInputBufferGood)
            return;
        
        float* inputBuffer = m_inputBuffer.data();
        unsigned writeIndex = m_writeIndex;
        
        for (unsigned i = 0; i < len; ++i)
            // Buffer access is protected due to modulo operation.
            destinationArray[i] = inputBuffer[(i + writeIndex - fftSize + InputBufferSize) % InputBufferSize];
    }
}
// LabSound end

void RealtimeAnalyser::getByteTimeDomainData(std::vector<uint8_t>& destinationArray)
{
    if (!destinationArray.size())
        return;
        
    size_t fftSize = this->fftSize();
    size_t len = min(fftSize, destinationArray.size());
    if (len > 0) {
        bool isInputBufferGood = m_inputBuffer.size() == InputBufferSize && m_inputBuffer.size() > fftSize;
        ASSERT(isInputBufferGood);
        if (!isInputBufferGood)
            return;

        float* inputBuffer = m_inputBuffer.data();
        unsigned writeIndex = m_writeIndex;

        for (unsigned i = 0; i < len; ++i) {
            // Buffer access is protected due to modulo operation.
            float value = inputBuffer[(i + writeIndex - fftSize + InputBufferSize) % InputBufferSize];

            // Scale from nominal -1 -> +1 to unsigned byte.
            double scaledValue = 128 * (value + 1);

            // Clip to valid range.
            if (scaledValue < 0)
                scaledValue = 0;
            if (scaledValue > UCHAR_MAX)
                scaledValue = UCHAR_MAX;
            
            destinationArray[i] = static_cast<unsigned char>(scaledValue);
        }
    }
}

} // namespace lab

