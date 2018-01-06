// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct OfflineRenderApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        // Run for 5 seconds
        auto context = lab::MakeOfflineAudioContext(5000);
        
        std::shared_ptr<OscillatorNode> oscillator;
        SoundBuffer tonbi("samples/tonbi.wav", context->sampleRate());
        std::shared_ptr<SampledAudioNode> tonbiSound;
        
        auto recorder = std::make_shared<RecorderNode>(context->sampleRate());
        
        context->addAutomaticPullNode(recorder);
        recorder->startRecording();
        {
            ContextGraphLock g(context.get(), "OfflineRenderApp");
            ContextRenderLock r(context.get(), "OfflineRenderApp");
            
            oscillator = std::make_shared<OscillatorNode>(r, context->sampleRate());
            context->connect(recorder, oscillator, 0, 0);

            oscillator->frequency()->setValue(880.f);
            oscillator->setType(r, OscillatorType::SINE);
            
            tonbiSound = tonbi.play(r, recorder, 0.0f);
            tonbiSound = tonbi.play(r, 0.0f);
            oscillator->start(0);
        }
        
        context->offlineRenderCompleteCallback = [&context, &recorder]()
        {
            recorder->stopRecording();
            context->removeAutomaticPullNode(recorder);
            recorder->writeRecordingToWav(1, "OfflineRender.wav");
        };
        
        // Offline rendering happens in a separate thread and blocks until complete.
        // It needs to acquire the graph and render lock itself, so it must
        // be outside the scope of where we make changes to the graph!
        context->startRendering();
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
};
