// SPDX-License-Identifier: BSD-3-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#pragma once

#ifndef labsound_audiodevice_rtaudio_hpp
#define labsound_audiodevice_rtaudio_hpp

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNode.h"

class RtAudio;

namespace lab
{

class AudioDevice_RtAudio : public AudioDevice
{
    std::unique_ptr<AudioBus> _renderBus;
    std::unique_ptr<AudioBus> _inputBus;
    SamplingInfo samplingInfo;

    void createContext();

public:
    AudioDevice_RtAudio(
            const AudioStreamConfig & inputConfig,
            const AudioStreamConfig & outputConfig);
    virtual ~AudioDevice_RtAudio();

    float authoritativeDeviceSampleRateAtRuntime {0.f};

    // AudioDevice Interface
    void render(AudioSourceProvider*, int numberOfFrames, void * outputBuffer, void * inputBuffer);
    virtual void start() override final;
    virtual void stop() override final;
    virtual bool isRunning() const override final;
    virtual void backendReinitialize() override final;
    
    static std::vector<AudioDeviceInfo> MakeAudioDeviceList();
};

}  // namespace lab

#endif  // labsound_audiodevice_rtaudio_hpp
