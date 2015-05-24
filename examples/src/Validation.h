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
        
		std::array<int, 8> majorScale = {0, 2, 4, 5, 7, 9, 11, 12};
		std::array<int, 8> naturalMinorScale = {0, 2, 3, 5, 7, 9, 11, 12};
		std::array<int, 6> pentatonicMajor = { 0, 2, 4, 7, 9, 12 };
		std::array<int, 8> pentatonicMinor = { 0, 3, 5, 7, 10, 12 };
		std::array<int, 8> delayTimes = { 266, 533, 399 };
    
        auto randomFloat = std::uniform_real_distribution<float>(0, 1);
        auto randomScaleDegree = std::uniform_int_distribution<int>(0, pentatonicMajor.size() - 1);
        auto randomTimeIndex = std::uniform_int_distribution<int>(0, delayTimes.size() - 1);
        
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
            sinOsc->connect(context.get(), limiter.get(), 0, 0); // Connect sinOsc to gain
            triOsc->connect(context.get(), limiter.get(), 0, 0); // Connect triOsc to gain
            //megaSuperSaw->connect(context.get(), limiter.get(), 0, 0); // Connect megaSuperSaw to gain
            
            limiter->connect(context.get(), context->destination().get(), 0, 0); // connect gain to DAC
            panner->connect(context.get(), context->destination().get(), 0, 0);  // connect panner to DAC
            
            //megaSuperSaw->noteOn(0);
            
            sinOsc->setType(r, OscillatorType::SINE);
            sinOsc->start(0);
            
            triOsc->setType(r, OscillatorType::TRIANGLE);
            triOsc->start(0);
            
            
            context->listener()->setPosition(0, 1, 0);
            panner->setVelocity(15, 3, 2);
            
            //SoundBuffer tonbi("samples/tonbi.wav", context->sampleRate());
            //tonbiSound = tonbi.play(r, 0.0f);
            
        }
        
        
        float elapsedTime = 0;
        for (int s = 0; s < 48; s++)
        {
            int octaveOffset = 52;
            float a = MidiToFrequency(pentatonicMajor[randomScaleDegree(randomgenerator)] + octaveOffset);
            float b = MidiToFrequency(pentatonicMajor[randomScaleDegree(randomgenerator)] + octaveOffset);
            
            float delayTime = elapsedTime + (delayTimes[randomTimeIndex(randomgenerator)] / 1000.f);
            
            std::cout << delayTime << std::endl;
            
            sinOsc->frequency()->setValueAtTime(a, delayTime * 2);
            triOsc->frequency()->setValueAtTime(b, delayTime);
            
            elapsedTime = delayTime;
        }
     
        //const int seconds = 2;
        //float halfTime = seconds * 0.5f;
        //for (float i = 0; i < seconds; i += 0.01f)
        //{
        //    float x = (i - halfTime) / halfTime;
        //    panner->setPosition(x, 0.1f, 0.1f);
        //    std::this_thread::sleep_for(std::chrono::milliseconds(10));
        //}
        
        std::this_thread::sleep_for(std::chrono::seconds((int)elapsedTime));
        LabSound::finish(context);
        
    }
    
    /*
    void PlayExample()
    {
        auto context = LabSound::init();
        float sampleRate = context->sampleRate();
    
        SoundBuffer sample("samples/voice.ogg", sampleRate);
        
        std::shared_ptr<GainNode> outGain;
        std::shared_ptr<DynamicsCompressorNode> compressor;
        std::shared_ptr<AudioBufferSourceNode> player;
        std::shared_ptr<StereoPannerNode> stereoPanner;
        std::shared_ptr<OscillatorNode> osc;

        {
            ContextGraphLock g(context, "dalek voice");
            ContextRenderLock r(context, "dalek voice");
            AudioContext * ac = context.get();
            
            stereoPanner = std::make_shared<StereoPannerNode>(context->sampleRate());
            osc = std::make_shared<OscillatorNode>(r, context->sampleRate());
            
            player = sample.create(r, sampleRate);
            if (!player)
            {
                std::cerr << "Sample buffer wasn't loaded" << std::endl;
                return;
            }
            
            outGain = std::make_shared<GainNode>(sampleRate);
            outGain->gain()->setValue(1.0f);

            compressor = std::make_shared<DynamicsCompressorNode>(sampleRate);
            compressor->threshold()->setValue(-12.0f);

            //player->connect(ac, compressor.get(), 0, 0);
            
            osc->connect(ac, compressor.get(), 0, 0);
            
            compressor->connect(ac, outGain.get(), 0, 0);
            
            outGain->connect(ac, context->destination().get(), 0, 0);
            
            osc->start(0);
            player->start(0);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        LabSound::finish(context);
        
    }
     */

};
