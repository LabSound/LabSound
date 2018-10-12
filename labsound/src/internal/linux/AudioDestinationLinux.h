// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioDestinationLinux_h
#define AudioDestinationLinux_h

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioBus.h"

#include "internal/AudioDestination.h"

#include "rtaudio/RtAudio.h"
#include <iostream>
#include <cstdlib>

namespace lab {

class AudioDestinationLinux : public AudioDestination 
{ 

public:

    AudioDestinationLinux(AudioIOCallback &, float sampleRate);
    virtual ~AudioDestinationLinux();

    virtual void start() override;
    virtual void stop() override;
    virtual void startRecording() override;
    virtual void stopRecording() override;

    bool isPlaying() override { return m_isPlaying; }
    bool isRecording() override { return m_isRecording; }
    float sampleRate() const override { return m_sampleRate; }

    void render(int numberOfFrames, void * outputBuffer, void * inputBuffer);

private:

    void configure();

    AudioIOCallback & m_callback;

    AudioBus m_renderBus = {2, AudioNode::ProcessingSizeInFrames, false};
    AudioBus m_inputBus = {1, AudioNode::ProcessingSizeInFrames, false};

    float m_sampleRate;
    bool m_isPlaying = false;
    bool m_isRecording = false;

    std::unique_ptr<RtAudio> dac; // XXX
};

int outputCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData ); 

} // namespace lab

#endif // AudioDestinationWin_h

