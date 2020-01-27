// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct MicrophoneLoopbackApp : public LabSoundExampleApp
{
    virtual void PlayExample(int argc, char** argv) override
    {
        auto context = lab::MakeRealtimeAudioContext(lab::Channels::Stereo);

        // Danger - this sample creates an open feedback loop :)
        std::shared_ptr<AudioHardwareInputNode> input;
        {
            ContextRenderLock r(context.get(), "MicrophoneLoopbackApp");
            input = lab::MakeAudioHardwareInputNode(r);
            context->connect(context->device(), input, 0, 0);
        }

        Wait(std::chrono::seconds(10));
    }
};
