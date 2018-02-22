// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioIOCallback_h
#define AudioIOCallback_h

#include <stddef.h>

namespace lab
{

class AudioBus;
struct AudioIOCallback
{
    // render() is called periodically to get the next render quantum of audio into destinationBus.
    // Optional audio input is given in sourceBus (if it's not 0).
    virtual void render(AudioBus * sourceBus, AudioBus * destinationBus, size_t framesToProcess) = 0;
    virtual ~AudioIOCallback() {}
};

} // lab

#endif // AudioIOCallback_h
