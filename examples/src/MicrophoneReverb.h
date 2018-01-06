// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct MicrophoneReverbApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext();
        auto ac = context.get();
        
        SoundBuffer ir("impulse/cardiod-rear-levelled.wav", context->sampleRate());
        
        std::shared_ptr<AudioHardwareSourceNode> input;
        std::shared_ptr<ConvolverNode> convolve;
        std::shared_ptr<GainNode> wetGain;
        std::shared_ptr<RecorderNode> recorder;
        
        {
            ContextGraphLock g(context.get(), "MicrophoneReverbApp");
            ContextRenderLock r(context.get(), "MicrophoneReverbApp");
            
            input = MakeHardwareSourceNode(r);
            
            recorder = std::make_shared<RecorderNode>(context->sampleRate());
            // input->connect(ac, recorder.get(), 0, 0); Debugging -- this works
            context->addAutomaticPullNode(recorder);
            recorder->startRecording();
            
            convolve = std::make_shared<ConvolverNode>(context->sampleRate());
            convolve->setBuffer(g, ir.audioBuffer); // dimitri
            
            wetGain = std::make_shared<GainNode>(context->sampleRate());
            wetGain->gain()->setValue(1.f);
            
            context->connect(convolve, input, 0, 0);
            context->connect(wetGain, convolve, 0, 0);
            context->connect(context->destination(), wetGain, 0, 0);
            context->connect(recorder, wetGain, 0, 0);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        recorder->stopRecording();
        context->removeAutomaticPullNode(recorder);
        
        recorder->writeRecordingToWav(1, "MicrophoneReverbApp.wav");
    }
};