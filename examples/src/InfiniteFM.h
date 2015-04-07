#include "ExampleBaseApp.h"

struct InfiniteFMApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = LabSound::init();
        
        std::shared_ptr<OscillatorNode> carrier;
        std::shared_ptr<GainNode> carrierGain;
        
        std::shared_ptr<OscillatorNode> modulator;
        std::shared_ptr<GainNode> gain;
        std::shared_ptr<ADSRNode> trigger;
        
        std::shared_ptr<GainNode> feedbackGain;
        std::shared_ptr<DelayNode> chainDelay;
        
        {
            ContextGraphLock g(context, "Infinite FM");
            ContextRenderLock r(context, "Infinite FM");
            
            carrier = std::make_shared<OscillatorNode>(r, context->sampleRate());
            carrier->setType(r, 0, ec);
            carrier->start(0);
            
            carrierGain = std::make_shared<GainNode>(context->sampleRate());
            carrierGain->gain()->setValue(0.0f);
            
            modulator = std::make_shared<OscillatorNode>(r, context->sampleRate());
            modulator->setType(r, 0, ec);
            modulator->start(0);
            
            gain = std::make_shared<GainNode>(context->sampleRate());
            gain->gain()->setValue(1.0f);
            
            feedbackGain = std::make_shared<GainNode>(context->sampleRate());
            feedbackGain->gain()->setValue(1.0f);
            
            trigger = std::make_shared<ADSRNode>(context->sampleRate());
            trigger->set(10, 5, 30, 1, 20);
            
            chainDelay = std::make_shared<DelayNode>(context->sampleRate(), 16.f, ec);
            
            // Set up processing chain
            
            // carrier => carrier(gain) => modulator => adsr => g
            carrier->connect(context.get(), carrierGain.get(), 0, 0, ec);
            carrier->connect(modulator->frequency(), 0, ec); // this is supposed to work!
            modulator->connect(context.get(), trigger.get(), 0, 0, ec);
            trigger->connect(context.get(), context->destination().get(), 0, 0, ec);
            
            carrierGain->connect(context.get(), context->destination().get(), 0, 0, ec);
            
            // feedback chain: g => feedback gain => delay
            //gain->connect(context.get(), feedbackGain.get(), 0, 0, ec);
            //feedbackGain->connect(context.get(), chainDelay.get(), 0, 0, ec);
            
            // g => dac
            //gain->connect(context.get(), context->destination().get(), 0, 0, ec);
            
        }
        
        int now = 0.0;
        while(true)
        {
            
            trigger->noteOn(now);
            
            carrier->frequency()->setValue(std::uniform_int_distribution<int>(20, 110)(randomgenerator));
            carrierGain->gain()->setValue(std::uniform_int_distribution<int>(1, 4)(randomgenerator));
            
            //modulator->frequency()->setValue(carrier->frequency()->value(context));
            
            trigger->set((std::uniform_int_distribution<int>(1, 40)(randomgenerator)), 5, 30, 1, 20);
            
            std::cout << carrier->frequency()->value(context) << std::endl;
            std::cout << modulator->frequency()->value(context) << std::endl;
            
            //feedbackGain->gain()->setValue(std::uniform_real_distribution<float>(0.1, .98)(randomgenerator));
            //chainDelay->gain()->setValue(std::uniform_real_distribution<float>(0.1, .98)(randomgenerator));
            
            auto nextDelay = std::uniform_int_distribution<int>(256, 1024)(randomgenerator);
            now += nextDelay;
        
            std::this_thread::sleep_for(std::chrono::milliseconds(nextDelay));
        }

        LabSound::finish(context);
        
    }
};
