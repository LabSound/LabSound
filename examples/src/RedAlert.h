// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"
#include <cmath>

struct RedAlertApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext();
        
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
            ContextRenderLock r(context.get(), "Red Alert");
            
            sweep = std::make_shared<FunctionNode>(1);
            sweep->setFunction([](ContextRenderLock & r, FunctionNode * me, int channel, float * values, size_t framesToProcess)
            {
                double dt = 1.0 / r.context()->sampleRate();
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
                        values[i] = std::sqrt((float) now * 1.f / 0.9f) * 487.f + 360.f;
                    }
                    
                    now += dt;
                }
            });
            
            sweep->start(0);
            
            outputGainFunction = std::make_shared<FunctionNode>(1);
            outputGainFunction->setFunction([](ContextRenderLock & r, FunctionNode * me, int channel, float * values, size_t framesToProcess)
            {
                double dt = 1.0 / r.context()->sampleRate();
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

            osc = std::make_shared<OscillatorNode>(context->sampleRate());
            osc->setType(OscillatorType::SAWTOOTH);
            osc->frequency()->setValue(220);
            osc->start(0);
            oscGain = std::make_shared<GainNode>();
            oscGain->gain()->setValue(0.5f);
            
            resonator = std::make_shared<OscillatorNode>(context->sampleRate());
            resonator->setType(OscillatorType::SINE);
            resonator->frequency()->setValue(220);
            resonator->start(0);
            
            resonatorGain = std::make_shared<GainNode>();
            resonatorGain->gain()->setValue(0.0f);

            resonanceSum = std::make_shared<GainNode>();
            resonanceSum->gain()->setValue(0.5f);
            
            // sweep drives oscillator frequency
            context->connectParam(osc->frequency(), sweep, 0);
            
            // oscillator drives resonator frequency
            context->connectParam(resonator->frequency(), osc, 0);

            // osc --> oscGain -------------+
            // resonator -> resonatorGain --+--> resonanceSum
            context->connect(oscGain, osc, 0, 0);
            context->connect(resonanceSum, oscGain, 0, 0);
            context->connect(resonatorGain, resonator, 0, 0);
            context->connect(resonanceSum, resonatorGain, 0, 0);
            
            delaySum = std::make_shared<GainNode>();
            delaySum->gain()->setValue(0.2f);
            
            // resonanceSum --+--> delay0 --+
            //                +--> delay1 --+
            //                + ...    .. --+
            //                +--> delay4 --+---> delaySum
            float delays[5] = {0.015f, 0.022f, 0.035f, 0.024f, 0.011f};
            for (int i = 0; i < 5; ++i) 
            {
                delay[i] = std::make_shared<DelayNode>(context->sampleRate(), 0.04f);
                delay[i]->delayTime()->setValue(delays[i]);
                context->connect(delay[i], resonanceSum, 0, 0);
                context->connect(delaySum, delay[i], 0, 0);
            }
            
            filterSum = std::make_shared<GainNode>();
            filterSum->gain()->setValue(0.2f);
            
            // delaySum --+--> filter0 --+
            //            +--> filter1 --+
            //            +--> filter2 --+
            //            +--> filter3 --+
            //            +--------------+----> filterSum
            //
            context->connect(filterSum, delaySum, 0, 0);

            float centerFrequencies[4] = {740.f, 1400.f, 1500.f, 1600.f};
            for (int i = 0; i < 4; ++i) 
            {
                filter[i] = std::make_shared<BiquadFilterNode>();
                filter[i]->frequency()->setValue(centerFrequencies[i]);
                filter[i]->q()->setValue(12.f);
                context->connect(filter[i], delaySum, 0, 0);
                context->connect(filterSum, filter[i], 0, 0);
            }
            
            // filterSum --> destination
            context->connectParam(filterSum->gain(), outputGainFunction, 0);
            context->connect( context->destination(), filterSum, 0, 0);
        }
        
        int now = 0;
        while (now < 10000)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            now += 1000;
        }
       
    }
};
