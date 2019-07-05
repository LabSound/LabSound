// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

// Illustrates the use of simple equal-power stereo panning
struct StereoPanningApp : public LabSoundExampleApp
{
    virtual void PlayExample(int argc, char** argv) override
    {
        auto context = lab::MakeRealtimeAudioContext(lab::Channels::Stereo);

        std::shared_ptr<AudioBus> audioClip = MakeBusFromSampleFile("samples/trainrolling.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>();
        auto stereoPanner = std::make_shared<StereoPannerNode>(context->sampleRate());

        {
            ContextRenderLock r(context.get(), "Stereo Panning");

            audioClipNode->setBus(r, audioClip);
            context->connect(stereoPanner, audioClipNode, 0, 0);
            audioClipNode->start(0.0f);

            context->connect(context->destination(), stereoPanner, 0, 0);
        }

        if (audioClipNode)
        {
            audioClipNode->setLoop(true);

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

            Wait(std::chrono::seconds(seconds));

            controlThreadTest.join();
        }
        else
        {
            std::cerr << std::endl << "Couldn't initialize train node to play" << std::endl;
        }
    }
};
