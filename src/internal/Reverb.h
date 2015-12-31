// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef Reverb_h
#define Reverb_h

#include "internal/ReverbConvolver.h"

#include <vector>

namespace lab {

class AudioBus;
    
// Multi-channel convolution reverb with channel matrixing - one or more ReverbConvolver objects are used internally.

class Reverb {
public:
    enum { MaxFrameSize = 256 };

    // renderSliceSize is a rendering hint, so the FFTs can be optimized to not all occur at the same time (very bad when rendering on a real-time thread).
    Reverb(AudioBus* impulseResponseBuffer, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads, bool normalize);

    void process(ContextRenderLock& r, const AudioBus* sourceBus, AudioBus* destinationBus, size_t framesToProcess);
    void reset();

    size_t impulseResponseLength() const { return m_impulseResponseLength; }
    size_t latencyFrames() const;

private:
    void initialize(AudioBus* impulseResponseBuffer, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads);

    size_t m_impulseResponseLength;

    std::vector<std::unique_ptr<ReverbConvolver> > m_convolvers;

    // For "True" stereo processing
    std::unique_ptr<AudioBus> m_tempBuffer;
};

} // namespace lab

#endif // Reverb_h
