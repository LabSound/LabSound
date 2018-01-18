// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct TremoloApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext();
        
        std::shared_ptr<OscillatorNode> modulator;
        std::shared_ptr<GainNode> modulatorGain;
        
        std::shared_ptr<OscillatorNode> osc;
        
        std::shared_ptr<ADSRNode> trigger;
        
        {
            ContextRenderLock r(context.get(), "Tremolo");
            
            modulator = std::make_shared<OscillatorNode>(context->sampleRate());
            modulator->setType(OscillatorType::SINE);
            modulator->start(0);
            modulator->frequency()->setValue(8.0f);
            
            modulatorGain = std::make_shared<GainNode>();
            modulatorGain->gain()->setValue(10);
            
            osc = std::make_shared<OscillatorNode>(context->sampleRate());
            osc->setType(OscillatorType::TRIANGLE);
            osc->frequency()->setValue(440);
            osc->start(0);
            
            // Set up processing chain
            // modulator > modulatorGain ---> osc frequency
            //                                osc > context
            context->connect(modulatorGain, modulator, 0, 0);
            context->connectParam(osc->frequency(), modulatorGain, 0);
            context->connect(context->destination(), osc, 0, 0);
        }
        
        int nowInSeconds = 0;
        while(nowInSeconds < 5)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            nowInSeconds += 1;
        }
    }
};
