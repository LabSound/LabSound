// SPDX-License-Identifier: BSD-3-Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioSourceProvider_h
#define AudioSourceProvider_h

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/core/AudioBus.h"

namespace lab
{
class AudioBus;

//----------------------------------------------------
//   AudioSourceProvider
// Buffers independent input reads to an output cache
//-----------------------------------------------------

// provideInput() gets called repeatedly to render time-slices of a continuous audio stream.
class AudioSourceProvider
{
    AudioBus _sourceBus;

public:
    AudioSourceProvider(int channelCount)
        : _sourceBus(channelCount, AudioNode::ProcessingSizeInFrames)
    {
    }

    virtual ~AudioSourceProvider() = default;

    // every input quantum set can be called to copy from the supplied bus to the internal buffer
    void set(AudioBus * bus)
    {
        if (bus)
            _sourceBus.copyFrom(*bus);
    }

    // every output quantum provideInput can be called to copy data retained in _sourceBus
    // to the supplied destinationBus
    virtual void provideInput(AudioBus * destinationBus, int bufferSize)
    {
        bool isGood = destinationBus && destinationBus->length() == bufferSize && _sourceBus.length() == bufferSize;
        if (isGood)
            destinationBus->copyFrom(_sourceBus);
    }};

}  // lab

#endif  // AudioSourceProvider_h
