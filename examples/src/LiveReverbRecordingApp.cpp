#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>
#include <iostream>

using namespace LabSound;
using namespace std;

 // Play live audio through a reverb convolution
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    
    SoundBuffer ir("impulse-responses/tim-warehouse/cardiod-rear-35-10/cardiod-rear-levelled.wav", context->sampleRate());
    //SoundBuffer ir(context, "impulse-responses/filter-telephone.wav");

    shared_ptr<MediaStreamAudioSourceNode> input;
    shared_ptr<ConvolverNode> convolve;
    shared_ptr<GainNode> wetGain;
    shared_ptr<GainNode> dryGain;
    shared_ptr<RecorderNode> recorder;
    
    {
        ContextGraphLock g(context);
        ContextRenderLock r(context);
        input = context->createMediaStreamSource(g, r, ec);
        convolve = make_shared<ConvolverNode>(context->sampleRate());
        convolve->setBuffer(g, r, ir.audioBuffer);
        wetGain = make_shared<GainNode>(context->sampleRate());
        wetGain->gain()->setValue(2.f);
        dryGain = make_shared<GainNode>(context->sampleRate());
        dryGain->gain()->setValue(1.f);
        input->connect(g, r, convolve.get(), 0, 0, ec);
        convolve->connect(g, r, wetGain.get(), 0, 0, ec);
        wetGain->connect(g, r, context->destination().get(), 0, 0, ec);
        dryGain->connect(g, r, context->destination().get(), 0, 0, ec);
        recorder = make_shared<RecorderNode>(context->sampleRate());
        recorder->startRecording();
        dryGain->connect(g, r, recorder.get(), 0, 0, ec);
        wetGain->connect(g, r, recorder.get(), 0, 0, ec);
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    std::cout << "Done" << std::endl;
    
    recorder->stopRecording();
    
    std::vector<float> data;
    recorder->getData(data);
    FILE* f = fopen("labsound_example_livereverbapp.raw", "wb");
    if (f) {
        fwrite(&data[0], 1, data.size(), f);
        fclose(f);
    }
    
    LabSound::finish(context);
    
    return 0;
}