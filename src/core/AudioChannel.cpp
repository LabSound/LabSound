// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioChannel.h"
#include "internal/VectorMath.h"
#include <stdlib.h>
#include <string.h>

namespace lab
{

AudioChannel::~AudioChannel()
{
    if (_data) free(_data);
}

void AudioChannel::setSize(size_t size)
{
    if (_capacity < size)
    {
        float * newData = reinterpret_cast<float *>(malloc(size * sizeof(float)));
        if (_data && newData && _sz)
            memcpy(newData, _data, _sz);

        if (_data)
            free(_data);

        _data = newData;
        _sz = newData ? size : 0;
        _capacity = _sz;
    }
    _sz = size;
}
void AudioChannel::setSilent()
{
    if (_silent)
        return;

    _silent = true;
    memset(_data, 0, sizeof(float) * _capacity);
}

float AudioChannel::maxAbsValue() const
{
    if (isSilent())
        return 0;

    float max = 0;
    VectorMath::vmaxmgv(data(), 1, &max, length());
    return max;
}

void AudioChannel::copyData(AudioChannel const * const sourceChannel)
{
    size_t sz = sourceChannel->size();
    if (_capacity < sz)
    {
        if (_data)
            free(_data);

        _data = reinterpret_cast<float *>(malloc(sz * sizeof(float)));
        _capacity = sz;
    }
    _sz = sz;

    if (sourceChannel->isSilent())
        setSilent();
    else if (_data && _sz)
        memcpy(_data, sourceChannel->data(), sz * sizeof(float));
}

// Adds sourceChannel data to the existing data
void AudioChannel::sumFrom(const AudioChannel * sourceChannel)
{
    if (sourceChannel->isSilent())
        return;

    if (isSilent())
    {
        copyData(sourceChannel);
    }
    else
    {
        size_t sz = _sz < sourceChannel->size() ? _sz : sourceChannel->size();
        VectorMath::vadd(data(), 1, sourceChannel->data(), 1, mutableData(), 1, _sz);
    }
    clearSilentFlag();
}

// Scales all samples by the same amount.
void AudioChannel::scale(float scale)
{
    if (!isSilent())
        VectorMath::vsmul(data(), 1, &scale, mutableData(), 1, size());
}

}  // lab

#if 0
#include "internal/Assertions.h"
#include "internal/VectorMath.h"

#include "LabSound/core/AudioChannel.h"

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

void AudioChannel::copyFrom(const AudioChannel * sourceChannel)
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

void AudioChannel::copyFromRange(const AudioChannel* sourceChannel, size_t startFrame, size_t endFrame)
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

#endif
