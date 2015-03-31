#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;
using namespace std;

  // Play a sound file through a reverb convolution
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();

    SoundBuffer ir("impulse-responses/tim-warehouse/cardiod-rear-35-10/cardiod-rear-levelled.wav", context->sampleRate());
    //SoundBuffer ir("impulse-responses/filter-telephone.wav", context->sampleRate());
    
    SoundBuffer sample("human-voice.mp4", context->sampleRate());
    shared_ptr<ConvolverNode> convolve;
    shared_ptr<GainNode> wetGain;
    shared_ptr<GainNode> dryGain;
    shared_ptr<AudioNode> voice;
    
    {
        ContextGraphLock g(context, "reverb");
        ContextRenderLock r(context, "reverb");
        convolve = make_shared<ConvolverNode>(context->sampleRate());
        convolve->setBuffer(ir.audioBuffer);
        wetGain = make_shared<GainNode>(context->sampleRate());
        wetGain->gain()->setValue(2.f);
        dryGain = make_shared<GainNode>(context->sampleRate());
        dryGain->gain()->setValue(1.f);
        
        convolve->connect(context.get(), wetGain.get(), 0, 0, ec);
        wetGain->connect(context.get(), context->destination().get(), 0, 0, ec);
        dryGain->connect(context.get(), context->destination().get(), 0, 0, ec);
        dryGain->connect(context.get(), convolve.get(), 0, 0, ec);
        
        voice = sample.play(g, r, dryGain, 0);
    }
    
    const int seconds = 20;
    for (int t = 0; t < seconds; ++t)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    LabSound::finish(context);
    return 0;
}
