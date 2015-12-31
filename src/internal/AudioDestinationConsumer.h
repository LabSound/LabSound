// License: BSD 3 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioDestinationConsumer_h
#define AudioDestinationConsumer_h

namespace lab {

class AudioBus;
    
struct AudioDestinationConsumer
{
    virtual ~AudioDestinationConsumer() { }
    virtual void setFormat(size_t numberOfChannels, float sampleRate) = 0;
    virtual void consumeAudio(AudioBus*, size_t numberOfFrames) = 0;
};

}

#endif
