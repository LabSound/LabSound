// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct SimpleApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeAudioContext();
        
        auto ac = context.get();
        
        std::shared_ptr<OscillatorNode> oscillator;
        SoundBuffer tonbi("samples/tonbi.wav", context->sampleRate());
        std::shared_ptr<GainNode> gain;
        std::shared_ptr<AudioBufferSourceNode> tonbiSound;

        auto lockedArea = [&](ContextGraphLock & g, ContextRenderLock & r)
        {
            oscillator = std::make_shared<OscillatorNode>(r, context->sampleRate());
            gain = std::make_shared<GainNode>(context->sampleRate());
            gain->gain()->setValue(0.0625f);

            // osc -> gain -> destination
            oscillator->connect(ac, gain.get(), 0, 0);
            gain->connect(ac, context->destination().get(), 0, 0);
            oscillator->start(0);
            oscillator->frequency()->setValue(440.f);
            oscillator->setType(r, OscillatorType::SINE);
            tonbiSound = tonbi.play(r, 0.0f);
        };

        lab::AcquireLocksForContext("Tone and Sample App", context, lockedArea);

        const int seconds = 12;
        for (int t = 0; t < seconds; ++t)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        lab::CleanupAudioContext(context);
    }
};
