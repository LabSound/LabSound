// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct LiveGraphUpdateApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        std::shared_ptr<OscillatorNode> oscillator1, oscillator2;
        std::shared_ptr<GainNode> gain;
        std::shared_ptr<SampledAudioNode> tonbiSoundNode;
        std::shared_ptr<AudioBus> tonbi;

        {
            std::unique_ptr<AudioContext> context = lab::MakeRealtimeAudioContext();

            lab::AcquireLocksForContext("Tone and Sample App", context.get(), [&](ContextGraphLock & g, ContextRenderLock & r)
            {
                tonbi = MakeBusFromFile("samples/tonbi.wav", false);

                oscillator1 = std::make_shared<OscillatorNode>(context->sampleRate());
                oscillator2 = std::make_shared<OscillatorNode>(context->sampleRate());

                gain = std::make_shared<GainNode>();
                gain->gain()->setValue(0.25);

                tonbiSoundNode = std::make_shared<SampledAudioNode>();
                tonbiSoundNode->setBus(r, tonbi);

                // osc -> gain -> destination
                context->connect(gain, oscillator1, 0, 0);
                context->connect(context->destination(), gain, 0, 0);

                oscillator1->start(0);
                oscillator1->frequency()->setValue(220.f);
                oscillator1->setType(OscillatorType::SINE);

                oscillator2->setType(OscillatorType::SINE);

                context->connect(gain, tonbiSoundNode, 0, 0);
                tonbiSoundNode->start(0.0f);
            });

            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            context->disconnect(nullptr, oscillator1, 0, 0);

            tonbiSoundNode->stop(1.0f);

            context->connect(gain, oscillator2, 0, 0);
            context->connect(context->destination(), gain, 0, 0);
            oscillator2->start(0);
            oscillator2->frequency()->setValue(440.f);

            std::this_thread::sleep_for(std::chrono::milliseconds(1500));

            context->disconnect(tonbiSoundNode, nullptr, 0, 0);

            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }

        std::cout << "OscillatorNode 1 use_count: " << oscillator1.use_count() << std::endl;
        std::cout << "OscillatorNode 2 use_count: " << oscillator2.use_count() << std::endl;
        std::cout << "GainNode use_count:         " << gain.use_count() << std::endl;
        std::cout << "AudioBus use_count:         " << tonbi.use_count() << std::endl;
        std::cout << "SampledAudioNode use_count: " << tonbiSoundNode.use_count() << std::endl;
    }
};
