// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#include "ExamplesCommon.h"
#include "LabSound/extended/Util.h"

/////////////////////////////////////
//    Example Utility Functions    //
/////////////////////////////////////

// Returns input, output
inline std::pair<AudioStreamConfig, AudioStreamConfig> GetDefaultAudioDeviceConfiguration(const bool with_input = false)
{
    AudioStreamConfig inputConfig;
    AudioStreamConfig outputConfig;

    const std::vector<AudioDeviceInfo> audioDevices = lab::MakeAudioDeviceList();
    const AudioDeviceIndex default_output_device = lab::GetDefaultOutputAudioDeviceIndex();
    const AudioDeviceIndex default_input_device = lab::GetDefaultInputAudioDeviceIndex();

    AudioDeviceInfo defaultOutputInfo, defaultInputInfo;
    for (auto & info : audioDevices)
    {
        if (info.index == default_output_device.index) defaultOutputInfo = info; 
        else if (info.index == default_input_device.index) defaultInputInfo = info;
    }

    if (defaultOutputInfo.index != -1)
    {
        outputConfig.device_index = defaultOutputInfo.index;
        outputConfig.desired_channels = std::min(uint32_t(2), defaultOutputInfo.num_output_channels);
        outputConfig.desired_samplerate = defaultOutputInfo.nominal_samplerate;
    }

    if (with_input)
    {
        if (defaultInputInfo.index != -1)
        {
            inputConfig.device_index = defaultInputInfo.index;
            inputConfig.desired_channels = std::min(uint32_t(1), defaultInputInfo.num_input_channels);
            inputConfig.desired_samplerate = defaultInputInfo.nominal_samplerate;
        }
        else
        {
            throw std::invalid_argument("the default audio input device was requested but none were found");
        }
    }

    return {inputConfig, outputConfig};
}

/////////////////////
//    ex_simple    //
/////////////////////

// ex_simple desmontrate the use of an audio clip loaded from disk and a basic sine oscillator. 
struct ex_simple : public labsound_example
{
    virtual void play(int argc, char** argv) override final
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        auto musicClip = MakeBusFromSampleFile("samples/stereo-music-clip.wav", argc, argv);
        if (!musicClip)
            return;

        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<SampledAudioNode> musicClipNode;
        std::shared_ptr<GainNode> gain;

        oscillator = std::make_shared<OscillatorNode>(ac);
        gain = std::make_shared<GainNode>(ac);
        gain->gain()->setValue(0.5f);

        musicClipNode = std::make_shared<SampledAudioNode>(ac);
        {
            ContextRenderLock r(context.get(), "ex_simple");
            musicClipNode->setBus(r, musicClip);
        }
        context->connect(context->device(), musicClipNode, 0, 0);
        musicClipNode->schedule(0.0);

        // osc -> gain -> destination
        context->connect(gain, oscillator, 0, 0);
        context->connect(context->device(), gain, 0, 0);

        oscillator->frequency()->setValue(440.f);
        oscillator->setType(OscillatorType::SINE);
        oscillator->start(0.0f);

        _nodes.push_back(oscillator);
        _nodes.push_back(musicClipNode);
        _nodes.push_back(gain);

        Wait(std::chrono::seconds(6));
    }
};





/////////////////////
//    ex_osc_pop   //
/////////////////////

// ex_osc_pop to test oscillator start/stop popping (it shouldn't pop). 
struct ex_osc_pop : public labsound_example
{
    virtual void play(int argc, char** argv) override final
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<RecorderNode> recorder;
        std::shared_ptr<GainNode> gain;
        {
            oscillator = std::make_shared<OscillatorNode>(ac);

            gain = std::make_shared<GainNode>(ac);
            gain->gain()->setValue(1);

            // osc -> destination
            context->connect(gain, oscillator, 0, 0);
            context->connect(context->device(), gain, 0, 0);

            oscillator->frequency()->setValue(1000.f);
            oscillator->setType(OscillatorType::SINE);

            recorder = std::make_shared<RecorderNode>(ac, defaultAudioDeviceConfigurations.second);
            context->addAutomaticPullNode(recorder);
            recorder->startRecording();
            context->connect(recorder, gain, 0, 0);
        }

        // retain nodes until demo end
        _nodes.push_back(oscillator);
        _nodes.push_back(recorder);
        _nodes.push_back(gain);

        // queue up 5 1/2 second chirps
        for (float i = 0; i < 5.f; i += 1.f)
        {
            oscillator->start(0);
            oscillator->stop(0.5f);
            Wait(std::chrono::milliseconds(1000));
        }

        recorder->stopRecording();
        context->removeAutomaticPullNode(recorder);
        recorder->writeRecordingToWav("ex_osc_pop.wav", false);

        // wait at least one context update to allow the disconnections to occur, and for any final
        // render quantum to finish.
        // @TODO the only safe and reasonable thing is to expose a "join" on the context that
        // disconnects the destination node from its graph, then waits a quantum.

        // @TODO the example app should have a set<shared_ptr<AudioNode>> so that the shared_ptrs
        // are not released until the example is finished.

        context->disconnect(context->device());
        Wait(std::chrono::milliseconds(100));
    }
};


//////////////////////////////
//    ex_playback_events    //
//////////////////////////////

// ex_playback_events showcases the use of a `setOnEnded` callback on a `SampledAudioNode`
struct ex_playback_events : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        auto musicClip = MakeBusFromSampleFile("samples/mono-music-clip.wav", argc, argv);
        if (!musicClip)
            return;

        auto sampledAudio = std::make_shared<SampledAudioNode>(ac);
        {
            ContextRenderLock r(context.get(), "ex_playback_events");
            sampledAudio->setBus(r, musicClip);
        }
        context->connect(context->device(), sampledAudio, 0, 0);

        sampledAudio->setOnEnded([]() {
            std::cout << "sampledAudio finished..." << std::endl;
        });

        sampledAudio->schedule(0.0);

        Wait(std::chrono::seconds(6));
    }
};

////////////////////////////////
//    ex_offline_rendering    //
////////////////////////////////

// This sample illustrates how LabSound can be used "offline," where the graph is not
// pulled by an actual audio device, but rather a null destination. This sample shows
// how a `RecorderNode` can be used to capture the rendered audio to disk.
struct ex_offline_rendering : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        AudioStreamConfig offlineConfig;
        offlineConfig.device_index = 0;
        offlineConfig.desired_samplerate = LABSOUND_DEFAULT_SAMPLERATE;
        offlineConfig.desired_channels = LABSOUND_DEFAULT_CHANNELS;

        const float recording_time_ms = 1000.f;

        std::unique_ptr<lab::AudioContext> context = lab::MakeOfflineAudioContext(offlineConfig, recording_time_ms);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<AudioBus> musicClip = MakeBusFromSampleFile("samples/stereo-music-clip.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> musicClipNode;
        std::shared_ptr<GainNode> gain;

        auto recorder = std::make_shared<RecorderNode>(ac, offlineConfig);

        context->addAutomaticPullNode(recorder);

        recorder->startRecording();

        {
            ContextRenderLock r(context.get(), "ex_offline_rendering");

            gain = std::make_shared<GainNode>(ac);
            gain->gain()->setValue(0.125f);

            // osc -> gain -> recorder
            oscillator = std::make_shared<OscillatorNode>(ac);
            context->connect(gain, oscillator, 0, 0);
            context->connect(recorder, gain, 0, 0);
            oscillator->frequency()->setValue(880.f);
            oscillator->setType(OscillatorType::SINE);
            oscillator->start(0.0f);

            musicClipNode = std::make_shared<SampledAudioNode>(ac);
            context->connect(recorder, musicClipNode, 0, 0);
            musicClipNode->setBus(r, musicClip);
            musicClipNode->schedule(0.0);
        }

        bool complete = false;
        context->offlineRenderCompleteCallback = [&context, &recorder, &complete]() {
            recorder->stopRecording();

            printf("Recorded %f seconds of audio\n", recorder->recordedLengthInSeconds());

            context->removeAutomaticPullNode(recorder);
            recorder->writeRecordingToWav("ex_offline_rendering.wav", false);
            complete = true;
        };

        // Offline rendering happens in a separate thread and blocks until complete.
        // It needs to acquire the graph + render locks, so it must
        // be outside the scope of where we make changes to the graph.
        context->startOfflineRendering();

        while (!complete)
        {
            Wait(std::chrono::milliseconds(100));
        }
    }
};

//////////////////////
//    ex_tremolo    //
//////////////////////

// This demonstrates the use of `connectParam` as a way of modulating one node through another. 
// Params are effectively control signals that operate at audio rate.
struct ex_tremolo : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<OscillatorNode> modulator;
        std::shared_ptr<GainNode> modulatorGain;
        std::shared_ptr<OscillatorNode> osc;
        {
            modulator = std::make_shared<OscillatorNode>(ac);
            modulator->setType(OscillatorType::SINE);
            modulator->frequency()->setValue(8.0f);
            modulator->start(0);

            modulatorGain = std::make_shared<GainNode>(ac);
            modulatorGain->gain()->setValue(10);

            osc = std::make_shared<OscillatorNode>(ac);
            osc->setType(OscillatorType::TRIANGLE);
            osc->frequency()->setValue(440);
            osc->start(0);

            // Set up processing chain
            // modulator > modulatorGain ---> osc frequency
            //                                osc > context
            context->connect(modulatorGain, modulator, 0, 0);
            context->connectParam(osc->detune(), modulatorGain, 0);
            context->connect(context->device(), osc, 0, 0);
        }

        Wait(std::chrono::seconds(5));
    }
};

///////////////////////////////////
//    ex_frequency_modulation    //
///////////////////////////////////

// This is inspired by a patch created in the ChucK audio programming language. It showcases
// LabSound's ability to construct arbitrary graphs of oscillators a-la FM synthesis.
struct ex_frequency_modulation : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        UniformRandomGenerator fmrng;

        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<OscillatorNode> modulator;
        std::shared_ptr<GainNode> modulatorGain;
        std::shared_ptr<OscillatorNode> osc;
        std::shared_ptr<ADSRNode> trigger;

        std::shared_ptr<GainNode> signalGain;
        std::shared_ptr<GainNode> feedbackTap;
        std::shared_ptr<DelayNode> chainDelay;

        {
            modulator = std::make_shared<OscillatorNode>(ac);
            modulator->setType(OscillatorType::SQUARE);
            modulator->start(0);

            modulatorGain = std::make_shared<GainNode>(ac);

            osc = std::make_shared<OscillatorNode>(ac);
            osc->setType(OscillatorType::SQUARE);
            osc->frequency()->setValue(300);
            osc->start(0);

            trigger = std::make_shared<ADSRNode>(ac);

            signalGain = std::make_shared<GainNode>(ac);
            signalGain->gain()->setValue(1.0f);

            feedbackTap = std::make_shared<GainNode>(ac);
            feedbackTap->gain()->setValue(0.5f);

            chainDelay = std::make_shared<DelayNode>(ac, 4);
            chainDelay->delayTime()->setFloat(0.0f);  // passthrough delay, not sure if this has the same DSP semantic as ChucK

            // Set up FM processing chain:
            context->connect(modulatorGain, modulator, 0, 0);  // Modulator to Gain
            context->connectParam(osc->frequency(), modulatorGain, 0);  // Gain to frequency parameter
            context->connect(trigger, osc, 0, 0);  // Osc to ADSR
            context->connect(signalGain, trigger, 0, 0);  // ADSR to signalGain
            context->connect(feedbackTap, signalGain, 0, 0);  // Signal to Feedback
            context->connect(chainDelay, feedbackTap, 0, 0);  // Feedback to Delay
            context->connect(signalGain, chainDelay, 0, 0);  // Delay to signalGain
            context->connect(context->device(), signalGain, 0, 0);  // signalGain to DAC
        }

        double now_in_ms = 0;
        while (true)
        {
            const float carrier_freq = fmrng.random_float(80.f, 440.f);
            osc->frequency()->setValue(carrier_freq);

            const float mod_freq = fmrng.random_float(4.f, 512.f);
            modulator->frequency()->setValue(mod_freq);

            const float mod_gain = fmrng.random_float(16.f, 1024.f);
            modulatorGain->gain()->setValue(mod_gain);

            const float attack_length = fmrng.random_float(0.25f, 0.5f);
            trigger->set(attack_length, 0.50f, 0.50f, 0.25f, 0.50f, 0.1f);
            trigger->gate()->setValue(1.f);

            const uint32_t delay_time_ms = 500;
            now_in_ms += delay_time_ms;

            std::cout << "[ex_frequency_modulation] car_freq: " << carrier_freq << std::endl;
            std::cout << "[ex_frequency_modulation] mod_freq: " << mod_freq << std::endl;
            std::cout << "[ex_frequency_modulation] mod_gain: " << mod_gain << std::endl;

            Wait(std::chrono::milliseconds(delay_time_ms));

            if (now_in_ms >= 10000) break;
        };
    }
};

///////////////////////////////////
//    ex_runtime_graph_update    //
///////////////////////////////////

// In most examples, nodes are not disconnected during playback. This sample shows how nodes
// can be arbitrarily connected/disconnected during runtime while the graph is live. 
struct ex_runtime_graph_update : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::shared_ptr<OscillatorNode> oscillator1, oscillator2;
        std::shared_ptr<GainNode> gain;

        {
            std::unique_ptr<lab::AudioContext> context;
            const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
            context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
            lab::AudioContext& ac = *context.get();

            {
                oscillator1 = std::make_shared<OscillatorNode>(ac);
                oscillator2 = std::make_shared<OscillatorNode>(ac);

                gain = std::make_shared<GainNode>(ac);
                gain->gain()->setValue(0.50);

                // osc -> gain -> destination
                context->connect(gain, oscillator1, 0, 0);
                context->connect(gain, oscillator2, 0, 0);
                context->connect(context->device(), gain, 0, 0);

                oscillator1->setType(OscillatorType::SINE);
                oscillator1->frequency()->setValue(220.f);
                oscillator1->start(0.00f);

                oscillator2->setType(OscillatorType::SINE);
                oscillator2->frequency()->setValue(440.f);
                oscillator2->start(0.00);
            }

            _nodes.push_back(oscillator1);
            _nodes.push_back(oscillator2);
            _nodes.push_back(gain);

            for (int i = 0; i < 4; ++i)
            {
                context->disconnect(nullptr, oscillator1, 0, 0);
                context->connect(gain, oscillator2, 0, 0);
                Wait(std::chrono::milliseconds(200));

                context->disconnect(nullptr, oscillator2, 0, 0);
                context->connect(gain, oscillator1, 0, 0);
                Wait(std::chrono::milliseconds(200));
            }

            context->disconnect(nullptr, oscillator1, 0, 0);
            context->disconnect(nullptr, oscillator2, 0, 0);
        }

        std::cout << "OscillatorNode 1 use_count: " << oscillator1.use_count() << std::endl;
        std::cout << "OscillatorNode 2 use_count: " << oscillator2.use_count() << std::endl;
        std::cout << "GainNode use_count:         " << gain.use_count() << std::endl;
    }
};

//////////////////////////////////
//    ex_microphone_loopback    //
//////////////////////////////////

// This example simply connects an input device (e.g. a microphone) to the output audio device (e.g. your speakers). 
// DANGER! This sample creates an open feedback loop. It is best used when the output audio device is a pair of headphones. 
struct ex_microphone_loopback : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration(true);
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);

        std::shared_ptr<AudioHardwareInputNode> input;
        {
            ContextRenderLock r(context.get(), "ex_microphone_loopback");
            input = lab::MakeAudioHardwareInputNode(r);
            context->connect(context->device(), input, 0, 0);
        }

        Wait(std::chrono::seconds(10));
    }
};

////////////////////////////////
//    ex_microphone_reverb    //
////////////////////////////////

// This sample takes input from a microphone and convolves it with an impulse response to create reverb (i.e. use of the `ConvolverNode`).
// The sample convolution is for a rather large room, so there is a delay.
// DANGER! This sample creates an open feedback loop. It is best used when the output audio device is a pair of headphones. 
struct ex_microphone_reverb : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration(true);
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        {
            std::shared_ptr<AudioBus> impulseResponseClip = MakeBusFromFile("impulse/cardiod-rear-levelled.wav", false);
            std::shared_ptr<AudioHardwareInputNode> input;
            std::shared_ptr<ConvolverNode> convolve;
            std::shared_ptr<GainNode> wetGain;
            std::shared_ptr<RecorderNode> recorder;

            {
                ContextRenderLock r(context.get(), "ex_microphone_reverb");

                input = lab::MakeAudioHardwareInputNode(r);

                recorder = std::make_shared<RecorderNode>(ac, defaultAudioDeviceConfigurations.second);
                context->addAutomaticPullNode(recorder);
                recorder->startRecording();

                convolve = std::make_shared<ConvolverNode>(ac);
                convolve->setImpulse(impulseResponseClip);

                wetGain = std::make_shared<GainNode>(ac);
                wetGain->gain()->setValue(0.6f);

                context->connect(convolve, input, 0, 0);
                context->connect(wetGain, convolve, 0, 0);
                context->connect(context->device(), wetGain, 0, 0);
                context->connect(recorder, wetGain, 0, 0);
            }

            Wait(std::chrono::seconds(10));

            recorder->stopRecording();
            context->removeAutomaticPullNode(recorder);
            recorder->writeRecordingToWav("ex_microphone_reverb.wav", true);

            context.reset();
        }
    }
};

//////////////////////////////
//    ex_peak_compressor    //
//////////////////////////////

// Demonstrates the use of the `PeakCompNode` and many scheduled audio sources.
struct ex_peak_compressor : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<AudioBus> kick = MakeBusFromSampleFile("samples/kick.wav", argc, argv);
        std::shared_ptr<AudioBus> hihat = MakeBusFromSampleFile("samples/hihat.wav", argc, argv);
        std::shared_ptr<AudioBus> snare = MakeBusFromSampleFile("samples/snare.wav", argc, argv);

        std::shared_ptr<SampledAudioNode> kick_node = std::make_shared<SampledAudioNode>(ac);
        std::shared_ptr<SampledAudioNode> hihat_node = std::make_shared<SampledAudioNode>(ac);
        std::shared_ptr<SampledAudioNode> snare_node = std::make_shared<SampledAudioNode>(ac);

        std::shared_ptr<BiquadFilterNode> filter;
        std::shared_ptr<PeakCompNode> peakComp;

        {
            ContextRenderLock r(context.get(), "ex_peak_compressor");

            filter = std::make_shared<BiquadFilterNode>(ac);
            filter->setType(lab::FilterType::LOWPASS);
            filter->frequency()->setValue(1800.f);

            peakComp = std::make_shared<PeakCompNode>(ac);
            context->connect(peakComp, filter, 0, 0);
            context->connect(context->device(), peakComp, 0, 0);

            kick_node->setBus(r, kick);
            context->connect(filter, kick_node, 0, 0);

            hihat_node->setBus(r, hihat);
            context->connect(filter, hihat_node, 0, 0);
            //hihat_node->gain()->setValue(0.2f);

            snare_node->setBus(r, snare);
            context->connect(filter, snare_node, 0, 0);

            _nodes.push_back(kick_node);
            _nodes.push_back(hihat_node);
            _nodes.push_back(snare_node);
            _nodes.push_back(peakComp);
            _nodes.push_back(filter);
        }

        // Speed Metal
        float startTime = 0.1f;
        float bpm = 30.f;
        float bar_length = 60.f / bpm;
        float eighthNoteTime = bar_length / 8.0f;
        for (float bar = 0; bar < 8; bar += 1)
        {
            float time = startTime + bar * bar_length;

            kick_node->schedule(time);
            kick_node->schedule(time + 4 * eighthNoteTime);

            snare_node->schedule(time + 2 * eighthNoteTime);
            snare_node->schedule(time + 6 * eighthNoteTime);
                
            float hihat_beat = 8;
            for (float i = 0; i < hihat_beat; i += 1)
                hihat_node->schedule(time + bar_length * i / hihat_beat);
        }

        Wait(std::chrono::seconds(10));
    }
};

/////////////////////////////
//    ex_stereo_panning    //
/////////////////////////////

// This illustrates the use of equal-power stereo panning.
struct ex_stereo_panning : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<AudioBus> audioClip = MakeBusFromSampleFile("samples/trainrolling.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>(ac);
        auto stereoPanner = std::make_shared<StereoPannerNode>(ac);

        {
            ContextRenderLock r(context.get(), "ex_stereo_panning");

            audioClipNode->setBus(r, audioClip);
            context->connect(stereoPanner, audioClipNode, 0, 0);
            audioClipNode->schedule(0.0, -1); // -1 to loop forever

            context->connect(context->device(), stereoPanner, 0, 0);
        }

        if (audioClipNode)
        {
            _nodes.push_back(audioClipNode);
            _nodes.push_back(stereoPanner);

            const int seconds = 8;

            std::thread controlThreadTest([&stereoPanner, seconds]() {
                float halfTime = seconds * 0.5f;
                for (float i = 0; i < seconds; i += 0.01f)
                {
                    float x = (i - halfTime) / halfTime;
                    stereoPanner->pan()->setValue(x);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });

            Wait(std::chrono::seconds(seconds));

            controlThreadTest.join();
        }
        else
        {
            std::cerr << "Couldn't initialize train node to play" << std::endl;
        }
    }
};

//////////////////////////////////
//    ex_hrtf_spatialization    //
//////////////////////////////////

// This illustrates 3d sound spatialization and doppler shift. Headphones are recommended for this sample.
struct ex_hrtf_spatialization : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<AudioBus> audioClip = MakeBusFromSampleFile("samples/trainrolling.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>(ac);
        std::cout << "Sample Rate is: " << context->sampleRate() << std::endl;
        std::shared_ptr<PannerNode> panner = std::make_shared<PannerNode>(ac, "hrtf");  // note hrtf search path

        {
            ContextRenderLock r(context.get(), "ex_hrtf_spatialization");

            panner->setPanningModel(PanningMode::HRTF);
            context->connect(context->device(), panner, 0, 0);

            audioClipNode->setBus(r, audioClip);
            context->connect(panner, audioClipNode, 0, 0);
            audioClipNode->schedule(0.0, -1); // -1 to loop forever
        }

        if (audioClipNode)
        {
            _nodes.push_back(audioClipNode);
            _nodes.push_back(panner);

            context->listener()->setPosition({0, 0, 0});
            panner->setVelocity(4, 0, 0);

            const int seconds = 10;
            float halfTime = seconds * 0.5f;
            for (float i = 0; i < seconds; i += 0.01f)
            {
                float x = (i - halfTime) / halfTime;

                // Put position a +up && +front, because if it goes right through the
                // listener at (0, 0, 0) it abruptly switches from left to right.
                panner->setPosition({x, 0.1f, 0.1f});

                Wait(std::chrono::milliseconds(10));
            }
        }
        else
        {
            std::cerr << "Couldn't initialize train node to play" << std::endl;
        }
    }
};

////////////////////////////////
//    ex_convolution_reverb    //
////////////////////////////////

// This shows the use of the `ConvolverNode` to produce reverb from an arbitrary impulse response.
struct ex_convolution_reverb : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<AudioBus> impulseResponseClip = MakeBusFromFile("impulse/cardiod-rear-levelled.wav", false);
        std::shared_ptr<AudioBus> voiceClip = MakeBusFromFile("samples/voice.ogg", false);

        if (!impulseResponseClip || !voiceClip)
        {
            std::cerr << "Could not open sample data\n";
            return;
        }

        std::shared_ptr<ConvolverNode> convolve;
        std::shared_ptr<GainNode> wetGain;
        std::shared_ptr<GainNode> dryGain;
        std::shared_ptr<SampledAudioNode> voiceNode;
        std::shared_ptr<GainNode> outputGain = std::make_shared<GainNode>(ac);

        {
            // voice --+-> dry -------------------+
            //         |                          |
            //         +---> convolve ---> wet ---+--->out ---> device

            ContextRenderLock r(context.get(), "ex_convolution_reverb");

            convolve = std::make_shared<ConvolverNode>(ac);
            convolve->setImpulse(impulseResponseClip);

            wetGain = std::make_shared<GainNode>(ac);
            wetGain->gain()->setValue(0.5f);
            dryGain = std::make_shared<GainNode>(ac);
            dryGain->gain()->setValue(0.1f);

            context->connect(wetGain, convolve, 0, 0);
            context->connect(outputGain, wetGain, 0, 0);
            context->connect(outputGain, dryGain, 0, 0);
            context->connect(convolve, dryGain, 0, 0);

            outputGain->gain()->setValue(0.5f);

            voiceNode = std::make_shared<SampledAudioNode>(ac);
            voiceNode->setBus(r, voiceClip);
            context->connect(dryGain, voiceNode, 0, 0);

            voiceNode->schedule(0.0);

            context->connect(context->device(), outputGain, 0, 0);
        }

        _nodes.push_back(convolve);
        _nodes.push_back(wetGain);
        _nodes.push_back(dryGain);
        _nodes.push_back(voiceNode);
        _nodes.push_back(outputGain);

        Wait(std::chrono::seconds(20));
    }
};

///////////////////
//    ex_misc    //
///////////////////

// An example with a several of nodes to verify api + functionality changes/improvements/regressions
struct ex_misc : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::array<int, 8> majorScale = {0, 2, 4, 5, 7, 9, 11, 12};
        std::array<int, 8> naturalMinorScale = {0, 2, 3, 5, 7, 9, 11, 12};
        std::array<int, 6> pentatonicMajor = {0, 2, 4, 7, 9, 12};
        std::array<int, 8> pentatonicMinor = {0, 3, 5, 7, 10, 12};
        std::array<int, 8> delayTimes = {266, 533, 399};

        auto randomFloat = std::uniform_real_distribution<float>(0, 1);
        auto randomScaleDegree = std::uniform_int_distribution<int>(0, int(pentatonicMajor.size()) - 1);
        auto randomTimeIndex = std::uniform_int_distribution<int>(0, static_cast<int>(delayTimes.size()) - 1);

        std::shared_ptr<AudioBus> audioClip = MakeBusFromSampleFile("samples/cello_pluck/cello_pluck_As0.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>(ac);
        std::shared_ptr<PingPongDelayNode> pingping = std::make_shared<PingPongDelayNode>(ac, 240.0f);

        {
            ContextRenderLock r(context.get(), "ex_misc");

            pingping->BuildSubgraph(*context.get());
            pingping->SetFeedback(.75f);
            pingping->SetDelayIndex(lab::TempoSync::TS_16);

            context->connect(context->device(), pingping->output, 0, 0);

            audioClipNode->setBus(r, audioClip);

            context->connect(pingping->input, audioClipNode, 0, 0);

            audioClipNode->schedule(0.25);
        }

        _nodes.push_back(audioClipNode);
        //_nodes.push_back(pingping);

        Wait(std::chrono::seconds(10));
    }
};

///////////////////////////
//    ex_dalek_filter    //
///////////////////////////

// Send live audio to a Dalek filter, constructed according to the recipe at http://webaudio.prototyping.bbc.co.uk/ring-modulator/.
// This is used as an example of a complex graph constructed using the LabSound API.
struct ex_dalek_filter : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration(true);
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

#ifndef USE_LIVE
        auto audioClip = MakeBusFromSampleFile("samples/voice.ogg", argc, argv);
        if (!audioClip)
            return;
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>();
#endif

        std::shared_ptr<AudioHardwareInputNode> input;

        std::shared_ptr<OscillatorNode> vIn;
        std::shared_ptr<GainNode> vInGain;
        std::shared_ptr<GainNode> vInInverter1;
        std::shared_ptr<GainNode> vInInverter2;
        std::shared_ptr<GainNode> vInInverter3;
        std::shared_ptr<DiodeNode> vInDiode1;
        std::shared_ptr<DiodeNode> vInDiode2;
        std::shared_ptr<GainNode> vcInverter1;
        std::shared_ptr<DiodeNode> vcDiode3;
        std::shared_ptr<DiodeNode> vcDiode4;
        std::shared_ptr<GainNode> outGain;
        std::shared_ptr<DynamicsCompressorNode> compressor;

        {
            ContextRenderLock r(context.get(), "ex_dalek_filter");

            vIn = std::make_shared<OscillatorNode>(ac);
            vIn->frequency()->setValue(30.0f);
            vIn->start(0.f);

            vInGain = std::make_shared<GainNode>(ac);
            vInGain->gain()->setValue(0.5f);

            // GainNodes can take negative gain which represents phase inversion
            vInInverter1 = std::make_shared<GainNode>(ac);
            vInInverter1->gain()->setValue(-1.0f);
            vInInverter2 = std::make_shared<GainNode>(ac);
            vInInverter2->gain()->setValue(-1.0f);

            vInDiode1 = std::make_shared<DiodeNode>(ac);
            vInDiode2 = std::make_shared<DiodeNode>(ac);

            vInInverter3 = std::make_shared<GainNode>(ac);
            vInInverter3->gain()->setValue(-1.0f);

            // Now we create the objects on the Vc side of the graph
            vcInverter1 = std::make_shared<GainNode>(ac);
            vcInverter1->gain()->setValue(-1.0f);

            vcDiode3 = std::make_shared<DiodeNode>(ac);
            vcDiode4 = std::make_shared<DiodeNode>(ac);

            // A gain node to control master output levels
            outGain = std::make_shared<GainNode>(ac);
            outGain->gain()->setValue(1.0f);

            // A small addition to the graph given in Parker's paper is a compressor node
            // immediately before the output. This ensures that the user's volume remains
            // somewhat constant when the distortion is increased.
            compressor = std::make_shared<DynamicsCompressorNode>(ac);
            compressor->threshold()->setValue(-14.0f);

            // Now we connect up the graph following the block diagram above (on the web page).
            // When working on complex graphs it helps to have a pen and paper handy!

#ifdef USE_LIVE
            input = lab::MakeAudioHardwareInputNode(r);
            context->connect(vcInverter1, input, 0, 0);
            context->connect(vcDiode4, input, 0, 0);
#else
            audioClipNode->setBus(r, audioClip);
            //context->connect(vcInverter1, audioClipNode, 0, 0); // dimitri
            context->connect(vcDiode4, audioClipNode, 0, 0);
            audioClipNode->start(0.f);
#endif

            context->connect(vcDiode3, vcInverter1, 0, 0);

            // Then the Vin side
            context->connect(vInGain, vIn, 0, 0);
            context->connect(vInInverter1, vInGain, 0, 0);
            context->connect(vcInverter1, vInGain, 0, 0);
            context->connect(vcDiode4, vInGain, 0, 0);

            context->connect(vInInverter2, vInInverter1, 0, 0);
            context->connect(vInDiode2, vInInverter1, 0, 0);
            context->connect(vInDiode1, vInInverter2, 0, 0);

            // Finally connect the four diodes to the destination via the output-stage compressor and master gain node
            context->connect(vInInverter3, vInDiode1, 0, 0);
            context->connect(vInInverter3, vInDiode2, 0, 0);

            context->connect(compressor, vInInverter3, 0, 0);
            context->connect(compressor, vcDiode3, 0, 0);
            context->connect(compressor, vcDiode4, 0, 0);

            context->connect(outGain, compressor, 0, 0);
            context->connect(context->device(), outGain, 0, 0);
        }

        _nodes.push_back(input);
        _nodes.push_back(vIn);
        _nodes.push_back(vInGain);
        _nodes.push_back(vInInverter1);
        _nodes.push_back(vInInverter2);
        _nodes.push_back(vInInverter3);
        _nodes.push_back(vInDiode1);
        _nodes.push_back(vInDiode2);
        _nodes.push_back(vcDiode3);
        _nodes.push_back(vcDiode4);
        _nodes.push_back(vcInverter1);
        _nodes.push_back(outGain);
        _nodes.push_back(compressor);

        Wait(std::chrono::seconds(30));
    }
};

/////////////////////////////////
//    ex_redalert_synthesis    //
/////////////////////////////////

// This is another example of a non-trival graph constructed with the LabSound API. Furthermore, it incorporates
// the use of several `FunctionNodes` that are base types used for implementing complex DSP without modifying
// LabSound internals directly.
struct ex_redalert_synthesis : public labsound_example
{
    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<FunctionNode> sweep;
        std::shared_ptr<FunctionNode> outputGainFunction;

        std::shared_ptr<OscillatorNode> osc;
        std::shared_ptr<GainNode> oscGain;
        std::shared_ptr<OscillatorNode> resonator;
        std::shared_ptr<GainNode> resonatorGain;
        std::shared_ptr<GainNode> resonanceSum;

        std::shared_ptr<DelayNode> delay[5];

        std::shared_ptr<GainNode> delaySum;
        std::shared_ptr<GainNode> filterSum;

        std::shared_ptr<BiquadFilterNode> filter[5];

        {
            ContextRenderLock r(context.get(), "ex_redalert_synthesis");

            sweep = std::make_shared<FunctionNode>(ac, 1);
            sweep->setFunction([](ContextRenderLock & r, FunctionNode * me, int channel, float * values, size_t framesToProcess) {
                double dt = 1.0 / r.context()->sampleRate();
                double now = fmod(me->now(), 1.2f);

                for (size_t i = 0; i < framesToProcess; ++i)
                {
                    //0 to 1 in 900 ms with a 1200ms gap in between
                    if (now > 0.9)
                    {
                        values[i] = 487.f + 360.f;
                    }
                    else
                    {
                        values[i] = std::sqrt((float) now * 1.f / 0.9f) * 487.f + 360.f;
                    }

                    now += dt;
                }
            });

            sweep->start(0);

            outputGainFunction = std::make_shared<FunctionNode>(ac, 1);
            outputGainFunction->setFunction([](ContextRenderLock & r, FunctionNode * me, int channel, float * values, size_t framesToProcess) {
                double dt = 1.0 / r.context()->sampleRate();
                double now = fmod(me->now(), 1.2f);

                for (size_t i = 0; i < framesToProcess; ++i)
                {
                    //0 to 1 in 900 ms with a 1200ms gap in between
                    if (now > 0.9)
                    {
                        values[i] = 0;
                    }
                    else
                    {
                        values[i] = 0.333f;
                    }

                    now += dt;
                }
            });

            outputGainFunction->start(0);

            osc = std::make_shared<OscillatorNode>(ac);
            osc->setType(OscillatorType::SAWTOOTH);
            osc->frequency()->setValue(220);
            osc->start(0);
            oscGain = std::make_shared<GainNode>(ac);
            oscGain->gain()->setValue(0.5f);

            resonator = std::make_shared<OscillatorNode>(ac);
            resonator->setType(OscillatorType::SINE);
            resonator->frequency()->setValue(220);
            resonator->start(0);

            resonatorGain = std::make_shared<GainNode>(ac);
            resonatorGain->gain()->setValue(0.0f);

            resonanceSum = std::make_shared<GainNode>(ac);
            resonanceSum->gain()->setValue(0.5f);

            // sweep drives oscillator frequency
            context->connectParam(osc->frequency(), sweep, 0);

            // oscillator drives resonator frequency
            context->connectParam(resonator->frequency(), osc, 0);

            // osc --> oscGain -------------+
            // resonator -> resonatorGain --+--> resonanceSum
            context->connect(oscGain, osc, 0, 0);
            context->connect(resonanceSum, oscGain, 0, 0);
            context->connect(resonatorGain, resonator, 0, 0);
            context->connect(resonanceSum, resonatorGain, 0, 0);

            delaySum = std::make_shared<GainNode>(ac);
            delaySum->gain()->setValue(0.2f);

            // resonanceSum --+--> delay0 --+
            //                +--> delay1 --+
            //                + ...    .. --+
            //                +--> delay4 --+---> delaySum
            float delays[5] = {0.015f, 0.022f, 0.035f, 0.024f, 0.011f};
            for (int i = 0; i < 5; ++i)
            {
                delay[i] = std::make_shared<DelayNode>(ac, 0.04f);
                delay[i]->delayTime()->setFloat(delays[i]);
                context->connect(delay[i], resonanceSum, 0, 0);
                context->connect(delaySum, delay[i], 0, 0);
            }

            filterSum = std::make_shared<GainNode>(ac);
            filterSum->gain()->setValue(0.2f);

            // delaySum --+--> filter0 --+
            //            +--> filter1 --+
            //            +--> filter2 --+
            //            +--> filter3 --+
            //            +--------------+----> filterSum
            //
            context->connect(filterSum, delaySum, 0, 0);

            float centerFrequencies[4] = {740.f, 1400.f, 1500.f, 1600.f};
            for (int i = 0; i < 4; ++i)
            {
                filter[i] = std::make_shared<BiquadFilterNode>(ac);
                filter[i]->frequency()->setValue(centerFrequencies[i]);
                filter[i]->q()->setValue(12.f);
                context->connect(filter[i], delaySum, 0, 0);
                context->connect(filterSum, filter[i], 0, 0);
            }

            // filterSum --> destination
            context->connectParam(filterSum->gain(), outputGainFunction, 0);
            context->connect(context->device(), filterSum, 0, 0);
        }

        _nodes.push_back(sweep);
        _nodes.push_back(outputGainFunction);
        _nodes.push_back(osc);
        _nodes.push_back(oscGain);
        _nodes.push_back(resonator);
        _nodes.push_back(resonatorGain);
        _nodes.push_back(resonanceSum);
        _nodes.push_back(delaySum);
        _nodes.push_back(filterSum);
        for (int i = 0; i < 5; ++i) _nodes.push_back(delay[i]);
        for (int i = 0; i < 5; ++i) _nodes.push_back(filter[i]);

        Wait(std::chrono::seconds(10));
    }
};

//////////////////////////
//    ex_wavepot_dsp    //
//////////////////////////

// "Unexpected Token" from Wavepot. Original by Stagas: http://wavepot.com/stagas/unexpected-token (MIT License)
// Wavepot is effectively ShaderToy but for the WebAudio API. 
// This sample shows the utility of LabSound as an experimental playground for DSP (synthesis + processing) using the `FunctionNode`. 
struct ex_wavepot_dsp : public labsound_example
{
    float note(int n, int octave = 0)
    {
        return std::pow(2.0f, (n - 33.f + (12.f * octave)) / 12.0f) * 440.f;
    }

    std::vector<std::vector<int>> bassline = {
        {7, 7, 7, 12, 10, 10, 10, 15},
        {7, 7, 7, 15, 15, 17, 10, 29},
        {7, 7, 7, 24, 10, 10, 10, 19},
        {7, 7, 7, 15, 29, 24, 15, 10}};

    std::vector<int> melody = {
        7, 15, 7, 15,
        7, 15, 10, 15,
        10, 12, 24, 19,
        7, 12, 10, 19};

    std::vector<std::vector<int>> chords = {{7, 12, 17, 10}, {10, 15, 19, 24}};

    float quickSin(float x, float t)
    {
        return std::sin(2.0f * float(static_cast<float>(LAB_PI)) * t * x);
    }

    float quickSaw(float x, float t)
    {
        return 1.0f - 2.0f * fmod(t, (1.f / x)) * x;
    }

    float quickSqr(float x, float t)
    {
        return quickSin(x, t) > 0 ? 1.f : -1.f;
    }

    // perc family of functions implement a simple attack/decay, creating a short & percussive envelope for the signal
    float perc(float wave, float decay, float o, float t)
    {
        float env = std::max(0.f, 0.889f - (o * decay) / ((o * decay) + 1.f));
        auto ret = wave * env;
        return ret;
    }

    float perc_b(float wave, float decay, float o, float t)
    {
        float env = std::min(0.f, 0.950f - (o * decay) / ((o * decay) + 1.f));
        auto ret = wave * env;
        return ret;
    }

    float hardClip(float n, float x)
    {
        return x > n ? n : x < -n ? -n : x;
    }

    struct FastLowpass
    {
        float v = 0;
        float operator()(float n, float input)
        {
            return v += (input - v) / n;
        }
    };

    struct FastHighpass
    {
        float v = 0;
        float operator()(float n, float input)
        {
            return v += input - v * n;
        }
    };

    // http://www.musicdsp.org/showone.php?id=24
    // A Moog-style 24db resonant lowpass
    struct MoogFilter
    {
        float y1 = 0;
        float y2 = 0;
        float y3 = 0;
        float y4 = 0;

        float oldx = 0;
        float oldy1 = 0;
        float oldy2 = 0;
        float oldy3 = 0;

        float p, k, t1, t2, r, x;

        float process(float cutoff_, float resonance_, float sample_, float sampleRate)
        {
            float cutoff = 2.0f * cutoff_ / sampleRate;
            float resonance = static_cast<float>(resonance_);
            float sample = static_cast<float>(sample_);

            p = cutoff * (1.8f - 0.8f * cutoff);
            k = 2.f * std::sin(cutoff * static_cast<float>(M_PI) * 0.5f) - 1.0f;
            t1 = (1.0f - p) * 1.386249f;
            t2 = 12.0f + t1 * t1;
            r = resonance * (t2 + 6.0f * t1) / (t2 - 6.0f * t1);

            x = sample - r * y4;

            // Four cascaded one-pole filters (bilinear transform)
            y1 = x * p + oldx * p - k * y1;
            y2 = y1 * p + oldy1 * p - k * y2;
            y3 = y2 * p + oldy2 * p - k * y3;
            y4 = y3 * p + oldy3 * p - k * y4;

            // Clipping band-limited sigmoid
            y4 -= (y4 * y4 * y4) / 6.f;

            oldx = x;
            oldy1 = y1;
            oldy2 = y2;
            oldy3 = y3;

            return y4;
        }
    };

    MoogFilter lp_a[2];
    MoogFilter lp_b[2];
    MoogFilter lp_c[2];

    FastLowpass fastlp_a[2];
    FastHighpass fasthp_c[2];

    virtual void play(int argc, char ** argv) override
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<FunctionNode> grooveBox;
        std::shared_ptr<ADSRNode> envelope;

        float songLenSeconds = 12.0f;
        {
            double elapsedTime = 0.;

            envelope = std::make_shared<ADSRNode>(ac);
            envelope->set(6.0f, 0.75f, 0.125, 14.0f, 0.0f, songLenSeconds);

            float lfo_a, lfo_b, lfo_c;
            float bassWaveform, percussiveWaveform, bassSample;
            float padWaveform, padSample;
            float kickWaveform, kickSample;
            float synthWaveform, synthPercussive, synthDegradedWaveform, synthSample;

            grooveBox = std::make_shared<FunctionNode>(ac, 2);
            grooveBox->setFunction([&](ContextRenderLock & r, FunctionNode * self, int channel, float * samples, size_t framesToProcess) {
                float dt = 1.f / r.context()->sampleRate();  // time duration of one sample
                float now = static_cast<float>(self->now());

                int nextMeasure = int((now / 2)) % bassline.size();
                auto bm = bassline[nextMeasure];

                int nextNote = int((now * 4.f)) % bm.size();
                float bn = note(bm[nextNote], 0);

                auto p = chords[int(now / 4) % chords.size()];

                auto mn = note(melody[int(now * 3.f) % melody.size()], int(2 - (now * 3)) % 4);

                for (size_t i = 0; i < framesToProcess; ++i)
                {
                    lfo_a = quickSin(2.0f, now);
                    lfo_b = quickSin(1.0f / 32.0f, now);
                    lfo_c = quickSin(1.0f / 128.0f, now);

                    // Bass
                    bassWaveform = quickSaw(bn, now) * 1.9f + quickSqr(bn / 2.f, now) * 1.0f + quickSin(bn / 2.f, now) * 2.2f + quickSqr(bn * 3.f, now) * 3.f;
                    percussiveWaveform = perc(bassWaveform / 3.f, 48.0f, fmod(now, 0.125f), now) * 1.0f;
                    bassSample = lp_a[channel].process(1000.f + (lfo_b * 140.f), quickSin(0.5f, now + 0.75f) * 0.2f, percussiveWaveform, r.context()->sampleRate());

                    // Pad
                    padWaveform = 5.1f * quickSaw(note(p[0], 1), now) + 3.9f * quickSaw(note(p[1], 2), now) + 4.0f * quickSaw(note(p[2], 1), now) + 3.0f * quickSqr(note(p[3], 0), now);
                    padSample = 1.0f - ((quickSin(2.0f, now) * 0.28f) + 0.5f) * fasthp_c[channel](0.5f, lp_c[channel].process(1100.f + (lfo_a * 150.f), 0.05f, padWaveform * 0.03f, r.context()->sampleRate()));

                    // Kick
                    kickWaveform = hardClip(0.37f, quickSin(note(7, -1), now)) * 2.0f + hardClip(0.07f, quickSaw(note(7, -1), now * 0.2f)) * 4.00f;
                    kickSample = quickSaw(2.f, now) * 0.054f + fastlp_a[channel](240.0f, perc(hardClip(0.6f, kickWaveform), 54.f, fmod(now, 0.5f), now)) * 2.f;

                    // Synth
                    synthWaveform = quickSaw(mn, now + 1.0f) + quickSqr(mn * 2.02f, now) * 0.4f + quickSqr(mn * 3.f, now + 2.f);
                    synthPercussive = lp_b[channel].process(3200.0f + (lfo_a * 400.f), 0.1f, perc(synthWaveform, 1.6f, fmod(now, 4.f), now) * 1.7f, r.context()->sampleRate()) * 1.8f;
                    synthDegradedWaveform = synthPercussive * quickSin(note(5, 2), now);
                    synthSample = 0.4f * synthPercussive + 0.05f * synthDegradedWaveform;

                    // Mixer
                    samples[i] = (0.66f * hardClip(0.65f, bassSample)) + (0.50f * padSample) + (0.66f * synthSample) + (2.75f * kickSample);

                    now += dt;
                }

                elapsedTime += now;
            });

            grooveBox->start(0);
            envelope->gate()->setValue(1.f);

            context->connect(envelope, grooveBox, 0, 0);
            context->connect(context->device(), envelope, 0, 0);
        }

        _nodes.push_back(grooveBox);
        _nodes.push_back(envelope);

        Wait(std::chrono::seconds(1 + (int) songLenSeconds));
        context.reset();
    }
};

///////////////////////////////
//    ex_granulation_node    //
///////////////////////////////

struct ex_granulation_node : public labsound_example
{
    virtual void play(int argc, char** argv) override final
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        auto grain_source = MakeBusFromSampleFile("samples/voice.ogg", argc, argv);
        if (!grain_source) return;

        std::shared_ptr<GranulationNode> granulation_node = std::make_shared<GranulationNode>(ac);
        std::shared_ptr<GainNode> gain = std::make_shared<GainNode>(ac);
        std::shared_ptr<RecorderNode> recorder;
        gain->gain()->setValue(0.75f);

        {
            ContextRenderLock r(context.get(), "ex_granulation_node");
            recorder = std::make_shared<RecorderNode>(ac, defaultAudioDeviceConfigurations.second);
            context->addAutomaticPullNode(recorder);
            recorder->startRecording();

            granulation_node->setGrainSource(r, grain_source);
        }

        context->connect(gain, granulation_node, 0, 0);
        context->connect(context->device(), gain, 0, 0);
        context->connect(recorder, gain, 0, 0);

        granulation_node->start(0.0f);

        _nodes.push_back(granulation_node);
        _nodes.push_back(gain);
        _nodes.push_back(recorder);

        Wait(std::chrono::seconds(10));

        recorder->stopRecording();
        context->removeAutomaticPullNode(recorder);
        recorder->writeRecordingToWav("ex_granulation_node.wav", false);
    }
};

////////////////////////
//    ex_poly_blep    //
////////////////////////

struct ex_poly_blep : public labsound_example
{
    virtual void play(int argc, char** argv) override final
    {
        std::unique_ptr<lab::AudioContext> context;
        const auto defaultAudioDeviceConfigurations = GetDefaultAudioDeviceConfiguration();
        context = lab::MakeRealtimeAudioContext(defaultAudioDeviceConfigurations.second, defaultAudioDeviceConfigurations.first);
        lab::AudioContext& ac = *context.get();

        std::shared_ptr<PolyBLEPNode> polyBlep = std::make_shared<PolyBLEPNode>(ac);
        std::shared_ptr<GainNode> gain = std::make_shared<GainNode>(ac);

        gain->gain()->setValue(1.0f);
        context->connect(gain, polyBlep, 0, 0);
        context->connect(context->device(), gain, 0, 0);

        polyBlep->frequency()->setValue(220.f);
        polyBlep->setType(PolyBLEPType::TRIANGLE);
        polyBlep->start(0.0f);

        std::vector<PolyBLEPType> blepWaveforms = 
        {
            PolyBLEPType::TRIANGLE,
            PolyBLEPType::SQUARE,
            PolyBLEPType::RECTANGLE,
            PolyBLEPType::SAWTOOTH,
            PolyBLEPType::RAMP,
            PolyBLEPType::MODIFIED_TRIANGLE,
            PolyBLEPType::MODIFIED_SQUARE,
            PolyBLEPType::HALF_WAVE_RECTIFIED_SINE,
            PolyBLEPType::FULL_WAVE_RECTIFIED_SINE,
            PolyBLEPType::TRIANGULAR_PULSE,
            PolyBLEPType::TRAPEZOID_FIXED,
            PolyBLEPType::TRAPEZOID_VARIABLE
        };

        _nodes.push_back(polyBlep);
        _nodes.push_back(gain);

        double now_in_ms = 0;
        int waveformIndex = 0;
        while (true)
        {
            const uint32_t delay_time_ms = 500;
            now_in_ms += delay_time_ms;

            auto waveform = blepWaveforms[waveformIndex % blepWaveforms.size()];
            polyBlep->setType(waveform);

            Wait(std::chrono::milliseconds(delay_time_ms));

            waveformIndex++;
            if (now_in_ms >= 10000) break;
        };
    }
};
