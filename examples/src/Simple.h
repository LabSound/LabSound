// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct SimpleApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext();
        
        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<SampledAudioNode> musicClipNode;
        std::shared_ptr<GainNode> gain;
        std::shared_ptr<AudioBus> musicClip = MakeBusFromFile("samples/stereo-music-clip.wav", false);

        {
            ContextRenderLock r(context.get(), "Red Alert");

            oscillator = std::make_shared<OscillatorNode>(context->sampleRate());
            gain = std::make_shared<GainNode>();
            gain->gain()->setValue(0.0625f);

            musicClipNode = std::make_shared<SampledAudioNode>();
            musicClipNode->setBus(r, musicClip);
            context->connect(gain, musicClipNode, 0, 0);
            musicClipNode->start(0.0f);

            // osc -> gain -> destination
            context->connect(gain, oscillator, 0, 0);
            context->connect(context->destination(), gain, 0, 0);

            oscillator->frequency()->setValue(440.f);
            oscillator->setType(OscillatorType::SINE);
            oscillator->start(0.0f); 
        };

        const int seconds = 4;
        for (int t = 0; t < seconds; ++t)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1)); 
        }

    }
};
