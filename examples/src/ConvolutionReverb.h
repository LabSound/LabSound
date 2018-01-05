// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct ConvolutionReverbApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext();
        
        SoundBuffer impulseResponse("impulse/cardiod-rear-levelled.wav", context->sampleRate());
        //SoundBuffer impulseResponse("impulse/filter-telephone.wav", context->sampleRate()); // alternate
        
        SoundBuffer sample("samples/voice.ogg", context->sampleRate());
        std::shared_ptr<ConvolverNode> convolve;
        std::shared_ptr<GainNode> wetGain;
        std::shared_ptr<GainNode> dryGain;
        std::shared_ptr<AudioNode> voice;
        
        {
            ContextGraphLock g(context, "ConvolutionReverbApp");
            ContextRenderLock r(context, "ConvolutionReverbApp");

            auto ac = context.get();

            convolve = std::make_shared<ConvolverNode>(context->sampleRate());
            convolve->setBuffer(g, impulseResponse.audioBuffer);
            wetGain = std::make_shared<GainNode>(context->sampleRate());
            wetGain->gain()->setValue(1.15f);
            dryGain = std::make_shared<GainNode>(context->sampleRate());
            dryGain->gain()->setValue(0.75f);
            
            ac->connect(wetGain, convolve, 0, 0);
            ac->connect(context->destination(), wetGain, 0, 0);
            ac->connect(context->destination(), dryGain, 0, 0);
            ac->connect(convolve, dryGain, 0, 0);
            
            voice = sample.play(r, dryGain, 0);
        }
        
        const int seconds = 10;
        for (int t = 0; t < seconds; ++t)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        lab::CleanupAudioContext(context);
    }
};
