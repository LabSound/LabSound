// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct OfflineRenderApp : public LabSoundExampleApp
{
    virtual void PlayExample(int argc, char** argv) override
    {
        // Run for 5 seconds
        auto context = lab::Sound::MakeOfflineAudioContext(LABSOUND_DEFAULT_CHANNELS, 5000.f);

        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<AudioBus> musicClip = MakeBusFromSampleFile("samples/mono-music-clip.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> musicClipNode;

        auto recorder = std::make_shared<RecorderNode>();

        context->addAutomaticPullNode(recorder);
        recorder->startRecording();
        {
            ContextRenderLock r(context.get(), "OfflineRenderApp");

            oscillator = std::make_shared<OscillatorNode>(context->sampleRate());
            context->connect(recorder, oscillator, 0, 0);
            oscillator->frequency()->setValue(880.f);
            oscillator->setType(OscillatorType::SINE);
            oscillator->start(0);

            musicClipNode = std::make_shared<SampledAudioNode>();
            musicClipNode->setBus(r, musicClip);
            context->connect(recorder, musicClipNode, 0, 0);
            musicClipNode->start(0.0f);
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

        Wait(std::chrono::seconds(1));
    }
};
