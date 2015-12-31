// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ReverbInputBuffer_h
#define ReverbInputBuffer_h

#include "LabSound/core/AudioArray.h"

namespace lab {

// ReverbInputBuffer is used to buffer input samples for deferred processing by the background threads.
class ReverbInputBuffer {
public:
    ReverbInputBuffer(size_t length);

    // The realtime audio thread keeps writing samples here.
    // The assumption is that the buffer's length is evenly divisible by numberOfFrames (for nearly all cases this will be fine).
    // FIXME: remove numberOfFrames restriction...
    void write(const float* sourceP, size_t numberOfFrames);

    // Background threads can call this to check if there's anything to read...
    size_t writeIndex() const { return m_writeIndex; }

    // The individual background threads read here (and hope that they can keep up with the buffer writing).
    // readIndex is updated with the next readIndex to read from...
    // The assumption is that the buffer's length is evenly divisible by numberOfFrames.
    // FIXME: remove numberOfFrames restriction...
    float* directReadFrom(int* readIndex, size_t numberOfFrames);

    void reset();

private:
    AudioFloatArray m_buffer;
    size_t m_writeIndex;
};

} // namespace lab

#endif // ReverbInputBuffer_h
