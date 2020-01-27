// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioSourceProvider_h
#define AudioSourceProvider_h

#include "LabSound/extended/AudioContextLock.h"

namespace lab
{
    class AudioBus;

    // Abstract base-class for a pull-model client.
    // provideInput() gets called repeatedly to render time-slices of a continuous audio stream.
    struct AudioSourceProvider
    {
        virtual void provideInput(AudioBus * bus, size_t framesToProcess) = 0;
        virtual ~AudioSourceProvider() { }
    };

}  // lab

#endif  // AudioSourceProvider_h
