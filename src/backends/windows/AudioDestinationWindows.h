
// License: BSD 3 Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#ifndef AudioDestinationWin_h
#define AudioDestinationWin_h

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNode.h"

#include "internal/AudioDestination.h"

#include "rtaudio/RtAudio.h"
#include <cstdlib>
#include <iostream>
#include <memory>

namespace lab
{

class AudioDestinationWin : public AudioDestination
{

public:
    AudioDestinationWin(AudioIOCallback &, size_t numChannels, float sampleRate);
    virtual ~AudioDestinationWin();

    virtual void start() override;
    virtual void stop() override;

    float sampleRate() const override { return _sampleRate; }

    void render(int numberOfFrames, void * outputBuffer, void * inputBuffer);

    uint32_t outputChannelCount() const { return _numOutputChannels; }

private:
    void configure();

    AudioIOCallback & m_callback;
    AudioBus m_renderBus = {2, AudioNode::ProcessingSizeInFrames, false};
    std::unique_ptr<AudioBus> m_inputBus;
    size_t m_numChannels;
    float m_sampleRate;
    RtAudio dac;
};

int outputCallback(void * outputBuffer, void * inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void * userData);

}  // namespace lab

#endif  // AudioDestinationWin_h
