// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioIOCallback_h
#define AudioIOCallback_h

#include <stddef.h>
#include "LabSound/LabSound.h"

namespace lab
{
    class AudioBus;

    ///////////////////////////////////
    //   AudioDeviceRenderCallback   //
    ///////////////////////////////////

    // `render()` is called periodically to get the next render quantum of audio into destinationBus.
    // Optional audio input is given in sourceBus (if not nullptr).
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
        // @todo - pass in AudioStreamConfig
        static AudioDevice * MakePlatformSpecificDevice(AudioDeviceRenderCallback &, uint32_t num_inputs, uint32_t num_outputs, float sampleRate);
        virtual ~AudioDevice() {}
        virtual void start() = 0;
        virtual void stop() = 0;
        //  virtual float sampleRate() const = 0; // basically get configured settings
    };

} // lab

#endif // AudioIOCallback_h
