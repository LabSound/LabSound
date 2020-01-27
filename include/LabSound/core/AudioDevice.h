// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef lab_audiodevice_h
#define lab_audiodevice_h

#include <stddef.h>
#include <stdint.h>

namespace lab
{
    class AudioBus;

    /////////////////////////////////////////////
    //   Audio Device Configuration Settings   //
    /////////////////////////////////////////////

    struct AudioDeviceInfo
    {
        int32_t index{-1};
        std::string identifier;
        uint32_t num_output_channels;
        uint32_t num_input_channels;
        std::vector<float> supported_samplerates;
        float nominal_samplerate;
        bool is_default;
    };

    // Input and Output
    struct AudioStreamConfig
    {
        int32_t device_index{-1};
        uint32_t desired_channels;
        float desired_samplerate;
    };

    ///////////////////////////////////
    //   AudioDeviceRenderCallback   //
    ///////////////////////////////////

    // `render()` is called periodically to get the next render quantum of audio into destinationBus.
    // Optional audio input is given in sourceBus (if not nullptr). This structure also keeps
    // track of timing information with respect to the callback.
    class AudioDeviceRenderCallback
    {
    protected:
        uint64_t m_currentSampleFrame {0};
    public:
        virtual ~AudioDeviceRenderCallback() {}
        virtual void render(AudioBus * sourceBus, AudioBus * destinationBus, size_t framesToProcess) = 0;
        virtual void start()                        = 0;
        virtual void stop()                         = 0;
        virtual uint64_t currentSampleFrame() const = 0;
        virtual double currentTime() const          = 0;
        virtual double currentSampleTime() const    = 0;
    };

    /////////////////////
    //   AudioDevice   //
    /////////////////////

    // The audio hardware periodically calls the AudioDeviceRenderCallback `render()` method asking it to 
    // render/output the next render quantum of audio. It optionally will pass in local/live audio 
    // input when it calls `render()`.
    struct AudioDevice
    {
        static std::vector<AudioDeviceInfo> MakeAudioDeviceList();
        static uint32_t GetDefaultOutputAudioDeviceIndex();
        static uint32_t GetDefaultInputAudioDeviceIndex();
        static AudioDevice * MakePlatformSpecificDevice(AudioDeviceRenderCallback &, const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig);

        virtual ~AudioDevice() {}
        virtual void start() = 0;
        virtual void stop() = 0;
    };

    //  virtual float sampleRate() const = 0; // basically get configured settings

} // lab

#endif // lab_audiodevice_h
