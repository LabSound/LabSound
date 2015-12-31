// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct PeakCompressorApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeAudioContext();
        float sampleRate = context->sampleRate();
        
        SoundBuffer kick("samples/kick.wav", sampleRate);
        SoundBuffer hihat("samples/hihat.wav", sampleRate);
        SoundBuffer snare("samples/snare.wav", sampleRate);
        
        std::shared_ptr<BiquadFilterNode> filter;
        std::shared_ptr<PeakCompNode> peakComp;
        
        std::vector<std::shared_ptr<AudioNode>> notes;
        {
            ContextGraphLock g(context, "peak comp");
            ContextRenderLock r(context, "peak comp");
            
            filter = std::make_shared<BiquadFilterNode>(context->sampleRate());
            filter->setType(BiquadFilterNode::LOWPASS);
            filter->frequency()->setValue(2800.0f);
            
            peakComp = std::make_shared<PeakCompNode>(context->sampleRate());
            filter->connect(context.get(), peakComp.get(), 0, 0);
            peakComp->connect(context.get(), context->destination().get(), 0, 0);
            
            float startTime = 0;
            float eighthNoteTime = 1.0f / 4.0f;
            for (int bar = 0; bar < 2; bar++)
            {
                float time = startTime + bar * 8 * eighthNoteTime;
                
                notes.emplace_back(kick.play(r, filter, time));
                notes.emplace_back(kick.play(r, filter, time + 4 * eighthNoteTime));
                
                notes.emplace_back(snare.play(r, filter, time + 2 * eighthNoteTime));
                notes.emplace_back(snare.play(r, filter, time + 6 * eighthNoteTime));
                
                for (int i = 0; i < 8; ++i)
                {
                    notes.emplace_back(hihat.play(r, filter, time + i * eighthNoteTime));
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(4));
        
        lab::CleanupAudioContext(context);
    }
};
