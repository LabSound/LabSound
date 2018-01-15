// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct MicrophoneLoopbackApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext();
        
        // Danger - this sample creates an open feedback loop :)
        std::shared_ptr<AudioHardwareSourceNode> input;
        {
            ContextRenderLock r(context.get(), "MicrophoneLoopbackApp");
            input = lab::MakeHardwareSourceNode(r);
            context->connect(context->destination(), input, 0, 0);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
};
