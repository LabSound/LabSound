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
            modulator->setType(r, 0, ec);
            modulator->start(0);
            
            modulatorGain = std::make_shared<GainNode>(context->sampleRate());
            modulatorGain ->gain()->setValue(4.0f);
            
            osc = std::make_shared<OscillatorNode>(r, context->sampleRate());
            osc->setType(r, 0, ec);
            osc->frequency()->setValue(220);
            osc->start(0);
            
            //trigger = std::make_shared<ADSRNode>(context->sampleRate());
            //trigger->set(10, 5, 30, 1, 20);

            // Set up processing chain
            modulator->connect(context.get(), modulatorGain.get(), 0, 0, ec);
            modulatorGain->connect(osc->frequency(), 0, ec); // I don't think this works...
            osc->connect(context.get(), context->destination().get(), 0, 0, ec);
            
            // Shouldn't be needed... 
            context->addAutomaticPullNode(modulatorGain);
            
        }
        
        int now = 0.0;
        while(true)
        {
            
            // Debugging cruft --
            modulator->frequency()->setValue(std::uniform_int_distribution<int>(20, 110)(randomgenerator));
            //carrierGain->gain()->setValue(std::uniform_int_distribution<int>(2, 4)(randomgenerator));
            //modulator->frequency()->setValue(carrier->frequency()->value(context))
            //trigger->noteOn(now);
            //trigger->set((std::uniform_int_distribution<int>(1, 40)(randomgenerator)), 2.5, 30, 1.0, (std::uniform_int_distribution<int>(1, 10)(randomgenerator)));
            // --
            
            std::cout << modulator->frequency()->value(context) << std::endl;
            
            auto nextDelay = std::uniform_int_distribution<int>(256, 1024)(randomgenerator);
            now += nextDelay;
        
            std::this_thread::sleep_for(std::chrono::milliseconds(nextDelay));
        }

        LabSound::finish(context);
        
    }
};
