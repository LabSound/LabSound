// License: BSD 3 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef SincResampler_h
#define SincResampler_h

#include "LabSound/core/AudioArray.h"
#include "LabSound/core/AudioSourceProvider.h"

namespace lab
{
class SincResampler
{
public:
    // scaleFactor == sourceSampleRate / destinationSampleRate
    // kernelSize can be adjusted for quality (higher is better)
    // numberOfKernelOffsets is used for interpolation and is the number of sub-sample kernel shifts.
    SincResampler(double scaleFactor, size_t kernelSize = 32, size_t numberOfKernelOffsets = 32);

    // Processes numberOfSourceFrames from source to produce numberOfSourceFrames / scaleFactor frames in destination.
    void process(const float * source, float * destination, size_t numberOfSourceFrames);

    // Process with input source callback function for streaming applications.
    void process(AudioSourceProvider *, float * destination, size_t framesToProcess);

protected:
    void initializeKernel();
    void consumeSource(float * buffer, size_t numberOfSourceFrames);

    double m_scaleFactor;
    size_t m_kernelSize;
    size_t m_numberOfKernelOffsets;

    // m_kernelStorage has m_numberOfKernelOffsets kernels back-to-back, each of size m_kernelSize.
    // The kernel offsets are sub-sample shifts of a windowed sinc() shifted from 0.0 to 1.0 sample.
    AudioFloatArray m_kernelStorage;

    // m_virtualSourceIndex is an index on the source input buffer with sub-sample precision.
    // It must be double precision to avoid drift.
    double m_virtualSourceIndex{0};

    // This is the number of destination frames we generate per processing pass on the buffer.
    size_t m_blockSize{512};

    // Source is copied into this buffer for each processing pass.
    AudioFloatArray m_inputBuffer;

    const float * m_source{nullptr};
    size_t m_sourceFramesAvailable{0};

    // m_sourceProvider is used to provide the audio input stream to the resampler.
    AudioSourceProvider * m_sourceProvider{nullptr};

    // The buffer is primed once at the very beginning of processing.
    bool m_isBufferPrimed{false};
};

}  // namespace lab

#endif  // SincResampler_h
