// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct PeakCompressorApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext(lab::Channels::Stereo);

        std::shared_ptr<AudioBus> kick = MakeBusFromFile("samples/kick.wav", false);
        std::shared_ptr<AudioBus> hihat = MakeBusFromFile("samples/hihat.wav", false);
        std::shared_ptr<AudioBus> snare = MakeBusFromFile("samples/snare.wav", false);

        std::vector<std::shared_ptr<SampledAudioNode>> samples;

        std::shared_ptr<BiquadFilterNode> filter;
        std::shared_ptr<PeakCompNode> peakComp;

        {
            ContextRenderLock r(context.get(), "PeakCompressorApp");

            filter = std::make_shared<BiquadFilterNode>();
            filter->setType(lab::FilterType::LOWPASS);
            filter->frequency()->setValue(2800.0f);

            peakComp = std::make_shared<PeakCompNode>();
            context->connect(peakComp, filter, 0, 0);
            context->connect(context->destination(), peakComp, 0, 0);

            auto schedule_node = [&](std::shared_ptr<AudioBus> sample, std::shared_ptr<AudioNode> destination, const float time)
            {
                auto s = std::make_shared<SampledAudioNode>();
                s->setBus(r, sample);
                context->connect(destination, s, 0, 0);
                s->start(time);
                samples.push_back(s);
            };

            float startTime = 0.25f;
            float eighthNoteTime = 1.0f / 4.0f;
            for (int bar = 0; bar < 2; bar++)
            {
                float time = startTime + bar * 8 * eighthNoteTime;

                schedule_node(kick, filter, time);
                schedule_node(kick, filter, time + 4 * eighthNoteTime);

                schedule_node(snare, filter, time + 2 * eighthNoteTime);
                schedule_node(snare, filter, time + 6 * eighthNoteTime);

                for (int i = 0; i < 8; ++i)
                {
                    schedule_node(hihat, filter, time + i * eighthNoteTime);
                }
            }
        }

        Wait(std::chrono::seconds(10));

        // Scheduled nodes need to be explicitly cleaned up before the context
        for (auto s : samples)
        {
            s.reset();
        }

        context.reset();
    }
};
