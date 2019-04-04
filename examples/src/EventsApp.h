// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct EventsApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext(lab::Channels::Stereo);
        auto musicClip = MakeBusFromFile("samples/stereo-music-clip.wav", false);
        auto gain = std::make_shared<GainNode>();
        gain->gain()->setValue(0.0625f);

        auto musicClipNode = std::make_shared<SampledAudioNode>();
        {
            ContextRenderLock r(context.get(), "EventsSampleApp");
            musicClipNode->setBus(r, musicClip);
        }
        context->connect(gain, musicClipNode, 0, 0);
        context->connect(context->destination(), gain, 0, 0);

        musicClipNode->setOnEnded([](){ 
            std::cout << "clip finished" << std::endl;
        });
        musicClipNode->start(0.0f);

        Wait(std::chrono::seconds(6));
    }
};
