// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/AudioContextLock.h"

#include "internal/ReverbConvolverStage.h"
#include "internal/VectorMath.h"
#include "internal/ReverbAccumulationBuffer.h"
#include "internal/ReverbConvolver.h"
#include "internal/ReverbInputBuffer.h"

namespace lab {

using namespace VectorMath;

ReverbConvolverStage::ReverbConvolverStage(const float* impulseResponse, size_t, size_t reverbTotalLatency, size_t stageOffset, size_t stageLength,
                                           size_t fftSize, size_t renderPhase, size_t renderSliceSize, ReverbAccumulationBuffer* accumulationBuffer, bool directMode)
    : m_accumulationBuffer(accumulationBuffer)
    , m_accumulationReadIndex(0)
    , m_inputReadIndex(0)
    , m_directMode(directMode)
{
    ASSERT(impulseResponse);
    ASSERT(accumulationBuffer);

    if (!m_directMode) {
        m_fftKernel = std::unique_ptr<FFTFrame>(new FFTFrame(fftSize));
        m_fftKernel->doPaddedFFT(impulseResponse + stageOffset, stageLength);
        m_fftConvolver = std::unique_ptr<FFTConvolver>(new FFTConvolver(fftSize));
    } else {
        m_directKernel = std::unique_ptr<AudioFloatArray>(new AudioFloatArray(fftSize / 2));
        m_directKernel->copyToRange(impulseResponse + stageOffset, 0, fftSize / 2);
        m_directConvolver = std::unique_ptr<DirectConvolver>(new DirectConvolver(renderSliceSize));
    }
    m_temporaryBuffer.allocate(renderSliceSize);

    // The convolution stage at offset stageOffset needs to have a corresponding delay to cancel out the offset.
    size_t totalDelay = stageOffset + reverbTotalLatency;

    // But, the FFT convolution itself incurs fftSize / 2 latency, so subtract this out...
    size_t halfSize = fftSize / 2;
    if (!m_directMode) {
        ASSERT(totalDelay >= halfSize);
        if (totalDelay >= halfSize)
            totalDelay -= halfSize;
    }

    // We divide up the total delay, into pre and post delay sections so that we can schedule at exactly the moment when the FFT will happen.
    // This is coordinated with the other stages, so they don't all do their FFTs at the same time...
    int maxPreDelayLength = std::min(halfSize, totalDelay);
    m_preDelayLength = totalDelay > 0 ? renderPhase % maxPreDelayLength : 0;
    if (m_preDelayLength > totalDelay)
        m_preDelayLength = 0;

    m_postDelayLength = totalDelay - m_preDelayLength;
    m_preReadWriteIndex = 0;
    m_framesProcessed = 0; // total frames processed so far

    size_t delayBufferSize = m_preDelayLength < fftSize ? fftSize : m_preDelayLength;
    delayBufferSize = delayBufferSize < renderSliceSize ? renderSliceSize : delayBufferSize;
    m_preDelayBuffer.allocate(delayBufferSize);
}

void ReverbConvolverStage::processInBackground(ReverbConvolver* convolver, size_t framesToProcess)
{
    ReverbInputBuffer* inputBuffer = convolver->inputBuffer();
    float* source = inputBuffer->directReadFrom(&m_inputReadIndex, framesToProcess);
    process(source, framesToProcess);
}

void ReverbConvolverStage::process(const float* source, size_t framesToProcess)
{
    ASSERT(source);
    if (!source)
        return;
    
    // Deal with pre-delay stream : note special handling of zero delay.

    const float* preDelayedSource;
    float* preDelayedDestination;
    float* temporaryBuffer;
    bool isTemporaryBufferSafe = false;
    if (m_preDelayLength > 0) {
        // Handles both the read case (call to process() ) and the write case (memcpy() )
        bool isPreDelaySafe = m_preReadWriteIndex + framesToProcess <= m_preDelayBuffer.size();
        ASSERT(isPreDelaySafe);
        if (!isPreDelaySafe)
            return;

        isTemporaryBufferSafe = framesToProcess <= m_temporaryBuffer.size();

        preDelayedDestination = m_preDelayBuffer.data() + m_preReadWriteIndex;
        preDelayedSource = preDelayedDestination;
        temporaryBuffer = m_temporaryBuffer.data();        
    } else {
        // Zero delay
        preDelayedDestination = 0;
        preDelayedSource = source;
        temporaryBuffer = m_preDelayBuffer.data();
        
        isTemporaryBufferSafe = framesToProcess <= m_preDelayBuffer.size();
    }
    
    ASSERT(isTemporaryBufferSafe);
    if (!isTemporaryBufferSafe)
        return;

    if (m_framesProcessed < m_preDelayLength) {
        // For the first m_preDelayLength frames don't process the convolver, instead simply buffer in the pre-delay.
        // But while buffering the pre-delay, we still need to update our index.
        m_accumulationBuffer->updateReadIndex(&m_accumulationReadIndex, framesToProcess);
    } else {
        // Now, run the convolution (into the delay buffer).
        // An expensive FFT will happen every fftSize / 2 frames.
        // We process in-place here...
        if (!m_directMode)
            m_fftConvolver->process(m_fftKernel.get(), preDelayedSource, temporaryBuffer, framesToProcess);
        else
            m_directConvolver->process(m_directKernel.get(), preDelayedSource, temporaryBuffer, framesToProcess);

        // Now accumulate into reverb's accumulation buffer.
        m_accumulationBuffer->accumulate(temporaryBuffer, framesToProcess, &m_accumulationReadIndex, m_postDelayLength);
    }

    // Finally copy input to pre-delay.
    if (m_preDelayLength > 0) {
        memcpy(preDelayedDestination, source, sizeof(float) * framesToProcess);
        m_preReadWriteIndex += framesToProcess;

        ASSERT(m_preReadWriteIndex <= m_preDelayLength);
        if (m_preReadWriteIndex >= m_preDelayLength)
            m_preReadWriteIndex = 0;
    }

    m_framesProcessed += framesToProcess;
}

void ReverbConvolverStage::reset()
{
    if (!m_directMode)
        m_fftConvolver->reset();
    else
        m_directConvolver->reset();
    
    m_preDelayBuffer.zero();
    m_accumulationReadIndex = 0;
    m_inputReadIndex = 0;
    m_framesProcessed = 0;
}

} // namespace lab
