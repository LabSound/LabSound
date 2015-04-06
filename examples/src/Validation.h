#ifdef _MSC_VER
#include <windows.h>
std::string PrintCurrentDirectory()
{
    char buffer[MAX_PATH] = { 0 };
    GetCurrentDirectory(MAX_PATH, buffer);
    return std::string(buffer);
}
#endif

#include "ExampleBaseApp.h"

// An example with a bunch of nodes to verify api + functionality changes/improvements/regressions
struct ValidationApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        
        //std::cout << "Current Directory: " << PrintCurrentDirectory() << std::endl;
        
        auto context = LabSound::init();
        
        std::shared_ptr<OscillatorNode> sinOsc;
        std::shared_ptr<OscillatorNode> triOsc;
        std::shared_ptr<GainNode> limiter;
        std::shared_ptr<AudioBufferSourceNode> tonbiSound;
        std::shared_ptr<PannerNode> panner;
        std::shared_ptr<SupersawNode> megaSuperSaw;
        
        {
            ContextGraphLock g(context, "Validator");
            ContextRenderLock r(context, "Validator");
            
            sinOsc = std::make_shared<OscillatorNode>(r, context->sampleRate());
            triOsc = std::make_shared<OscillatorNode>(r, context->sampleRate());
            megaSuperSaw = std::make_shared<SupersawNode>(r, context->sampleRate());
            
            limiter = std::make_shared<GainNode>(context->sampleRate());
            panner = std::make_shared<PannerNode>(context->sampleRate());
            
            limiter->gain()->setValue(0.5f);
            sinOsc->connect(context.get(), limiter.get(), 0, 0, ec); // Connect sinOsc to gain
            triOsc->connect(context.get(), limiter.get(), 0, 0, ec); // Connect triOsc to gain
            megaSuperSaw->connect(context.get(), limiter.get(), 0, 0, ec); // Connect megaSuperSaw to gain
            
            limiter->connect(context.get(), context->destination().get(), 0, 0, ec); // connect gain to DAC
            panner->connect(context.get(), context->destination().get(), 0, 0, ec);  // connect panner to DAC
            
            megaSuperSaw->noteOn(0);
            
            sinOsc->setType(r, 0, ec);
            sinOsc->start(1);
            
            triOsc->setType(r, 1, ec);
            triOsc->start(2);
            
            sinOsc->stop(3);
            triOsc->stop(4);
            
            context->listener()->setPosition(0, 1, 0);
            panner->setVelocity(15, 3, 2);
            
            //SoundBuffer tonbi("samples/tonbi.wav", context->sampleRate());
            //tonbiSound = tonbi.play(r, 0.0f);
            
        }
        
        float f = 220.f;
        for (int s = 0; s < 4; s++)
        {
            f *= 1.25;
            sinOsc->frequency()->setValueAtTime(f, s);
            triOsc->frequency()->setValueAtTime(f, s / 2);
        }
        
        const int seconds = 2;
        float halfTime = seconds * 0.5f;
        for (float i = 0; i < seconds; i += 0.01f)
        {
            float x = (i - halfTime) / halfTime;
            panner->setPosition(x, 0.1f, 0.1f);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(4));
        LabSound::finish(context);
        
    }
};
