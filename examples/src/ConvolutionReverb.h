// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct ConvolutionReverbApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeAudioContext();
        
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
            convolve = std::make_shared<ConvolverNode>(context->sampleRate());
            convolve->setBuffer(g, impulseResponse.audioBuffer);
            wetGain = std::make_shared<GainNode>(context->sampleRate());
            wetGain->gain()->setValue(1.15f);
            dryGain = std::make_shared<GainNode>(context->sampleRate());
            dryGain->gain()->setValue(0.75f);
            
            convolve->connect(context.get(), wetGain.get(), 0, 0);
            wetGain->connect(context.get(), context->destination().get(), 0, 0);
            dryGain->connect(context.get(), context->destination().get(), 0, 0);
            dryGain->connect(context.get(), convolve.get(), 0, 0);
            
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
