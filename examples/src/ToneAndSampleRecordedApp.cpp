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
    auto ac = context.get();
    
    std::shared_ptr<OscillatorNode> oscillator;
    SoundBuffer tonbi("samples/tonbi.wav", context->sampleRate());
    std::shared_ptr<AudioBufferSourceNode> tonbiSound;
    auto recorder = std::make_shared<RecorderNode>(context->sampleRate());
    context->addAutomaticPullNode(recorder);
    recorder->startRecording();
    {
        ContextGraphLock g(context, "tone and sample");
        ContextRenderLock r(context, "tone and sample");
        oscillator = make_shared<OscillatorNode>(r, context->sampleRate());
        oscillator->connect(ac, context->destination().get(), 0, 0, ec);
        oscillator->connect(ac, recorder.get(), 0, 0, ec);
        oscillator->start(0);   // play now
        oscillator->frequency()->setValue(440.f);
        oscillator->setType(r, 1, ec);
        tonbiSound = tonbi.play(r, recorder, 0.0f);
        tonbiSound = tonbi.play(r, 0.0f);
    }
    
    const int seconds = 4;
    for (int t = 0; t < seconds; ++t)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    recorder->stopRecording();
    context->removeAutomaticPullNode(recorder);
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
