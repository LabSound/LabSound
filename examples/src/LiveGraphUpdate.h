// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct LiveGraphUpdateApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        std::shared_ptr<OscillatorNode> oscillator1, oscillator2;
        std::shared_ptr<GainNode> gain;
        auto recorder = std::make_shared<RecorderNode>();

        {
            std::unique_ptr<AudioContext> context = lab::MakeRealtimeAudioContext();

            lab::AcquireLocksForContext("connect-disconnect", context.get(), [&](ContextGraphLock & g, ContextRenderLock & r)
            {
                context->addAutomaticPullNode(recorder);

                oscillator1 = std::make_shared<OscillatorNode>(context->sampleRate());
                //oscillator2 = std::make_shared<OscillatorNode>(context->sampleRate());

                gain = std::make_shared<GainNode>();
                gain->gain()->setValue(0.50);

                // osc -> gain -> destination
                context->connect(gain, oscillator1, 0, 0);
                //context->connect(gain, oscillator2, 0, 0);

                context->connect(context->destination(), gain, 0, 0);
                context->connect(recorder, gain, 0, 0);

                oscillator1->setType(OscillatorType::SINE);
                oscillator1->frequency()->setValue(220.f);
                oscillator1->start(0.00f);
                    
                //oscillator2->setType(OscillatorType::SINE);
                //oscillator2->frequency()->setValue(440.f);
                //oscillator2->start(0.00f);

                recorder->startRecording();
            });

            for (int i = 0; i < 4; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                context->connect(gain, oscillator1, 0, 0);


                //context->connect(gain, oscillator2, 0, 0);
                //std::this_thread::sleep_for(std::chrono::milliseconds(500));


                //context->disconnect(nullptr, oscillator2, 0, 0);
                //context->disconnect(nullptr, oscillator1, 0, 0);
                //std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            context->disconnect(nullptr, oscillator1, 0, 0);
            context->disconnect(nullptr, oscillator2, 0, 0);

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            recorder->stopRecording();

            recorder->writeRecordingToWav(1, "click-click.wav");

            context.reset();
        }

        std::cout << "OscillatorNode 1 use_count: " << oscillator1.use_count() << std::endl;
        std::cout << "OscillatorNode 2 use_count: " << oscillator2.use_count() << std::endl;
        std::cout << "GainNode use_count:         " << gain.use_count() << std::endl;
    }
};
