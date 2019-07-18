// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct MicrophoneLoopbackApp : public LabSoundExampleApp
{
    virtual void PlayExample(int argc, char** argv) override
    {
        auto context = lab::Sound::MakeRealtimeAudioContext(lab::Channels::Stereo);

        // Danger - this sample creates an open feedback loop :)
        std::shared_ptr<AudioHardwareSourceNode> input;
        {
            ContextRenderLock r(context.get(), "MicrophoneLoopbackApp");
            input = lab::Sound::MakeHardwareSourceNode(r);
            context->connect(context->destination(), input, 0, 0);
        }

        Wait(std::chrono::seconds(10));
    }
};
