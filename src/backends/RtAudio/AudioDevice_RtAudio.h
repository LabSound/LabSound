// License: BSD 3 Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#ifndef AudioDestinationRtAudio_h
#define AudioDestinationRtAudio_h

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"
#include "LabSound/core/AudioNode.h"

#include "RtAudio.h"

namespace lab
{

class AudioDevice_RtAudio : public AudioDevice
{
    AudioDeviceRenderCallback & _callback;

    std::unique_ptr<AudioBus> _renderBus;
    std::unique_ptr<AudioBus> _inputBus;

    RtAudio rtaudio_ctx;

public:

    AudioDevice_RtAudio(AudioDeviceRenderCallback &, const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig);
    virtual ~AudioDevice_RtAudio();

    AudioStreamConfig outputConfig;
    AudioStreamConfig inputConfig;
    float authoritativeDeviceSampleRateAtRuntime {0.f};

    // AudioDevice Interface
    void render(int numberOfFrames, void * outputBuffer, void * inputBuffer);
    virtual void start() override final;
    virtual void stop() override final;
    virtual float getSampleRate() override final;
};

int rt_audio_callback(void * outputBuffer, void * inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void * userData);

}  // namespace lab

#endif  // AudioDestinationRtAudio_h
