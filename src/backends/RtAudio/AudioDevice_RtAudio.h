// License: BSD 3 Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#ifndef AudioDestinationRtAudio_h
#define AudioDestinationRtAudio_h

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"

#include "RtAudio.h"

namespace lab
{

class AudioDevice_RtAudio : public AudioDevice
{
    AudioDeviceRenderCallback & _callback;

    AudioBus * _renderBus = nullptr;
    AudioBus * _inputBus = nullptr;

    RtAudio _dac;

public:

    AudioDevice_RtAudio(AudioDeviceRenderCallback &, const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig);
    virtual ~AudioDevice_RtAudio();

    // AudioDevice Interface
    void render(int numberOfFrames, void * outputBuffer, void * inputBuffer);
    virtual void start() override;
    virtual void stop() override;
};

int rt_audio_callback(void * outputBuffer, void * inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void * userData);

}  // namespace lab

#endif  // AudioDestinationRtAudio_h
