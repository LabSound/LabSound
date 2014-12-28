#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>
#include <iostream>

using namespace LabSound;

 // Play live audio through a reverb convolution
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    
    SoundBuffer ir(context, "impulse-responses/tim-warehouse/cardiod-rear-35-10/cardiod-rear-levelled.wav");
    //SoundBuffer ir(context, "impulse-responses/filter-telephone.wav");
    
    auto input = context->createMediaStreamSource(context, ec);
    auto convolve = context->createConvolver();
    convolve->setBuffer(ir.audioBuffer.get());
    
    auto wetGain = GainNode::create(context, context->sampleRate());
    wetGain->gain()->setValue(2.f);
    auto dryGain = GainNode::create(context, context->sampleRate());
    dryGain->gain()->setValue(1.f);
    
    input->connect(convolve.get(), 0, 0, ec);
    convolve->connect(wetGain.get(), 0, 0, ec);
    wetGain->connect(context->destination().get(), 0, 0, ec);
    dryGain->connect(context->destination().get(), 0, 0, ec);
    
    auto recorder = RecorderNode::create(context.get(), context->sampleRate());
    recorder->startRecording();
    dryGain->connect(recorder.get(), 0, 0, ec);
    wetGain->connect(recorder.get(), 0, 0, ec);
    
    const int seconds = 10;
    for (int t = 0; t < seconds; ++t)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "Done" << std::endl;
    
    recorder->stopRecording();
    
    std::vector<float> data;
    recorder->getData(data);
    FILE* f = fopen(path, "wb");
    if (f) {
        fwrite(&data[0], 1, data.size(), f);
        fclose(f);
    }
    
    LabSound::finish(context);
    
    return 0;
}