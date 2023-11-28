// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#ifndef labsound_audiodevice_miniaudio_hpp
#define labsound_audiodevice_miniaudio_hpp

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/ConcurrentQueue.h"

struct ma_device;

namespace lab {

class AudioDevice_Miniaudio : public AudioDevice
{
    AudioBus * _renderBus = nullptr;
    AudioBus * _inputBus = nullptr;
    ma_device* _device = nullptr;
    SamplingInfo samplingInfo;

    lab::RingBufferT<float> * _ring = nullptr;
    float * _scratch = nullptr;
    int _remainder = 0;

public:

#ifdef _MSC_VER
    // satisfy ma_device alignment requirements for msvc.
    void * operator new(size_t i) { return _mm_malloc(i, 64); }
    void operator delete(void * p) { _mm_free(p); }
#endif

    AudioDevice_Miniaudio(
        const AudioStreamConfig & outputConfig, 
        const AudioStreamConfig & inputConfig);
    virtual ~AudioDevice_Miniaudio();

    float authoritativeDeviceSampleRateAtRuntime{0.f};

    // AudioDevice Interface
    void render(int numberOfFrames, void * outputBuffer, void * inputBuffer);
    virtual void start() override final;
    virtual void stop() override final;
    virtual bool isRunning() const override final;
    virtual void backendReinitialize() override final;

    static std::vector<AudioDeviceInfo> MakeAudioDeviceList();
};

}  // namespace lab

#endif  // labsound_audiodevice_miniaudio_hpp
