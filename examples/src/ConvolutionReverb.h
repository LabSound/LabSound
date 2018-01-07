// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct ConvolutionReverbApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext();
        
        std::shared_ptr<AudioBus> impulseResponseClip = MakeBusFromFile("impulse/cardiod-rear-levelled.wav", false);
        std::shared_ptr<AudioBus> voiceClip = MakeBusFromFile("samples/voice.ogg", false);

        std::shared_ptr<ConvolverNode> convolve;
        std::shared_ptr<GainNode> wetGain;
        std::shared_ptr<GainNode> dryGain;
        std::shared_ptr<SampledAudioNode> voiceNode;
        
        {
            ContextGraphLock g(context.get(), "ConvolutionReverbApp");
            ContextRenderLock r(context.get(), "ConvolutionReverbApp");

            auto ac = context.get();

            convolve = std::make_shared<ConvolverNode>();
            convolve->setImpulse(impulseResponseClip);

            wetGain = std::make_shared<GainNode>();
            wetGain->gain()->setValue(1.15f);
            dryGain = std::make_shared<GainNode>();
            dryGain->gain()->setValue(0.75f);
            
            ac->connect(wetGain, convolve, 0, 0);
            ac->connect(context->destination(), wetGain, 0, 0);
            ac->connect(context->destination(), dryGain, 0, 0);
            ac->connect(convolve, dryGain, 0, 0);
            
            voiceNode = std::make_shared<SampledAudioNode>();
            voiceNode->setBus(r, voiceClip);
            context->connect(dryGain, voiceNode, 0, 0);
            voiceNode->start(0.0f);
        }
        
        const int seconds = 10;
        for (int t = 0; t < seconds; ++t)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
