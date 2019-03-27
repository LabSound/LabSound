// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"
#include <iostream>

struct SimpleApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext(lab::Channels::Stereo);

        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<SampledAudioNode> musicClipNode;
        std::shared_ptr<GainNode> gain;
        std::shared_ptr<AudioBus> musicClip = MakeBusFromFile("samples/stereo-music-clip.wav", false);
        if (!musicClip)
        {
            std::cerr << "Could not open samples/stereo-music-clip.wav" << std::endl;
            return;
        }

        oscillator = std::make_shared<OscillatorNode>(context->sampleRate());
        gain = std::make_shared<GainNode>();
        gain->gain()->setValue(0.0625f);

        musicClipNode = std::make_shared<SampledAudioNode>();
        {
            ContextRenderLock r(context.get(), "Simple");
            musicClipNode->setBus(r, musicClip);
        }
        context->connect(gain, musicClipNode, 0, 0);
        musicClipNode->start(0.0f);

        // osc -> gain -> destination
        context->connect(gain, oscillator, 0, 0);
        context->connect(context->destination(), gain, 0, 0);

        oscillator->frequency()->setValue(440.f);
        oscillator->setType(OscillatorType::SINE);
        oscillator->start(0.0f);

        Wait(std::chrono::seconds(6));
    }
};
