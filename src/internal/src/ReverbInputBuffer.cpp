// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/ReverbInputBuffer.h"
#include "internal/Assertions.h"

namespace lab {

ReverbInputBuffer::ReverbInputBuffer(size_t length)
    : m_buffer(length)
    , m_writeIndex(0)
{
}

void ReverbInputBuffer::write(const float* sourceP, size_t numberOfFrames)
{
    size_t bufferLength = m_buffer.size();
    bool isCopySafe = m_writeIndex + numberOfFrames <= bufferLength;
    ASSERT(isCopySafe);
    if (!isCopySafe)
        return;
        
    memcpy(m_buffer.data() + m_writeIndex, sourceP, sizeof(float) * numberOfFrames);

    m_writeIndex += numberOfFrames;
    ASSERT(m_writeIndex <= bufferLength);

    if (m_writeIndex >= bufferLength)
        m_writeIndex = 0;
}

float* ReverbInputBuffer::directReadFrom(int* readIndex, size_t numberOfFrames)
{
    size_t bufferLength = m_buffer.size();
    bool isPointerGood = readIndex && *readIndex >= 0 && *readIndex + numberOfFrames <= bufferLength;
    ASSERT(isPointerGood);
    if (!isPointerGood) {
        // Should never happen in practice but return pointer to start of buffer (avoid crash)
        if (readIndex)
            *readIndex = 0;
        return m_buffer.data();
    }
        
    float* sourceP = m_buffer.data();
    float* p = sourceP + *readIndex;

    // Update readIndex
    *readIndex = (*readIndex + numberOfFrames) % bufferLength;

    return p;
}

void ReverbInputBuffer::reset()
{
    m_buffer.zero();
    m_writeIndex = 0;
}

} // namespace lab
