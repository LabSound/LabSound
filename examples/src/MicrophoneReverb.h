#include "ExampleBaseApp.h"

struct MicrophoneReverbApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = LabSound::init();
        auto ac = context.get();
        
        SoundBuffer ir("impulse/cardiod-rear-levelled.wav", context->sampleRate());
        
        std::shared_ptr<MediaStreamAudioSourceNode> input;
        std::shared_ptr<ConvolverNode> convolve;
        std::shared_ptr<GainNode> wetGain;
        std::shared_ptr<GainNode> dryGain;
        std::shared_ptr<RecorderNode> recorder;
        
        {
            ContextGraphLock g(context, "live reverb recording");
            ContextRenderLock r(context, "live reverb recording");
            input = context->createMediaStreamSource(g, r);
            convolve = std::make_shared<ConvolverNode>(context->sampleRate());
            convolve->setBuffer(ir.audioBuffer);
            wetGain = std::make_shared<GainNode>(context->sampleRate());
            wetGain->gain()->setValue(2.f);
            dryGain = std::make_shared<GainNode>(context->sampleRate());
            dryGain->gain()->setValue(1.f);
            input->connect(ac, convolve.get(), 0, 0);
            convolve->connect(ac, wetGain.get(), 0, 0);
            wetGain->connect(ac, context->destination().get(), 0, 0);
            dryGain->connect(ac, context->destination().get(), 0, 0);
            recorder = std::make_shared<RecorderNode>(context->sampleRate());
            recorder->startRecording();
            dryGain->connect(ac, recorder.get(), 0, 0);
            wetGain->connect(ac, recorder.get(), 0, 0);
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
    }
};