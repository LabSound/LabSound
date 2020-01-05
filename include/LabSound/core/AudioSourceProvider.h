// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioSourceProvider_h
#define AudioSourceProvider_h

#include "LabSound/extended/AudioContextLock.h"

namespace lab {

class AudioBus;

class AudioSourceProviderClient
{
    public:
        virtual void setFormat(ContextRenderLock& r, uint32_t numberOfChannels, float sampleRate) = 0;
    protected:
        virtual ~AudioSourceProviderClient() { }
};

// Abstract base-class for pull-model client
class AudioSourceProvider
{
public:
    virtual ~AudioSourceProvider() = default;

    // provideInput() renders from a continuous audio stream into the supplied bus
    virtual void provideInput(AudioBus* bus, size_t framesToProcess) = 0;
    virtual void provideInput(uint32_t channel, float* bus, size_t framesToProcess) = 0;

    virtual uint32_t numberOfChannels() const = 0;
};

} // lab

#endif // AudioSourceProvider_h
