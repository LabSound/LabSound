
#include "LabSound/LabSound.h"
#include "LabSound/ConvolverNode.h"
#include "LabSound/GainNode.h"
#include "LabSound/SoundBuffer.h"

#include <iostream>
#include <thread>

using namespace LabSound;

void reverbSample(RefPtr<AudioContext> context, float seconds)
{
    ExceptionCode ec;
    //--------------------------------------------------------------
    // Play a sound file through a reverb convolution
    //
    SoundBuffer ir(context, "impulse-responses/tim-warehouse/cardiod-rear-35-10/cardiod-rear-levelled.wav");
    //SoundBuffer ir(context, "impulse-responses/filter-telephone.wav");
    
    SoundBuffer sample(context, "human-voice.mp4");
    
    RefPtr<ConvolverNode> convolve = context->createConvolver();
    convolve->setBuffer(ir.audioBuffer.get());
    RefPtr<GainNode> wetGain = context->createGain();
    wetGain->gain()->setValue(2.f);
    RefPtr<GainNode> dryGain = context->createGain();
    dryGain->gain()->setValue(1.f);
    
    convolve->connect(wetGain.get(), 0, 0, ec);
    wetGain->connect(context->destination(), 0, 0, ec);
    dryGain->connect(context->destination(), 0, 0, ec);
    dryGain->connect(convolve.get(), 0, 0, ec);
    
    sample.play(dryGain.get(), 0);
    
    std::cout << "Starting convolved echo" << std::endl;
    for (float i = 0; i < seconds; i += 0.01f) {
		std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    std::cout << "Ending echo" << std::endl;
}
