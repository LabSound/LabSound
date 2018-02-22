// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ReverbConvolverStage_h
#define ReverbConvolverStage_h

#include "LabSound/core/AudioArray.h"

#include "internal/FFTFrame.h"

namespace lab {

class ReverbAccumulationBuffer;
class ReverbConvolver;
class FFTConvolver;
class DirectConvolver;
    
// A ReverbConvolverStage represents the convolution associated with a sub-section of a large impulse response.
// It incorporates a delay line to account for the offset of the sub-section within the larger impulse response.
class ReverbConvolverStage {
public:
    // renderPhase is useful to know so that we can manipulate the pre versus post delay so that stages will perform
    // their heavy work (FFT processing) on different slices to balance the load in a real-time thread.
    ReverbConvolverStage(const float* impulseResponse, size_t responseLength, size_t reverbTotalLatency, size_t stageOffset, size_t stageLength, size_t fftSize, size_t renderPhase, size_t renderSliceSize, ReverbAccumulationBuffer*, bool directMode = false);

    // WARNING: framesToProcess must be such that it evenly divides the delay buffer size (stage_offset).
    void process(const float* source, size_t framesToProcess);

    void processInBackground(ReverbConvolver* convolver, size_t framesToProcess);

    void reset();

    // Useful for background processing
    int inputReadIndex() const { return m_inputReadIndex; }

private:
    std::unique_ptr<FFTFrame> m_fftKernel;
    std::unique_ptr<FFTConvolver> m_fftConvolver;

    AudioFloatArray m_preDelayBuffer;

    ReverbAccumulationBuffer* m_accumulationBuffer;
    int m_accumulationReadIndex;
    int m_inputReadIndex;

    size_t m_preDelayLength;
    size_t m_postDelayLength;
    size_t m_preReadWriteIndex;
    size_t m_framesProcessed;

    AudioFloatArray m_temporaryBuffer;

    bool m_directMode;
    std::unique_ptr<AudioFloatArray> m_directKernel;
    std::unique_ptr<DirectConvolver> m_directConvolver;
};

} // namespace lab

#endif // ReverbConvolverStage_h
