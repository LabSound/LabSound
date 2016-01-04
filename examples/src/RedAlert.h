// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct RedAlertApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeAudioContext();
        
        std::shared_ptr<FunctionNode> sweep;
        std::shared_ptr<FunctionNode> outputGainFunction;
        
        std::shared_ptr<OscillatorNode> osc;
        std::shared_ptr<GainNode> oscGain;
        std::shared_ptr<OscillatorNode> resonator;
        std::shared_ptr<GainNode> resonatorGain;
        std::shared_ptr<GainNode> resonanceSum;
        
        std::shared_ptr<DelayNode> delay[5];
        
        std::shared_ptr<GainNode> delaySum;
        std::shared_ptr<GainNode> filterSum;
        
        std::shared_ptr<BiquadFilterNode> filter[5];
        
        {
            ContextGraphLock g(context, "Red Alert");
            ContextRenderLock r(context, "Red Alert");
            
            sweep = std::make_shared<FunctionNode>(context->sampleRate(), 1);
            sweep->setFunction([](ContextRenderLock & r, FunctionNode * me, int channel, float * values, size_t framesToProcess)
            {
                double dt = 1.0 / me->sampleRate();
                double now = fmod(me->now(), 1.2f);
                
                for (size_t i = 0; i < framesToProcess; ++i) 
                {
                    //0 to 1 in 900 ms with a 1200ms gap in between
                    if (now > 0.9)
                    {
                        values[i] = 487.f + 360.f;
                    }
                    else 
                    {
                        values[i] = sqrt(now * 1.f / 0.9f) * 487.f + 360.f;
                    }
                    
                    now += dt;
                }
            });
            
            sweep->start(0);
            
            outputGainFunction = std::make_shared<FunctionNode>(context->sampleRate(), 1);
            outputGainFunction->setFunction([](ContextRenderLock & r, FunctionNode * me, int channel, float * values, size_t framesToProcess)
            {
                double dt = 1.0 / me->sampleRate();
                double now = fmod(me->now(), 1.2f);
                
                for (size_t i = 0; i < framesToProcess; ++i)
                {
                    //0 to 1 in 900 ms with a 1200ms gap in between
                    if (now > 0.9)
                    {
                        values[i] = 0;
                    }
                    else
                    {
                        values[i] = 0.333f;
                    }
                    
                    now += dt;
                }
            });
            
            outputGainFunction->start(0);

            osc = std::make_shared<OscillatorNode>(r, context->sampleRate());
            osc->setType(r, OscillatorType::SAWTOOTH);
            osc->frequency()->setValue(220);
            osc->start(0);
            oscGain = std::make_shared<GainNode>(context->sampleRate());
            oscGain->gain()->setValue(0.5f);
            
            resonator = std::make_shared<OscillatorNode>(r, context->sampleRate());
            resonator->setType(r, OscillatorType::SINE);
            resonator->frequency()->setValue(220);
            resonator->start(0);
            
            resonatorGain = std::make_shared<GainNode>(context->sampleRate());
            resonatorGain->gain()->setValue(0.0f);

            resonanceSum = std::make_shared<GainNode>(context->sampleRate());
            resonanceSum->gain()->setValue(0.5f);
            
            // sweep drives oscillator frequency
            sweep->connect(g, osc->frequency(), 0);
            
            // oscillator drives resonator frequency
            osc->connect(g, resonator->frequency(), 0);

            // osc --> oscGain -------------+
            // resonator -> resonatorGain --+--> resonanceSum
            //
            osc->connect(context.get(), oscGain.get(), 0, 0);
            oscGain->connect(context.get(), resonanceSum.get(), 0, 0);
            resonator->connect(context.get(), resonatorGain.get(), 0, 0);
            resonatorGain->connect(context.get(), resonanceSum.get(), 0, 0);
            
            delaySum = std::make_shared<GainNode>(context->sampleRate());
            delaySum->gain()->setValue(0.2f);
            
            // resonanceSum --+--> delay0 --+
            //                +--> delay1 --+
            //                + ...    .. --+
            //                +--> delay4 --+---> delaySum
            //
            float delays[5] = {0.015, 0.022, 0.035, 0.024, 0.011};
            for (int i = 0; i < 5; ++i) {
                delay[i] = std::make_shared<DelayNode>(context->sampleRate(), 0.04f);
                delay[i]->delayTime()->setValue(delays[i]);
                resonanceSum->connect(context.get(), delay[i].get(), 0, 0);
                delay[i]->connect(context.get(), delaySum.get(), 0, 0);
            }
            
            filterSum = std::make_shared<GainNode>(context->sampleRate());
            filterSum->gain()->setValue(0.2f);
            
            // delaySum --+--> filter0 --+
            //            +--> filter1 --+
            //            +--> filter2 --+
            //            +--> filter3 --+
            //            +--------------+----> filterSum
            //
            delaySum->connect(context.get(), filterSum.get(), 0, 0);

            float centerFrequencies[4] = {740.f, 1400.f, 1500.f, 1600.f};
            for (int i = 0; i < 4; ++i) {
                filter[i] = std::make_shared<BiquadFilterNode>(context->sampleRate());
                filter[i]->frequency()->setValue(centerFrequencies[i]);
                filter[i]->q()->setValue(12.f);
                delaySum->connect(context.get(), filter[i].get(), 0, 0);
                filter[i]->connect(context.get(), filterSum.get(), 0, 0);
            }
            
            // filterSum --> destination
            //
            outputGainFunction->connect(g, filterSum->gain(), 0);
            filterSum->connect(context.get(), context->destination().get(), 0, 0);
        }
        
        int now = 0.0;
        while(now < 10000)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            now += 1000;
        }
        
        lab::CleanupAudioContext(context);
    }
};
