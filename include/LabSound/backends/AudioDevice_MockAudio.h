// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#ifndef labsound_audiodevice_mockaudio_hpp
#define labsound_audiodevice_mockaudio_hpp

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/ConcurrentQueue.h"

#include <string>
#include <vector>

namespace lab {

class AudioDevice_Mockaudio : public AudioDevice
{
    AudioBus * _renderBus = nullptr;
    AudioBus * _inputBus = nullptr;
    SamplingInfo samplingInfo;

    // Memory buffer for output
    std::vector<float> _memoryBuffer;
    size_t _bufferSize = 0;
    size_t _currentBufferPosition = 0;
    std::string _wavFilePath;
    bool _bufferFull = false;
    
    int _remainder = 0;

public:
    AudioDevice_Mockaudio(
        const AudioStreamConfig & outputConfig, 
        const AudioStreamConfig & inputConfig,
        size_t bufferSize,
        const std::string& wavFilePath);
    virtual ~AudioDevice_Mockaudio();

    float authoritativeDeviceSampleRateAtRuntime{0.f};

    // AudioDevice Interface
    virtual int render(int numberOfFrames, void * outputBuffer, void * inputBuffer) override final;
    virtual void start() override final;
    virtual void stop() override final;
    virtual bool isRunning() const override final;
    virtual void backendReinitialize() override final;
    
    // Write the buffer to a WAV file
    void writeToWavFile();
    
    // Check if buffer is full
    bool isBufferFull() const { return _bufferFull; }
    
    static std::vector<AudioDeviceInfo> MakeAudioDeviceList();
};

}  // namespace lab

#endif  // labsound_audiodevice_mockaudio_hpp
