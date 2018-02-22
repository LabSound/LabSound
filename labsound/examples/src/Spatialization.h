// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

// Illustrates 3d sound spatialization and doppler shift
struct SpatializationApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext();
        
        std::shared_ptr<AudioBus> audioClip = MakeBusFromFile("samples/trainrolling.wav", false);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>();
        std::shared_ptr<PannerNode> panner = std::make_shared<PannerNode>(context->sampleRate(), "hrtf"); // note search path

        {
            ContextRenderLock r(context.get(), "spatialization");
            
            panner->setPanningModel(PanningMode::HRTF);
            context->connect(context->destination(), panner, 0, 0);

            audioClipNode->setBus(r, audioClip);
            context->connect(panner, audioClipNode, 0, 0);
            audioClipNode->start(0.0f);
        }
        
        if (audioClipNode)
        {
            audioClipNode->setLoop(true);
            context->listener().setPosition(0, 0, 0);
            panner->setVelocity(4, 0, 0);

            const int seconds = 10;
            float halfTime = seconds * 0.5f;
            for (float i = 0; i < seconds; i += 0.01f)
            {
                float x = (i - halfTime) / halfTime;

                // Put position a +up && +front, because if it goes right through the
                // listener at (0, 0, 0) it abruptly switches from left to right.
                panner->setPosition(x, 0.1f, 0.1f);

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        else
        {
            std::cerr << std::endl << "Couldn't initialize train node to play" << std::endl;
        }
    }
};
