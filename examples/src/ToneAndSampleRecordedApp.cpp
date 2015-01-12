#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;
using namespace std;

int main(int, char**)
{
    ExceptionCode ec;
    auto context = LabSound::init();
    std::shared_ptr<OscillatorNode> oscillator;
    SoundBuffer tonbi("tonbi.wav", context->sampleRate());
    std::shared_ptr<AudioBufferSourceNode> tonbiSound;
    auto recorder = std::make_shared<RecorderNode>(context->sampleRate());
    recorder->startRecording();
    {
        ContextGraphLock g(context);
        ContextRenderLock r(context);
        oscillator = make_shared<OscillatorNode>(r, context->sampleRate());
        oscillator->connect(g, r, context->destination().get(), 0, 0, ec);
        oscillator->start(r, 0);   // play now
        oscillator->frequency()->setValue(440.f);
        oscillator->setType(r, 1, ec);
        tonbiSound = tonbi.play(g, r, recorder, 0.0f);
        oscillator->connect(g,r, recorder.get(), 0, 0, ec);
    }
    
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
