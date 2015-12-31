// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct MicrophoneLoopbackApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeAudioContext();
        
        std::shared_ptr<AudioHardwareSourceNode> input;
        {
            ContextGraphLock g(context, "MicrophoneLoopbackApp");
            ContextRenderLock r(context, "MicrophoneLoopbackApp");
            
            input = MakeHardwareSourceNode(r);
            input->connect(context.get(), context->destination().get(), 0, 0);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        lab::CleanupAudioContext(context);
    }
};
