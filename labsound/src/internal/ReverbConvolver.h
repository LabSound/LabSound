// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ReverbConvolver_h
#define ReverbConvolver_h

#include "LabSound/core/AudioArray.h"

#include "internal/DirectConvolver.h"
#include "internal/FFTConvolver.h"
#include "internal/ReverbAccumulationBuffer.h"
#include "internal/ReverbConvolverStage.h"
#include "internal/ReverbInputBuffer.h"

#include <mutex>
#include <vector>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace lab {

class ContextRenderLock;
class AudioChannel;

class ReverbConvolver {
    
public:
    
    // maxFFTSize can be adjusted (from say 2048 to 32768) depending on how much precision is necessary.
    // For certain tweaky de-convolving applications the phase errors add up quickly and lead to non-sensical results with
    // larger FFT sizes and single-precision floats.  In these cases 2048 is a good size.
    // If not doing multi-threaded convolution, then should not go > 8192.
    ReverbConvolver(AudioChannel* impulseResponse, size_t renderSliceSize, size_t maxFFTSize, size_t convolverRenderPhase, bool useBackgroundThreads);
    ~ReverbConvolver();

    void process(ContextRenderLock& r, const AudioChannel* sourceChannel, AudioChannel* destinationChannel, size_t framesToProcess);
    void reset();

    size_t impulseResponseLength() const { return m_impulseResponseLength; }

    ReverbInputBuffer* inputBuffer() { return &m_inputBuffer; }

    bool useBackgroundThreads() const { return m_useBackgroundThreads; }
    void backgroundThreadEntry();

    size_t latencyFrames() const;

private:

    std::vector<std::unique_ptr<ReverbConvolverStage> > m_stages;
    std::vector<std::unique_ptr<ReverbConvolverStage> > m_backgroundStages;
    size_t m_impulseResponseLength;

    ReverbAccumulationBuffer m_accumulationBuffer;

    // One or more background threads read from this input buffer which is fed from the realtime thread.
    ReverbInputBuffer m_inputBuffer;

    // First stage will be of size m_minFFTSize.  Each next stage will be twice as big until we hit m_maxFFTSize.
    size_t m_minFFTSize;
    size_t m_maxFFTSize;

    // But don't exceed this size in the real-time thread (if we're doing background processing).
    size_t m_maxRealtimeFFTSize;

    // Background thread and synchronization
    bool m_useBackgroundThreads;
    std::thread m_backgroundThread;
    std::atomic<bool> m_wantsToExit;
    bool m_moreInputBuffered;
    mutable std::mutex m_backgroundThreadLock;
    mutable std::condition_variable m_backgroundThreadCondition;
};

} // namespace lab

#endif // ReverbConvolver_h
