// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

/* 
     Adapted from this ChucK patch:
 
     SinOsc a => SinOsc b => ADSR chainADSR => Gain g => Gain feedbackGain => DelayL chainDelay => g => dac;
     
     1 => b.gain;
     1 => b.freq;
     
     0.25 => chainADSR.gain;
     
     while(true)
     {
         Std.rand2(1, 100000) => a.gain;
         Std.rand2(1, 400) => a.freq;
         
         chainADSR.set(Std.rand2(1,100)::ms, Std.rand2(1,100)::ms, 0, 0::ms);
         chainADSR.keyOn();
         
         Std.rand2(1, 500)::ms => now;
     }
 */


// @tofix - This doesn't match the expected sonic result of the ChucK patch. Presumably
// it is because LabSound oscillators are wavetable generated?
struct InfiniteFMApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeAudioContext();
        
        std::shared_ptr<OscillatorNode> modulator;
        std::shared_ptr<GainNode> modulatorGain;
        std::shared_ptr<OscillatorNode> osc;
        std::shared_ptr<ADSRNode> trigger;
    
        std::shared_ptr<GainNode> signalGain;
        std::shared_ptr<GainNode> feedbackTap;
        std::shared_ptr<DelayNode> chainDelay;
        
        {
            ContextGraphLock g(context, "Infinite FM");
            ContextRenderLock r(context, "Infinite FM");
            
            modulator = std::make_shared<OscillatorNode>(r, context->sampleRate());
            modulator->setType(r, OscillatorType::SQUARE);
            modulator->start(0);
            
            modulatorGain = std::make_shared<GainNode>(context->sampleRate());
            
            osc = std::make_shared<OscillatorNode>(r, context->sampleRate());
            osc->setType(r, OscillatorType::SQUARE);
            osc->frequency()->setValue(220);
            osc->start(0);
            
            trigger = std::make_shared<ADSRNode>(context->sampleRate());

            signalGain = std::make_shared<GainNode>(context->sampleRate());
            signalGain->gain()->setValue(1.0f);
            
            feedbackTap = std::make_shared<GainNode>(context->sampleRate());
            feedbackTap->gain()->setValue(0.5f);
            
            chainDelay = std::make_shared<DelayNode>(context->sampleRate(), 4);
            chainDelay->delayTime()->setValue(0.0f); // passthrough delay, not sure if this has the same DSP semantic as ChucK
            
            // Set up processing chain:
            modulator->connect(context.get(), modulatorGain.get(), 0, 0);           // Modulator to Gain
            modulatorGain->connect(g, osc->frequency(), 0);                         // Gain to frequency parameter
            osc->connect(context.get(), trigger.get(), 0, 0);                       // Osc to ADSR
            trigger->connect(context.get(), signalGain.get(), 0, 0);                // ADSR to signalGain

            signalGain->connect(context.get(), feedbackTap.get(), 0, 0);            // Signal to Feedback
            feedbackTap->connect(context.get(), chainDelay.get(), 0, 0);            // Feedback to Delay
            
            chainDelay->connect(context.get(), signalGain.get(), 0, 0);             // Delay to signalGain
            
            signalGain->connect(context.get(), context->destination().get(), 0, 0); // signalGain to DAC
        }
        
        int now = 0.0;
        while(true)
        {
            // Debugging cruft --
            float f = (float) std::uniform_int_distribution<int>(2, 48)(randomgenerator);
            modulator->frequency()->setValue(f);
            std::cout << "Modulator Frequency: " << f << std::endl;
            
            float g = (float) std::uniform_int_distribution<int>(64, 1024)(randomgenerator);
            modulatorGain->gain()->setValue(g);
            std::cout << "Gain: " << g << std::endl;
            
            trigger->noteOn(now);
            trigger->set((std::uniform_int_distribution<int>(1, 64)(randomgenerator)), 0.5 , 0, 0, 0);

            auto nextDelay = std::uniform_int_distribution<int>(128, 1024)(randomgenerator);
            now += nextDelay;
        
            std::this_thread::sleep_for(std::chrono::milliseconds(nextDelay));
        }

        lab::CleanupAudioContext(context);
        
    }
};
