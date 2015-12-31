// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/AudioChannel.h"
#include "internal/VectorMath.h"
#include "internal/Assertions.h"

#include <algorithm>
#include <math.h>

namespace lab {

using namespace VectorMath;

void AudioChannel::resizeSmaller(size_t newLength)
{
    ASSERT(newLength <= m_length);
    if (newLength <= m_length)
        m_length = newLength;
}

void AudioChannel::scale(float scale)
{
    if (isSilent())
        return;

    vsmul(data(), 1, &scale, mutableData(), 1, length());
}

void AudioChannel::copyFrom(const AudioChannel* sourceChannel)
{
    bool isSafe = (sourceChannel && sourceChannel->length() >= length());
    ASSERT(isSafe);
    if (!isSafe)
        return;

    if (sourceChannel->isSilent()) {
        zero();
        return;
    }
    memcpy(mutableData(), sourceChannel->data(), sizeof(float) * length());
}

void AudioChannel::copyFromRange(const AudioChannel* sourceChannel, unsigned startFrame, unsigned endFrame)
{
    // Check that range is safe for reading from sourceChannel.
    bool isRangeSafe = sourceChannel && startFrame < endFrame && endFrame <= sourceChannel->length();
    ASSERT(isRangeSafe);
    if (!isRangeSafe)
        return;

    if (sourceChannel->isSilent() && isSilent())
        return;

    // Check that this channel has enough space.
    size_t rangeLength = endFrame - startFrame;
    bool isRangeLengthSafe = rangeLength <= length();
    ASSERT(isRangeLengthSafe);
    if (!isRangeLengthSafe)
        return;

    const float* source = sourceChannel->data();
    float* destination = mutableData();

    if (sourceChannel->isSilent()) {
        if (rangeLength == length())
            zero();
        else
            memset(destination, 0, sizeof(float) * rangeLength);
    } else
        memcpy(destination, source + startFrame, sizeof(float) * rangeLength);
}

void AudioChannel::sumFrom(const AudioChannel* sourceChannel)
{
    bool isSafe = sourceChannel && sourceChannel->length() >= length();
    ASSERT(isSafe);
    if (!isSafe)
        return;

    if (sourceChannel->isSilent())
        return;

    if (isSilent())
        copyFrom(sourceChannel);
    else
        vadd(data(), 1, sourceChannel->data(), 1, mutableData(), 1, length());
}

float AudioChannel::maxAbsValue() const
{
    if (isSilent())
        return 0;

    float max = 0;

    vmaxmgv(data(), 1, &max, length());

    return max;
}

} // lab
