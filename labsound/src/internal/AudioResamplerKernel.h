// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioResamplerKernel_h
#define AudioResamplerKernel_h

#include "LabSound/core/AudioArray.h"

namespace lab {

class AudioResampler;
class ContextRenderLock;
    
// AudioResamplerKernel does resampling on a single mono channel.
// It uses a simple linear interpolation for good performance.

class AudioResamplerKernel {
public:
    AudioResamplerKernel(AudioResampler*);

    // getSourcePointer() should be called each time before process() is called.
    // Given a number of frames to process (for subsequent call to process()), it returns a pointer and numberOfSourceFramesNeeded
    // where sample data should be copied. This sample data provides the input to the resampler when process() is called.
    // framesToProcess must be less than or equal to MaxFramesToProcess.
    float* getSourcePointer(size_t framesToProcess, size_t* numberOfSourceFramesNeeded);

    // process() resamples framesToProcess frames from the source into destination.
    // Each call to process() must be preceded by a call to getSourcePointer() so that source input may be supplied.
    // framesToProcess must be less than or equal to MaxFramesToProcess.
    void process(ContextRenderLock&, float* destination, size_t framesToProcess);

    // Resets the processing state.
    void reset();

    static const size_t MaxFramesToProcess;

private:
    double rate() const;

    AudioResampler* m_resampler;
    AudioFloatArray m_sourceBuffer;
    
    // This is a (floating point) read index on the input stream.
    double m_virtualReadIndex;

    // We need to have continuity from one call of process() to the next.
    // m_lastValues stores the last two sample values from the last call to process().
    // m_fillIndex represents how many buffered samples we have which can be as many as 2.
    // For the first call to process() (or after reset()) there will be no buffered samples.
    float m_lastValues[2];
    unsigned m_fillIndex;
};

} // namespace lab

#endif // AudioResamplerKernel_h
