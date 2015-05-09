#include "ExampleBaseApp.h"

struct InfiniteFMApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = LabSound::init();
        
        std::shared_ptr<OscillatorNode> modulator;
        std::shared_ptr<GainNode> modulatorGain;
        
        std::shared_ptr<OscillatorNode> osc;
        
        std::shared_ptr<ADSRNode> trigger;
        
        {
            ContextGraphLock g(context, "Infinite FM");
            ContextRenderLock r(context, "Infinite FM");
            
            modulator = std::make_shared<OscillatorNode>(r, context->sampleRate());
            modulator->setType(r, OscillatorType::SINE);
            modulator->start(0);
            modulator->frequency()->setValue(2.0f);
            
            modulatorGain = std::make_shared<GainNode>(context->sampleRate());
            modulatorGain->gain()->setValue(50.0f);
            
            osc = std::make_shared<OscillatorNode>(r, context->sampleRate());
            osc->setType(r, OscillatorType::SINE);
            osc->frequency()->setValue(220);
            osc->start(0);
            
            //trigger = std::make_shared<ADSRNode>(context->sampleRate());
            //trigger->set(10, 5, 30, 1, 20);

            // Set up processing chain
            // modulator > modulatorGain ---> osc frequency
            //                                osc > context
            modulator->connect(context.get(), modulatorGain.get(), 0, 0);
            modulatorGain->connect(g, osc->frequency(), 0);
            osc->connect(context.get(), context->destination().get(), 0, 0);
        }
        
        int now = 0.0;
        while(true)
        {
            
            // Debugging cruft --
            float frequency = (float) std::uniform_int_distribution<int>(20, 110)(randomgenerator);
            modulator->frequency()->setValue(frequency);
            //carrierGain->gain()->setValue(std::uniform_int_distribution<int>(2, 4)(randomgenerator));
            //modulator->frequency()->setValue(carrier->frequency()->value(context))
            //trigger->noteOn(now);
            //trigger->set((std::uniform_int_distribution<int>(1, 40)(randomgenerator)), 2.5, 30, 1.0, (std::uniform_int_distribution<int>(1, 10)(randomgenerator)));
            // --
            
            std::cout << "Modulator Frequency: " << frequency << std::endl;
            
            auto nextDelay = std::uniform_int_distribution<int>(256, 1024)(randomgenerator);
            now += nextDelay;
        
            std::this_thread::sleep_for(std::chrono::milliseconds(nextDelay));
        }

        LabSound::finish(context);
        
    }
};
