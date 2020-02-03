// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"
#include <iostream>
#include <string>

struct SimpleApp : public LabSoundExampleApp
{
    virtual void PlayExample(int argc, char** argv) override
    {
        const std::vector<AudioDeviceInfo> audioDevices = lab::MakeAudioDeviceList();
        const uint32_t default_output_device = lab::GetDefaultOutputAudioDeviceIndex();
        const uint32_t default_input_device = lab::GetDefaultInputAudioDeviceIndex();

        std::unique_ptr<lab::AudioContext> context;

        AudioDeviceInfo defaultDeviceInfo;
        for (auto & info : audioDevices)
        {
            if (info.index == default_output_device) defaultDeviceInfo = info;
        }

        if (defaultDeviceInfo.index != -1)
        {
            AudioStreamConfig inputConfig, outputConfig;

            outputConfig.device_index = defaultDeviceInfo.index;
            outputConfig.desired_channels = std::min(uint32_t(2), defaultDeviceInfo.num_output_channels);
            outputConfig.desired_samplerate = defaultDeviceInfo.nominal_samplerate;

            context = lab::MakeRealtimeAudioContext(outputConfig, inputConfig);
        }

        auto musicClip = MakeBusFromSampleFile("samples/stereo-music-clip.wav", argc, argv);
        if (!musicClip)
            return;

        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<SampledAudioNode> musicClipNode;
        std::shared_ptr<GainNode> gain;

        oscillator = std::make_shared<OscillatorNode>(context->sampleRate());
        gain = std::make_shared<GainNode>();
        gain->gain()->setValue(0.0625f);

        musicClipNode = std::make_shared<SampledAudioNode>();
        {
            ContextRenderLock r(context.get(), "Simple");
            musicClipNode->setBus(r, musicClip);
        }
        context->connect(gain, musicClipNode, 0, 0);
        musicClipNode->start(0.0f);

        // osc -> gain -> destination
        context->connect(gain, oscillator, 0, 0);
        context->connect(context->device(), gain, 0, 0);

        oscillator->frequency()->setValue(440.f);
        oscillator->setType(OscillatorType::SINE);
        oscillator->start(0.0f);

        Wait(std::chrono::seconds(6));
    }
};
