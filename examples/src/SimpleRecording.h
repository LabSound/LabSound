#include "ExampleBaseApp.h"

struct SimpleRecordingApp : public LabSoundExampleApp
{
    void PlayExample()
    {
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
            oscillator = std::make_shared<OscillatorNode>(r, context->sampleRate());
            oscillator->connect(ac, context->destination().get(), 0, 0);
            oscillator->connect(ac, recorder.get(), 0, 0);
            oscillator->start(0);
            oscillator->frequency()->setValue(440.f);
            oscillator->setType(r, 1);
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
    }
};
