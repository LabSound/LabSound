// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/AudioResamplerKernel.h"
#include "internal/AudioResampler.h"
#include "internal/AudioResampler.h"
#include "internal/Assertions.h"

#include <algorithm>

using namespace std;

namespace lab {
    
const size_t AudioResamplerKernel::MaxFramesToProcess = 128;

AudioResamplerKernel::AudioResamplerKernel(AudioResampler* resampler)
    : m_resampler(resampler)
    // The buffer size must be large enough to hold up to two extra sample frames for the linear interpolation.
    , m_sourceBuffer(2 + static_cast<int>(MaxFramesToProcess * AudioResampler::MaxRate))
    , m_virtualReadIndex(0.0)
    , m_fillIndex(0)
{
    m_lastValues[0] = 0.0f;
    m_lastValues[1] = 0.0f;
}

float* AudioResamplerKernel::getSourcePointer(size_t framesToProcess, size_t* numberOfSourceFramesNeededP)
{
    ASSERT(framesToProcess <= MaxFramesToProcess);
    
    // Calculate the next "virtual" index.  After process() is called, m_virtualReadIndex will equal this value.
    double nextFractionalIndex = m_virtualReadIndex + framesToProcess * rate();

    // Because we're linearly interpolating between the previous and next sample we need to round up so we include the next sample.
    int endIndex = static_cast<int>(nextFractionalIndex + 1.0); // round up to next integer index

    // Determine how many input frames we'll need.
    // We need to fill the buffer up to and including endIndex (so add 1) but we've already buffered m_fillIndex frames from last time.
    size_t framesNeeded = 1 + endIndex - m_fillIndex;
    if (numberOfSourceFramesNeededP)
        *numberOfSourceFramesNeededP = framesNeeded;

    // Do bounds checking for the source buffer.
    bool isGood = m_fillIndex < m_sourceBuffer.size() && m_fillIndex + framesNeeded <= m_sourceBuffer.size();
    ASSERT(isGood);
    if (!isGood)
        return 0;

    return m_sourceBuffer.data() + m_fillIndex;
}

void AudioResamplerKernel::process(ContextRenderLock&, float* destination, size_t framesToProcess)
{
    ASSERT(framesToProcess <= MaxFramesToProcess);

    float* source = m_sourceBuffer.data();
    
    double rate = this->rate();
    rate = max(0.0, rate);
    rate = min(AudioResampler::MaxRate, rate);
    
    // Start out with the previous saved values (if any).
    if (m_fillIndex > 0) {
        source[0] = m_lastValues[0];
        source[1] = m_lastValues[1];
    }

    // Make a local copy.
    double virtualReadIndex = m_virtualReadIndex;
    
    // Sanity check source buffer access.
    ASSERT(framesToProcess > 0);
    ASSERT(virtualReadIndex >= 0 && 1 + static_cast<unsigned>(virtualReadIndex + (framesToProcess - 1) * rate) < m_sourceBuffer.size());

    // Do the linear interpolation.
    int n = framesToProcess;
    while (n--) {
        unsigned readIndex = static_cast<unsigned>(virtualReadIndex);
        double interpolationFactor = virtualReadIndex - readIndex;

        double sample1 = source[readIndex];
        double sample2 = source[readIndex + 1];

        double sample = (1.0 - interpolationFactor) * sample1 + interpolationFactor * sample2;

        *destination++ = static_cast<float>(sample);

        virtualReadIndex += rate;
    }                        

    // Save the last two sample-frames which will later be used at the beginning of the source buffer the next time around.
    int readIndex = static_cast<int>(virtualReadIndex);
    m_lastValues[0] = source[readIndex];
    m_lastValues[1] = source[readIndex + 1];
    m_fillIndex = 2;

    // Wrap the virtual read index back to the start of the buffer.
    virtualReadIndex -= readIndex;

    // Put local copy back into member variable.
    m_virtualReadIndex = virtualReadIndex;
}

void AudioResamplerKernel::reset()
{
    m_virtualReadIndex = 0.0;
    m_fillIndex = 0;
    m_lastValues[0] = 0.0f;
    m_lastValues[1] = 0.0f;
}

double AudioResamplerKernel::rate() const
{
    return m_resampler->rate();
}

} // namespace lab
