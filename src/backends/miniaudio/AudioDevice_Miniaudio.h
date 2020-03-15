// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#ifndef labsound_audiodevice_miniaudio_hpp
#define labsound_audiodevice_miniaudio_hpp

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"
#include "LabSound/core/AudioNode.h"

#include "internal/RingBuffer.h"

#include "miniaudio.h"

namespace lab
{

class AudioDevice_Miniaudio : public AudioDevice
{
    RingBufferT<float> * _ring = nullptr;
    float * _scratch = nullptr;
    AudioDeviceRenderCallback & _callback;
    AudioBus * _renderBus = nullptr;
    AudioBus * _inputBus = nullptr;
    ma_device _device;
    int _remainder = 0;
    SamplingInfo samplingInfo;

public:
#ifdef _MSC_VER
    // satisfy ma_device alignment requirements for msvc.
    void * operator new(size_t i) { return _mm_malloc(i, 64); }
    void operator delete(void * p) { _mm_free(p); }
#endif

    AudioDevice_Miniaudio(AudioDeviceRenderCallback &, const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig);
    virtual ~AudioDevice_Miniaudio();

    AudioStreamConfig outputConfig;
    AudioStreamConfig inputConfig;
    float authoritativeDeviceSampleRateAtRuntime{0.f};

    // AudioDevice Interface
    void render(int numberOfFrames, void * outputBuffer, void * inputBuffer);
    virtual void start() override final;
    virtual void stop() override final;
};

}  // namespace lab

#endif  // labsound_audiodevice_miniaudio_hpp
