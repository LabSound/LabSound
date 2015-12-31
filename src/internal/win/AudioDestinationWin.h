// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioDestinationWin_h
#define AudioDestinationWin_h

#include "LabSound/core/AudioNode.h"

#include "internal/AudioBus.h"
#include "internal/AudioDestination.h"

#include "rtaudio/RtAudio.h"
#include <iostream>
#include <cstdlib>

namespace lab {

class AudioDestinationWin : public AudioDestination { 

public:

    AudioDestinationWin(AudioIOCallback&, float sampleRate);
    virtual ~AudioDestinationWin();

    virtual void start() override;
    virtual void stop() override;

    bool isPlaying() override { return m_isPlaying; }
    float sampleRate() const override { return m_sampleRate; }

    void render(int numberOfFrames, void *outputBuffer, void *inputBuffer); 

private:

    void configure();

    AudioIOCallback & m_callback;
    AudioBus m_renderBus = {2, AudioNode::ProcessingSizeInFrames, false};

    float m_sampleRate;
    bool m_isPlaying = false;;

    RtAudio dac;

};

int outputCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData ); 

} // namespace lab

#endif // AudioDestinationWin_h

