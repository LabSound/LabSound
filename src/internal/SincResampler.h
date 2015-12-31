// License: BSD 3 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef SincResampler_h
#define SincResampler_h

#include "LabSound/core/AudioSourceProvider.h"
#include "LabSound/core/AudioArray.h"

namespace lab {

// SincResampler is a high-quality sample-rate converter.

class SincResampler {
public:   
    // scaleFactor == sourceSampleRate / destinationSampleRate
    // kernelSize can be adjusted for quality (higher is better)
    // numberOfKernelOffsets is used for interpolation and is the number of sub-sample kernel shifts.
    SincResampler(double scaleFactor, unsigned kernelSize = 32, unsigned numberOfKernelOffsets = 32);
    
    // Processes numberOfSourceFrames from source to produce numberOfSourceFrames / scaleFactor frames in destination.
    void process(const float* source, float* destination, unsigned numberOfSourceFrames);

    // Process with input source callback function for streaming applications.
    void process(AudioSourceProvider*, float* destination, size_t framesToProcess);

protected:
    void initializeKernel();
    void consumeSource(float* buffer, unsigned numberOfSourceFrames);
    
    double m_scaleFactor;
    unsigned m_kernelSize;
    unsigned m_numberOfKernelOffsets;

    // m_kernelStorage has m_numberOfKernelOffsets kernels back-to-back, each of size m_kernelSize.
    // The kernel offsets are sub-sample shifts of a windowed sinc() shifted from 0.0 to 1.0 sample.
    AudioFloatArray m_kernelStorage;
    
    // m_virtualSourceIndex is an index on the source input buffer with sub-sample precision.
    // It must be double precision to avoid drift.
    double m_virtualSourceIndex;
    
    // This is the number of destination frames we generate per processing pass on the buffer.
    unsigned m_blockSize;

    // Source is copied into this buffer for each processing pass.
    AudioFloatArray m_inputBuffer;

    const float* m_source;
    unsigned m_sourceFramesAvailable;
    
    // m_sourceProvider is used to provide the audio input stream to the resampler.
    AudioSourceProvider* m_sourceProvider;    

    // The buffer is primed once at the very beginning of processing.
    bool m_isBufferPrimed;
};

} // namespace lab

#endif // SincResampler_h
