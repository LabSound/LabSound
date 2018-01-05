// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct LiveGraphUpdateApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeAudioContext();
        
        auto ac = context.get();
        
        std::shared_ptr<OscillatorNode> oscillator1, oscillator2;
        std::shared_ptr<GainNode> gain;
        std::shared_ptr<AudioBufferSourceNode> tonbiSound;
        {
            ContextGraphLock g(context, "connect/disconnect");
            ContextRenderLock r(context, "connect/disconnect");

            SoundBuffer tonbi("samples/tonbi.wav", context->sampleRate());
            
            oscillator1 = std::make_shared<OscillatorNode>(r, context->sampleRate());
            oscillator2 = std::make_shared<OscillatorNode>(r, context->sampleRate());

            gain = std::make_shared<GainNode>(context->sampleRate());
            gain->gain()->setValue(0.0625f);
            
            // osc -> gain -> destination
            ac->connect(gain, oscillator1, 0, 0);
            ac->connect(context->destination(), gain, 0, 0);

            oscillator1->start(0);
            oscillator1->frequency()->setValue(220.f);
            oscillator1->setType(r, OscillatorType::SINE);

            oscillator2->setType(r, OscillatorType::SINE);

            tonbiSound = tonbi.play(r, 1.0f);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        ac->disconnect(nullptr, oscillator1, 0, 0);

        ac->connect(gain, oscillator2, 0, 0);
        ac->connect(context->destination(), gain, 0, 0);
        oscillator2->start(0);
        oscillator2->frequency()->setValue(440.f);

        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        ac->disconnect(tonbiSound, nullptr, 0, 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        lab::CleanupAudioContext(context);
    }
};
