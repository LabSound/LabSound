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
            ContextGraphLock g(context, "SimpleRecordingApp");
            ContextRenderLock r(context, "SimpleRecordingApp");
            
            oscillator = std::make_shared<OscillatorNode>(r, context->sampleRate());
            oscillator->connect(ac, context->destination().get(), 0, 0);
            oscillator->connect(ac, recorder.get(), 0, 0);

            oscillator->frequency()->setValue(440.f);
            oscillator->setType(r, OscillatorType::SINE);
            
            tonbiSound = tonbi.play(r, recorder, 0.0f);
            tonbiSound = tonbi.play(r, 0.0f);
            oscillator->start(0);
            
        }
        
        const int seconds = 4;
        for (int t = 0; t < seconds; ++t)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        recorder->stopRecording();
        context->removeAutomaticPullNode(recorder);
        
        recorder->writeRecordingToWav(1, "SimpleRecordingApp.wav");
        
        LabSound::finish(context);
    }
};
