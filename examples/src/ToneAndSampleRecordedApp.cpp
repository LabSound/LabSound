#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;

int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    
    auto oscillator = OscillatorNode::create(context, context->sampleRate());
    oscillator->start(0);
    
    SoundBuffer tonbi(context, "tonbi.wav");
    
    auto recorder = RecorderNode::create(context, context->sampleRate());
    recorder->startRecording();
    oscillator->connect(recorder.get(), 0, 0, ec);
    tonbi.play(recorder.get(), 0.0f);
    
    const int seconds = 3;
    for (int t = 0; t < seconds; ++t)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    recorder->stopRecording();
    std::vector<float> data;
    recorder->getData(data);
    FILE* f = fopen("labsound_example_tone_and_sample.raw", "wb");
    if (f)
    {
        fwrite(&data[0], 1, data.size(), f);
        fclose(f);
    }
    
    LabSound::finish(context);
    
    return 0;
}