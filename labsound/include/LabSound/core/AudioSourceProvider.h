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
        virtual void setFormat(ContextRenderLock& r, size_t numberOfChannels, float sampleRate) = 0;
    protected:
        virtual ~AudioSourceProviderClient() { }
};

// Abstract base-class for a pull-model client.
class AudioSourceProvider 
{
public:
    // provideInput() gets called repeatedly to render time-slices of a continuous audio stream.
    virtual void provideInput(AudioBus* bus, size_t framesToProcess) = 0;

    // If a client is set, we call it back when the audio format is available or changes.
    virtual void setClient(AudioSourceProviderClient*) { };

    virtual ~AudioSourceProvider() { }
};

} // lab

#endif // AudioSourceProvider_h
