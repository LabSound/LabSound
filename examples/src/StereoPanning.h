// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

// Illustrates the use of simple equal-power stereo panning
struct StereoPanningApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeAudioContext();
        auto ac = context.get();
        
        SoundBuffer train("samples/trainrolling.wav", context->sampleRate());
        auto stereoPanner = std::make_shared<StereoPannerNode>(context->sampleRate());
        
        std::shared_ptr<AudioBufferSourceNode> trainNode;
        {
            ContextGraphLock g(context, "Panning");
            ContextRenderLock r(context, "Panning");
            stereoPanner->connect(ac, context->destination().get(), 0, 0);
            trainNode = train.play(r, stereoPanner, 0.0f);
        }
        
        if (trainNode)
        {
            trainNode->setLooping(true);
            
            const int seconds = 8;
            
            std::thread controlThreadTest([&stereoPanner, seconds]()
            {

                float halfTime = seconds * 0.5f;
                for (float i = 0; i < seconds; i += 0.01f)
                {
                    float x = (i - halfTime) / halfTime;
                    stereoPanner->pan()->setValue(x);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            
            controlThreadTest.join();

        }
        else
        {
            std::cerr << std::endl << "Couldn't initialize train node to play" << std::endl;
        }
        
        lab::CleanupAudioContext(context);
    }
};
