#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;

  // Play a sound file through a reverb convolution
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();

    SoundBuffer ir(context, "impulse-responses/tim-warehouse/cardiod-rear-35-10/cardiod-rear-levelled.wav");
    //SoundBuffer ir(context, "impulse-responses/filter-telephone.wav");
    
    SoundBuffer sample(context, "human-voice.mp4");
    
    auto convolve = ConvolverNode::create(context, context->sampleRate());
    convolve->setBuffer(ir.audioBuffer.get());
    auto wetGain = GainNode::create(context, context->sampleRate());
    wetGain->gain()->setValue(2.f);
    auto dryGain = GainNode::create(context, context->sampleRate());
    dryGain->gain()->setValue(1.f);
    
    convolve->connect(wetGain.get(), 0, 0, ec);
    wetGain->connect(context->destination().get(), 0, 0, ec);
    dryGain->connect(context->destination().get(), 0, 0, ec);
    dryGain->connect(convolve.get(), 0, 0, ec);
    
    sample.play(dryGain.get(), 0);
    
    const int seconds = 10;
    for (int t = 0; t < seconds; ++t)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    LabSound::finish(context);
    
    return 0;
}