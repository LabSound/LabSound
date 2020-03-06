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
    SincResampler(double scaleFactor, int kernelSize = 32, int numberOfKernelOffsets = 32);

    // Processes numberOfSourceFrames from source to produce numberOfSourceFrames / scaleFactor frames in destination.
    void process(const float * source, float * destination, int numberOfSourceFrames);

    // Process with input source callback function for streaming applications.
    void process(AudioSourceProvider *, float * destination, int framesToProcess);

protected:
    void initializeKernel();
    void consumeSource(float * buffer, int numberOfSourceFrames);

    double m_scaleFactor;
    int m_kernelSize;
    int m_numberOfKernelOffsets;

    // m_kernelStorage has m_numberOfKernelOffsets kernels back-to-back, each of size m_kernelSize.
    // The kernel offsets are sub-sample shifts of a windowed sinc() shifted from 0.0 to 1.0 sample.
    AudioFloatArray m_kernelStorage;

    // m_virtualSourceIndex is an index on the source input buffer with sub-sample precision.
    // It must be double precision to avoid drift.
    double m_virtualSourceIndex{0};

    // This is the number of destination frames we generate per processing pass on the buffer.
    int m_blockSize{512};

        // m_virtualSourceIndex is an index on the source input buffer with sub-sample precision.
        // It must be double precision to avoid drift.
        double m_virtualSourceIndex {0};

    const float * m_source{nullptr};
    int m_sourceFramesAvailable{0};

    // m_sourceProvider is used to provide the audio input stream to the resampler.
    AudioSourceProvider * m_sourceProvider{nullptr};

    // The buffer is primed once at the very beginning of processing.
    bool m_isBufferPrimed{false};
};

}  // namespace lab

#endif  // SincResampler_h

        // The buffer is primed once at the very beginning of processing.
        bool m_isBufferPrimed {false};
    };

}  // namespace lab

#endif  // SincResampler_h
