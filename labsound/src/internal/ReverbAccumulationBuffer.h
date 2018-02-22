// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ReverbAccumulationBuffer_h
#define ReverbAccumulationBuffer_h

#include "LabSound/core/AudioArray.h"

namespace lab {

// ReverbAccumulationBuffer is a circular delay buffer with one client reading from it and multiple clients
// writing/accumulating to it at different delay offsets from the read position.  The read operation will zero the memory
// just read from the buffer, so it will be ready for accumulation the next time around.
class ReverbAccumulationBuffer {
public:
    ReverbAccumulationBuffer(size_t length);

    // This will read from, then clear-out numberOfFrames
    void readAndClear(float* destination, size_t numberOfFrames);

    // Each ReverbConvolverStage will accumulate its output at the appropriate delay from the read position.
    // We need to pass in and update readIndex here, since each ReverbConvolverStage may be running in
    // a different thread than the realtime thread calling ReadAndClear() and maintaining m_readIndex
    // Returns the writeIndex where the accumulation took place
    int accumulate(float* source, size_t numberOfFrames, int* readIndex, size_t delayFrames);

    size_t readIndex() const { return m_readIndex; }
    void updateReadIndex(int* readIndex, size_t numberOfFrames) const;

    size_t readTimeFrame() const { return m_readTimeFrame; }

    void reset();

private:
    AudioFloatArray m_buffer;
    size_t m_readIndex;
    size_t m_readTimeFrame; // for debugging (frame on continuous timeline)
};

} // namespace lab

#endif // ReverbAccumulationBuffer_h
