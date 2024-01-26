// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#include "ExamplesCommon.h"
#include "LabSound/extended/Util.h"
#include "LabSound/backends/AudioDevice_RtAudio.h"

struct ex_devices : public labsound_example {
    ex_devices(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_devices() = default;
    virtual void play(int argc, char ** argv) override final
    {
        printf("\nex_devices fetching device info\n--------------------------------\n\n");
        
        const std::vector<AudioDeviceInfo> audioDevices = lab::AudioDevice_RtAudio::MakeAudioDeviceList();
        for (auto & info : audioDevices) {
            printf("Device %d: %s\n", info.index, info.identifier.c_str());
            printf("  input channels: %d\n", info.num_input_channels);
            printf("  output channels: %d\n", info.num_output_channels);
            printf("  default sample rate: %f\n", info.nominal_samplerate);
            printf("  is default input: %s\n", info.is_default_input ? "true" : "false");
            printf("  is default output: %s\n", info.is_default_output ? "true" : "false");
        }
        
        printf("\nex_devices has finished normally\n--------------------------------\n\n");
    }
};


/////////////////////
//    ex_simple    //
/////////////////////

// ex_simple demonstrates the use of an audio clip loaded from disk and a basic sine oscillator. 
struct ex_simple : public labsound_example
{
    ex_simple(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_simple() = default;
    virtual void play(int argc, char** argv) override final
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

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
            ContextRenderLock r(_context.get(), "ex_simple");
            musicClipNode->setBus(musicClip);
        }
        //ac.connect(ac.destinationNode(), musicClipNode, 0, 0);
        musicClipNode->schedule(0.0);

        // osc -> gain -> destination
        ac.connect(gain, oscillator, 0, 0);
        ac.connect(ac.destinationNode(), gain, 0, 0);

        oscillator->frequency()->setValue(440.f);
        oscillator->setType(OscillatorType::SINE);
        oscillator->start(0.0f);

        _nodes.push_back(oscillator);
        _nodes.push_back(musicClipNode);
        _nodes.push_back(gain);

        Wait(2000);
    }
};



/////////////////////
//   ex_play_file  //
/////////////////////

struct ex_play_file : public labsound_example
{
    ex_play_file(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_play_file() = default;
    virtual void play(int argc, char ** argv) override final
    {
        lab::AudioContext & ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        //auto musicClip = MakeBusFromSampleFile("samples/voice.ogg", argc, argv);
        auto musicClip = MakeBusFromSampleFile("samples/stereo-music-clip.wav", argc, argv);
        if (!musicClip)
            return;

        std::shared_ptr<SampledAudioNode> musicClipNode;
        std::shared_ptr<GainNode> gain;

        gain = std::make_shared<GainNode>(ac);
        gain->gain()->setValue(0.5f);

        musicClipNode = std::make_shared<SampledAudioNode>(ac);
        {
            ContextRenderLock r(&ac, "ex_simple");
            musicClipNode->setBus(musicClip);
        }
        musicClipNode->schedule(0.0);

        // osc -> gain -> destination
        ac.connect(gain, musicClipNode, 0, 0);
        ac.connect(ac.destinationNode(), gain, 0, 0);

        _nodes.push_back(musicClipNode);
        _nodes.push_back(gain);
        
        ac.synchronizeConnections();

        printf("------\n");
        ac.debugTraverse(ac.destinationNode().get());
        printf("------\n");

        Wait(static_cast<uint32_t>(1000.f * musicClip->length() / musicClip->sampleRate()));
    }
};




/////////////////////
//    ex_osc_pop   //
/////////////////////

// ex_osc_pop to test oscillator start/stop popping (it shouldn't pop). 
struct ex_osc_pop : public labsound_example
{
    ex_osc_pop(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_osc_pop() = default;
    virtual void play(int argc, char** argv) override final
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<GainNode> gain;
        {
            oscillator = std::make_shared<OscillatorNode>(ac);

            gain = std::make_shared<GainNode>(ac);
            gain->gain()->setValue(1);

            // osc -> destination
            ac.connect(gain, oscillator, 0, 0);
            ac.connect(ac.destinationNode(), gain, 0, 0);

            oscillator->frequency()->setValue(1000.f);
            oscillator->setType(OscillatorType::SINE);
        }

        // retain nodes until demo end
        _nodes.push_back(oscillator);
        _nodes.push_back(gain);

        // queue up 5 1/2 second chirps
        for (float i = 0; i < 5.f; i += 1.f)
        {
            oscillator->start(0);
            oscillator->stop(0.5f);
            Wait(1000);
        }

        // wait at least one context update to allow the disconnections to occur, and for any final
        // render quantum to finish.
        // @TODO the only safe and reasonable thing is to expose a "join" on the context that
        // disconnects the destination node from its graph, then waits a quantum.

        // @TODO the example app should have a set<shared_ptr<AudioNode>> so that the shared_ptrs
        // are not released until the example is finished.

        ac.disconnect(ac.destinationNode());
        Wait(100);
    }
};


//////////////////////////////
//    ex_playback_events    //
//////////////////////////////

// ex_playback_events showcases the use of a `setOnEnded` callback on a `SampledAudioNode`
struct ex_playback_events : public labsound_example
{
    ex_playback_events(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_playback_events() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        auto musicClip = MakeBusFromSampleFile("samples/mono-music-clip.wav", argc, argv);
        if (!musicClip)
            return;

        auto sampledAudio = std::make_shared<SampledAudioNode>(ac);
        {
            ContextRenderLock r(&ac, "ex_playback_events");
            sampledAudio->setBus(musicClip);
        }
        ac.connect(ac.destinationNode(), sampledAudio, 0, 0);

        sampledAudio->setOnEnded([]() {
            std::cout << "sampledAudio finished..." << std::endl;
        });

        sampledAudio->schedule(0.0);

        Wait(6000);
    }
};

//------------------------------
//    ex_offline_rendering
//------------------------------

// This sample illustrates how LabSound can be used "offline," where the graph is not
// pulled by an actual audio device, but rather a null destination. This sample shows
// how a `RecorderNode` can be used to capture the rendered audio to disk.
struct ex_offline_rendering : public labsound_example
{
    ex_offline_rendering(std::shared_ptr<lab::AudioContext> context, bool with_input)
    : labsound_example(context, with_input) {}
    virtual ~ex_offline_rendering() = default;

    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        AudioStreamConfig offlineConfig;
        offlineConfig.device_index = 0;
        offlineConfig.desired_samplerate = LABSOUND_DEFAULT_SAMPLERATE;
        offlineConfig.desired_channels = LABSOUND_DEFAULT_CHANNELS;
        AudioStreamConfig inputConfig = {};

        const float recording_time_ms = 1000.f;

        std::shared_ptr<lab::AudioDestinationNode> renderNode =
            std::make_shared<lab::AudioDestinationNode>(ac,
                std::make_unique<lab::AudioDevice_Null>(inputConfig, offlineConfig));

        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<AudioBus> musicClip = MakeBusFromSampleFile("samples/stereo-music-clip.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> musicClipNode;
        std::shared_ptr<GainNode> gain;

        std::shared_ptr<RecorderNode> recorder(new RecorderNode(ac, offlineConfig));

        ac.connect(renderNode, recorder);

        recorder->startRecording();
        {
            ContextRenderLock r(&ac, "ex_offline_rendering");

            gain.reset(new GainNode(ac));
            gain->gain()->setValue(0.125f);

            // osc -> gain -> recorder
            oscillator.reset(new OscillatorNode(ac));
            ac.connect(gain, oscillator, 0, 0);
            ac.connect(recorder, gain, 0, 0);
            oscillator->frequency()->setValue(880.f);
            oscillator->setType(OscillatorType::SINE);
            oscillator->start(0.0f);

            musicClipNode.reset(new SampledAudioNode(ac));
            ac.connect(recorder, musicClipNode, 0, 0);
            musicClipNode->setBus(musicClip);
            musicClipNode->schedule(0.0);
        }

        auto bus = std::make_shared<lab::AudioBus>(2, 128);
        renderNode->offlineRender(bus.get(), musicClip->length());
        recorder->stopRecording();
        printf("Recorded %f seconds of audio\n", recorder->recordedLengthInSeconds());
        recorder->writeRecordingToWav("ex_offline_rendering.wav", false);
    }
};

//////////////////////
//    ex_tremolo    //
//////////////////////

// This demonstrates the use of `connectParam` as a way of modulating one node through another. 
// Params are effectively control signals that operate at audio rate.
struct ex_tremolo : public labsound_example
{
    ex_tremolo(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_tremolo() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::shared_ptr<OscillatorNode> modulator;
        std::shared_ptr<GainNode> modulatorGain;
        std::shared_ptr<OscillatorNode> osc;
        {
            modulator = std::make_shared<OscillatorNode>(ac);
            modulator->setType(OscillatorType::SINE);
            modulator->frequency()->setValue(4.0f);
            modulator->amplitude()->setValue(40.f);
            modulator->start(0);

            modulatorGain = std::make_shared<GainNode>(ac);
            modulatorGain->gain()->setValue(1.f);

            osc = std::make_shared<OscillatorNode>(ac);
            osc->setType(OscillatorType::TRIANGLE);
            osc->frequency()->setValue(440);
            osc->start(0);

            // Set up processing chain
            // modulator > modulatorGain ---> osc frequency
            //                                osc > context
            ac.connect(modulatorGain, modulator, 0, 0);
            ac.connectParam(osc->detune(), modulatorGain, 0);
            ac.connect(ac.destinationNode(), osc, 0, 0);
        }

        ac.debugTraverse(ac.destinationNode().get());

        Wait(5000);
    }
};

///////////////////////////////////
//    ex_frequency_modulation    //
///////////////////////////////////

// This is inspired by a patch created in the ChucK audio programming language. It showcases
// LabSound's ability to construct arbitrary graphs of oscillators a-la FM synthesis.
struct ex_frequency_modulation : public labsound_example
{
    ex_frequency_modulation(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_frequency_modulation() = default;
    virtual void play(int argc, char ** argv) override
    {
        UniformRandomGenerator fmrng;

        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::shared_ptr<OscillatorNode> modulator;
        std::shared_ptr<GainNode> modulatorGain;
        std::shared_ptr<OscillatorNode> osc;
        std::shared_ptr<ADSRNode> trigger;

        std::shared_ptr<GainNode> signalGain;
        std::shared_ptr<GainNode> feedbackTap;
        std::shared_ptr<DelayNode> chainDelay;

        {
            modulator = std::make_shared<OscillatorNode>(ac);
            modulator->setType(OscillatorType::TRIANGLE);
            modulator->start(0);

            modulatorGain = std::make_shared<GainNode>(ac);

            osc = std::make_shared<OscillatorNode>(ac);
            osc->setType(OscillatorType::SINE);
            osc->frequency()->setValue(300);
            osc->start(0);

            trigger = std::make_shared<ADSRNode>(ac);
            trigger->oneShot()->setBool(true);

            signalGain = std::make_shared<GainNode>(ac);
            signalGain->gain()->setValue(1.0f);

            feedbackTap = std::make_shared<GainNode>(ac);
            feedbackTap->gain()->setValue(0.5f);

            chainDelay = std::make_shared<DelayNode>(ac, 4);
            chainDelay->delayTime()->setFloat(0.0f);  // passthrough delay, not sure if this has the same DSP semantic as ChucK

            // device
            //   signalGain <- chainDelay <- feedbackTap <- signalGain <- trigger <- osc
            //   osc->freq <- modulatorGain <- modulator
                        
            // Set up FM processing chain:
            ac.connect(modulatorGain, modulator, 0, 0);  // Modulator to Gain
            ac.connectParam(osc->frequency(), modulatorGain, 0);  // Gain to frequency parameter

#if 1
            // with adsr
            ac.connect(trigger, osc, 0, 0);  // Osc to ADSR
            ac.connect(signalGain, trigger, 0, 0);  // ADSR to signalGain
#else
            // sans adsr
            ac.connect(signalGain, osc, 0, 0);  // Osc to ADSR
#endif
            
            ac.connect(feedbackTap, signalGain, 0, 0);  // Signal to Feedback
            ac.connect(chainDelay, feedbackTap, 0, 0);  // Feedback to Delay
            ac.connect(signalGain, chainDelay, 0, 0);  // Delay to signalGain
            ac.connect(ac.destinationNode(), signalGain, 0, 0);  // signalGain to DAC
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

            std::cout << now_in_ms << "\n";
            std::cout << "[ex_frequency_modulation] car_freq: " << carrier_freq << "\n";
            std::cout << "[ex_frequency_modulation] mod_freq: " << mod_freq << "\n";
            std::cout << "[ex_frequency_modulation] mod_gain: " << mod_gain << "\n";

            Wait(delay_time_ms / 2);
            trigger->gate()->setValue(0.f); // reset the adsr
            Wait(delay_time_ms / 2);

            if (now_in_ms >= 10000) break;
        }
    }
};

///////////////////////////////////
//    ex_runtime_graph_update    //
///////////////////////////////////

// In most examples, nodes are not disconnected during playback. This sample shows how nodes
// can be arbitrarily connected/disconnected during runtime while the graph is live. 
struct ex_runtime_graph_update : public labsound_example
{
    ex_runtime_graph_update(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_runtime_graph_update() = default;
    virtual void play(int argc, char ** argv) override
    {
        std::shared_ptr<OscillatorNode> oscillator1, oscillator2;
        std::shared_ptr<GainNode> gain;

        {
            lab::AudioContext& ac = *_context.get();
            ac.disconnect(ac.destinationNode());
            ac.synchronizeConnections();
            _nodes.clear();

            {
                oscillator1 = std::make_shared<OscillatorNode>(ac);
                oscillator2 = std::make_shared<OscillatorNode>(ac);

                gain = std::make_shared<GainNode>(ac);
                gain->gain()->setValue(0.50);

                // osc -> gain -> destination
                ac.connect(gain, oscillator1, 0, 0);
                ac.connect(gain, oscillator2, 0, 0);
                ac.connect(ac.destinationNode(), gain, 0, 0);

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
                ac.disconnect(nullptr, oscillator1, 0, 0);
                ac.connect(gain, oscillator2, 0, 0);
                Wait(200);

                ac.disconnect(nullptr, oscillator2, 0, 0);
                ac.connect(gain, oscillator1, 0, 0);
                Wait(200);
            }

            ac.disconnect(nullptr, oscillator1, 0, 0);
            ac.disconnect(nullptr, oscillator2, 0, 0);
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
    ex_microphone_loopback(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_microphone_loopback() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        ContextRenderLock r(&ac, "ex_microphone_loopback");
        std::shared_ptr<AudioHardwareInputNode> inputNode(
                new AudioHardwareInputNode(ac, ac.destinationNode()->device()->sourceProvider()));
        ac.connect(ac.destinationNode(), inputNode, 0, 0);
        Wait(10000);
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
    ex_microphone_reverb(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_microphone_reverb() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        {
            std::shared_ptr<AudioBus> impulseResponseClip =
                MakeBusFromSampleFile("impulse/cardiod-rear-levelled.wav", argc, argv);
            std::shared_ptr<AudioHardwareInputNode> input;
            std::shared_ptr<ConvolverNode> convolve;
            std::shared_ptr<GainNode> wetGain;
            {
                ContextRenderLock r(&ac, "ex_microphone_reverb");

                std::shared_ptr<AudioHardwareInputNode> inputNode(
                        new AudioHardwareInputNode(ac, ac.destinationNode()->device()->sourceProvider()));

                convolve.reset(new ConvolverNode(ac));
                convolve->setImpulse(impulseResponseClip);

                wetGain.reset(new GainNode(ac));
                wetGain->gain()->setValue(0.6f);

                ac.connect(convolve, inputNode, 0, 0);
                ac.connect(wetGain, convolve, 0, 0);
                ac.connect(ac.destinationNode(), wetGain, 0, 0);
            }
            
            _nodes.push_back(input);
            _nodes.push_back(convolve);
            _nodes.push_back(wetGain);

            Wait(10000);
        }
    }
};

//////////////////////////////
//    ex_peak_compressor    //
//////////////////////////////

// Demonstrates the use of the `PeakCompNode` and many scheduled audio sources.
struct ex_peak_compressor : public labsound_example
{
    ex_peak_compressor(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_peak_compressor() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::shared_ptr<AudioBus> kick = MakeBusFromSampleFile("samples/kick.wav", argc, argv);
        std::shared_ptr<AudioBus> hihat = MakeBusFromSampleFile("samples/hihat.wav", argc, argv);
        std::shared_ptr<AudioBus> snare = MakeBusFromSampleFile("samples/snare.wav", argc, argv);

        std::shared_ptr<SampledAudioNode> kick_node = std::make_shared<SampledAudioNode>(ac);
        std::shared_ptr<SampledAudioNode> hihat_node = std::make_shared<SampledAudioNode>(ac);
        std::shared_ptr<SampledAudioNode> snare_node = std::make_shared<SampledAudioNode>(ac);

        std::shared_ptr<BiquadFilterNode> filter;
        std::shared_ptr<PeakCompNode> peakComp;

        {
            ContextRenderLock r(&ac, "ex_peak_compressor");

            filter = std::make_shared<BiquadFilterNode>(ac);
            filter->setType(lab::FilterType::LOWPASS);
            filter->frequency()->setValue(1800.f);

            peakComp = std::make_shared<PeakCompNode>(ac);
            ac.connect(peakComp, filter, 0, 0);
            ac.connect(ac.destinationNode(), peakComp, 0, 0);

            kick_node->setBus(kick);
            ac.connect(filter, kick_node, 0, 0);

            hihat_node->setBus(hihat);
            ac.connect(filter, hihat_node, 0, 0);
            //hihat_node->gain()->setValue(0.2f);

            snare_node->setBus(snare);
            ac.connect(filter, snare_node, 0, 0);

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

        Wait(10000);
    }
};

/////////////////////////////
//    ex_stereo_panning    //
/////////////////////////////

// This illustrates the use of equal-power stereo panning.
struct ex_stereo_panning : public labsound_example
{
    ex_stereo_panning(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_stereo_panning() = default;
    
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::shared_ptr<AudioBus> audioClip = MakeBusFromSampleFile("samples/trainrolling.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>(ac);
        auto stereoPanner = std::make_shared<StereoPannerNode>(ac);

        {
            ContextRenderLock r(&ac, "ex_stereo_panning");

            audioClipNode->setBus(audioClip);
            ac.connect(stereoPanner, audioClipNode, 0, 0);
            audioClipNode->schedule(0.0, -1); // -1 to loop forever

            ac.connect(ac.destinationNode(), stereoPanner, 0, 0);
        }

        if (audioClipNode)
        {
            _nodes.push_back(audioClipNode);
            _nodes.push_back(stereoPanner);

            const int seconds = 8;

            std::thread controlThreadTest([this, &stereoPanner, seconds]() {
                float halfTime = seconds * 0.5f;
                for (float i = 0; i < seconds; i += 0.01f)
                {
                    float x = (i - halfTime) / halfTime;
                    stereoPanner->pan()->setValue(x);
                    Wait(10);
                }
            });

            Wait(seconds * 1000);

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
    ex_hrtf_spatialization(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_hrtf_spatialization() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        if (!ac.loadHrtfDatabase("fail/hrtf")) {
            //should go in here after failing
            if (!ac.loadHrtfDatabase("hrtf"))
            {
                std::string path = std::string(SAMPLE_SRC_DIR) + "/hrtf";
                if (!ac.loadHrtfDatabase(path))
                {
                    printf("Could not load spatialization database");
                    return;
                }
            }
        }
        //should not load again if already successfully loaded above
        if (!ac.loadHrtfDatabase("hrtf"))
        {
            std::string path = std::string(SAMPLE_SRC_DIR) + "/hrtf";
            if (!ac.loadHrtfDatabase(path))
            {
                printf("Could not load spatialization database");
                return;
            }
        }

        std::shared_ptr<AudioBus> audioClip = MakeBusFromSampleFile("samples/trainrolling.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>(ac);
        if (audioClipNode)
        {
            std::shared_ptr<PannerNode> panner = std::make_shared<PannerNode>(ac);
            _nodes.push_back(audioClipNode);
            _nodes.push_back(panner);

            panner->setPanningModel(PanningModel::HRTF);
            {
                ContextRenderLock r(&ac, "ex_hrtf_spatialization");
                audioClipNode->setBus(audioClip);
            }
            audioClipNode->schedule(0.0, -1);  // -1 to loop forever
            ac.connect(panner, audioClipNode, 0, 0);
            ac.connect(ac.destinationNode(), panner, 0, 0);

            ac.listener()->setPosition({0, 0, 0});
            panner->setVelocity(4, 0, 0);

            ac.synchronizeConnections();
            //context->debugTraverse(context->device().get());

            const int seconds = 10;
            float halfTime = seconds * 0.5f;
            for (float i = 0; i < seconds; i += 0.01f)
            {
                float x = (i - halfTime) / halfTime;

                // Put position a +up && +front, because if it goes right through the
                // listener at (0, 0, 0) it abruptly switches from left to right.
                panner->setPosition({x, 0.1f, 0.1f});
                Wait(10);
            }
        }
        else
        {
            std::cerr << "Couldn't initialize train node to play" << std::endl;
        }
    }
};

//-------------------------
//    ex_convolution_reverb
//-------------------------

// This shows the use of the `ConvolverNode` to produce reverb from an arbitrary impulse response.
struct ex_convolution_reverb : public labsound_example
{
    ex_convolution_reverb(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_convolution_reverb() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::shared_ptr<AudioBus> impulseResponseClip = MakeBusFromSampleFile("impulse/cardiod-rear-levelled.wav", argc, argv);
        std::shared_ptr<AudioBus> voiceClip = MakeBusFromSampleFile("samples/voice.ogg", argc, argv);

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

            ContextRenderLock r(&ac, "ex_convolution_reverb");

            convolve = std::make_shared<ConvolverNode>(ac);
            convolve->setImpulse(impulseResponseClip);

            wetGain = std::make_shared<GainNode>(ac);
            wetGain->gain()->setValue(0.5f);
            dryGain = std::make_shared<GainNode>(ac);
            dryGain->gain()->setValue(0.1f);

            ac.connect(wetGain, convolve, 0, 0);
            ac.connect(outputGain, wetGain, 0, 0);
            ac.connect(outputGain, dryGain, 0, 0);
            ac.connect(convolve, dryGain, 0, 0);

            outputGain->gain()->setValue(0.5f);

            voiceNode = std::make_shared<SampledAudioNode>(ac);
            voiceNode->setBus(voiceClip);
            ac.connect(dryGain, voiceNode, 0, 0);

            voiceNode->schedule(0.0);

            ac.connect(ac.destinationNode(), outputGain, 0, 0);
        }

        _nodes.push_back(convolve);
        _nodes.push_back(wetGain);
        _nodes.push_back(dryGain);
        _nodes.push_back(voiceNode);
        _nodes.push_back(outputGain);

        Wait(20000);
    }
};

///////////////////
//    ex_misc    //
///////////////////

// An example with a several of nodes to verify api + functionality changes/improvements/regressions
struct ex_misc : public labsound_example
{
    ex_misc(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_misc() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::array<int, 8> majorScale = {0, 2, 4, 5, 7, 9, 11, 12};
        std::array<int, 8> naturalMinorScale = {0, 2, 3, 5, 7, 9, 11, 12};
        std::array<int, 6> pentatonicMajor = {0, 2, 4, 7, 9, 12};
        std::array<int, 8> pentatonicMinor = {0, 3, 5, 7, 10, 12};
        std::array<int, 8> delayTimes = {266, 533, 399};

        auto randomFloat = std::uniform_real_distribution<float>(0, 1);
        auto randomScaleDegree = std::uniform_int_distribution<int>(0, int(pentatonicMajor.size()) - 1);
        auto randomTimeIndex = std::uniform_int_distribution<int>(0, static_cast<int>(delayTimes.size()) - 1);

        auto audioClip = MakeBusFromSampleFile("samples/voice.ogg", argc, argv);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>(ac);
        std::shared_ptr<PingPongDelayNode> pingping = std::make_shared<PingPongDelayNode>(ac, 240.0f);

        {
            ContextRenderLock r(&ac, "ex_misc");

            pingping->BuildSubgraph(ac);
            pingping->SetFeedback(.75f);
            pingping->SetDelayIndex(lab::TempoSync::TS_16);

            ac.connect(ac.destinationNode(), pingping->output, 0, 0);

            audioClipNode->setBus(audioClip);

            ac.connect(pingping->input, audioClipNode, 0, 0);

            audioClipNode->schedule(0.25);
        }

        _nodes.push_back(audioClipNode);
        
        _nodes.push_back(pingping->output);
        _nodes.push_back(pingping->input);
        _nodes.push_back(pingping->leftDelay);
        _nodes.push_back(pingping->rightDelay);
        _nodes.push_back(pingping->splitterGain);
        _nodes.push_back(pingping->wetGain);
        _nodes.push_back(pingping->feedbackGain);
        _nodes.push_back(pingping->merger);
        _nodes.push_back(pingping->splitter);

        Wait(10000);
    }
};

///////////////////////////
//    ex_dalek_filter    //
///////////////////////////

// Send live audio to a Dalek filter, constructed according to the recipe at http://webaudio.prototyping.bbc.co.uk/ring-modulator/.
// This is used as an example of a complex graph constructed using the LabSound API.
struct ex_dalek_filter : public labsound_example
{
    ex_dalek_filter(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_dalek_filter() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

#ifndef USE_LIVE
        auto audioClip = MakeBusFromSampleFile("samples/voice.ogg", argc, argv);
        if (!audioClip)
            return;
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>();
#endif

        std::shared_ptr<AudioHardwareInputNode> inputNode;

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
            ContextRenderLock r(&ac, "ex_dalek_filter");

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
            std::shared_ptr<AudioHardwareInputNode> inputNode(
                    new AudioHardwareInputNode(ac, ac.destinationNode()->device()->sourceProvider()));
            ac.connect(vcInverter1, inputNode, 0, 0);
            ac.connect(vcDiode4, inputNode, 0, 0);
#else
            audioClipNode->setBus(r, audioClip);
            //ac.connect(vcInverter1, audioClipNode, 0, 0); // dimitri
            ac.connect(vcDiode4, audioClipNode, 0, 0);
            audioClipNode->start(0.f);
#endif

            ac.connect(vcDiode3, vcInverter1, 0, 0);

            // Then the Vin side
            ac.connect(vInGain, vIn, 0, 0);
            ac.connect(vInInverter1, vInGain, 0, 0);
            ac.connect(vcInverter1, vInGain, 0, 0);
            ac.connect(vcDiode4, vInGain, 0, 0);

            ac.connect(vInInverter2, vInInverter1, 0, 0);
            ac.connect(vInDiode2, vInInverter1, 0, 0);
            ac.connect(vInDiode1, vInInverter2, 0, 0);

            // Finally connect the four diodes to the destination via the output-stage compressor and master gain node
            ac.connect(vInInverter3, vInDiode1, 0, 0);
            ac.connect(vInInverter3, vInDiode2, 0, 0);

            ac.connect(compressor, vInInverter3, 0, 0);
            ac.connect(compressor, vcDiode3, 0, 0);
            ac.connect(compressor, vcDiode4, 0, 0);

            ac.connect(outGain, compressor, 0, 0);
            ac.connect(ac.destinationNode(), outGain, 0, 0);
        }

        _nodes.push_back(inputNode);
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

        Wait(30000);
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
    ex_redalert_synthesis(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_redalert_synthesis() = default;
    virtual void play(int argc, char ** argv) override
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

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
            ContextRenderLock r(&ac, "ex_redalert_synthesis");

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
            ac.connectParam(osc->frequency(), sweep, 0);

            // oscillator drives resonator frequency
            ac.connectParam(resonator->frequency(), osc, 0);

            // osc --> oscGain -------------+
            // resonator -> resonatorGain --+--> resonanceSum
            ac.connect(oscGain, osc, 0, 0);
            ac.connect(resonanceSum, oscGain, 0, 0);
            ac.connect(resonatorGain, resonator, 0, 0);
            ac.connect(resonanceSum, resonatorGain, 0, 0);

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
                ac.connect(delay[i], resonanceSum, 0, 0);
                ac.connect(delaySum, delay[i], 0, 0);
            }

            filterSum = std::make_shared<GainNode>(ac);
            filterSum->gain()->setValue(0.2f);

            // delaySum --+--> filter0 --+
            //            +--> filter1 --+
            //            +--> filter2 --+
            //            +--> filter3 --+
            //            +--------------+----> filterSum
            //
            ac.connect(filterSum, delaySum, 0, 0);

            float centerFrequencies[4] = {740.f, 1400.f, 1500.f, 1600.f};
            for (int i = 0; i < 4; ++i)
            {
                filter[i] = std::make_shared<BiquadFilterNode>(ac);
                filter[i]->frequency()->setValue(centerFrequencies[i]);
                filter[i]->q()->setValue(12.f);
                ac.connect(filter[i], delaySum, 0, 0);
                ac.connect(filterSum, filter[i], 0, 0);
            }

            // filterSum --> destination
            ac.connectParam(filterSum->gain(), outputGainFunction, 0);
            ac.connect(ac.destinationNode(), filterSum, 0, 0);
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

        Wait(10000);
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
    ex_wavepot_dsp(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_wavepot_dsp() = default;
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

        float p = 0, k = 0, t1 = 0, t2 = 0, r = 0, x = 0;

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
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

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

            ac.connect(envelope, grooveBox, 0, 0);
            ac.connect(ac.destinationNode(), envelope, 0, 0);
        }

        _nodes.push_back(grooveBox);
        _nodes.push_back(envelope);

        Wait(1000 * (1 + (int) songLenSeconds));
    }
};

//-----------------------
//    ex_granulation_node
//-----------------------

struct ex_granulation_node : public labsound_example
{
    ex_granulation_node(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_granulation_node() = default;
    virtual void play(int argc, char** argv) override final
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        auto grain_source = MakeBusFromSampleFile("samples/voice.ogg", argc, argv);
        if (!grain_source)
            return;

        std::shared_ptr<GranulationNode> granulation_node(new GranulationNode(ac));
        std::shared_ptr<GainNode> gain(new GainNode(ac));
        std::shared_ptr<RecorderNode> recorder;
        gain->gain()->setValue(0.75f);
        granulation_node->numGrains->setValue(20.f);

        {
            ContextRenderLock r(&ac, "ex_granulation_node");
            granulation_node->setGrainSource(r, grain_source);
        }

        ac.connect(gain, granulation_node, 0, 0);
        ac.connect(ac.destinationNode(), gain, 0, 0);

        granulation_node->start(0.0f);

        _nodes.push_back(granulation_node);
        _nodes.push_back(gain);
        _nodes.push_back(recorder);
        
        //AddMonitorNodes();  // testing
        Wait(10000);
    }
};

////////////////////////
//    ex_poly_blep    //
////////////////////////

struct ex_poly_blep : public labsound_example
{
    ex_poly_blep(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_poly_blep() = default;
    
    virtual void play(int argc, char** argv) override final
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::shared_ptr<PolyBLEPNode> polyBlep = std::make_shared<PolyBLEPNode>(ac);
        std::shared_ptr<GainNode> gain = std::make_shared<GainNode>(ac);

        gain->gain()->setValue(1.0f);
        ac.connect(gain, polyBlep, 0, 0);
        ac.connect(ac.destinationNode(), gain, 0, 0);

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

            Wait(delay_time_ms);

            waveformIndex++;
            if (now_in_ms >= 10000) break;
        }
    }
};


struct ex_split_merge : public labsound_example
{
    ex_split_merge(std::shared_ptr<lab::AudioContext> context, bool with_input) : labsound_example(context, with_input) {}
    virtual ~ex_split_merge() = default;
    
    virtual void play(int argc, char ** argv) override final
    {
        lab::AudioContext& ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::shared_ptr<ChannelSplitterNode> splitter = std::make_shared<ChannelSplitterNode>(ac,6);
        std::shared_ptr<ChannelMergerNode> merger = std::make_shared<ChannelMergerNode>(ac,6);
        std::shared_ptr<GainNode> gain = std::make_shared<GainNode>(ac);
        gain->gain()->setValueAtTime(.5f,(float)ac.currentTime());

        auto chan6_source = MakeBusFromSampleFile("samples/6_Channel_ID.wav", argc, argv);
        std::shared_ptr<SampledAudioNode> musicClipNode;
        musicClipNode = std::make_shared<SampledAudioNode>(ac);
        {
            ContextRenderLock r(&ac, "ex_split_merge");
            musicClipNode->setBus(chan6_source);
        }

        ac.connect(splitter, musicClipNode);
        if (0)
        {
            // tests channel swapping
            ac.connect(merger, splitter, 0, 1);  // swap front-left
            ac.connect(merger, splitter, 1, 0);  // and front right to demonstrate channels can be swapped
        }
        else
        {
            // tests getting 6 channels from .wav, and merging them into 2 channels (can hear all 6 audibles)
            //  https://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples.html
            //                                    // chan audible    chron-order
            ac.connect(merger, splitter, 0, 0);  // 0    front-left  1
            ac.connect(merger, splitter, 1, 1);  // 1    front right 2
            ac.connect(merger, splitter, 0, 2);  // 3    center      3
            ac.connect(merger, splitter, 1, 3);  // 4    tone        6 (plays last)
            ac.connect(merger, splitter, 0, 4);  // 5    back left   4
            ac.connect(merger, splitter, 1, 5);  // 6    back right  5
        }
        ac.connect(gain, merger, 0, 0);
        ac.connect(ac.destinationNode(), gain, 0, 0);
        musicClipNode->schedule(0.0);

        // ensure nodes last until the end of the demo
        _nodes.push_back(musicClipNode);
        _nodes.push_back(gain);
        _nodes.push_back(merger);
        _nodes.push_back(splitter);

        const uint32_t delay_time_ms = 7000;
        Wait(delay_time_ms);
    }
    
};
// https://developer.mozilla.org/en-US/docs/Web/API/BaseAudioContext/createWaveShaper#example

static void makeDistortionCurveA(std::vector<float> & curve, unsigned n_samples, float amount = .5f)
{
    float k = amount * 10.0f;
    float deg = (float) M_PI / 180.0f;

    for (unsigned i = 0; i < n_samples; i++)
    {
        float x = (float)i * 2.0f / (float)n_samples - 1.0f;
        curve[i] = ((3.0f + k) * x * 20.0f * deg) / ((float) M_PI + k * std::abs(x));
        //printf("A curve[%d]= %f, ", i, curve[i]);
    }
}

static void makeDistortionCurveB(std::vector<float> & curve, unsigned n_samples, float amount = 0.5f)
{
    constexpr float epsilon = 0.00001f; // Division by zero results in nan output = dropped audio
    float k = 2.0f * amount / (1.0f - amount + epsilon);
    for (unsigned i = 0; i < n_samples; i++)
    {
        float x = (float)i * 2.0f / (float)n_samples -1.0f;
        curve[i] = (1.0f + k) * x / (1.0f + k * std::abs(x));
        //printf("B curve[%d]= %f, ", i, curve[i]);
    }
}

struct ex_waveshaper : public labsound_example
{
    ex_waveshaper(std::shared_ptr<lab::AudioContext> context, bool with_input)
        : labsound_example(context, with_input)
    {
    }
    virtual ~ex_waveshaper() = default;

    virtual void play(int argc, char ** argv) override final
    {
        lab::AudioContext & ac = *_context.get();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        _nodes.clear();

        std::shared_ptr<WaveShaperNode> waveshaper = std::make_shared<WaveShaperNode>(ac);

        std::shared_ptr<OscillatorNode> oscillator = std::make_shared<OscillatorNode>(ac);
        oscillator->setType(OscillatorType::SINE);

        ac.connect(ac.destinationNode(), waveshaper, 0, 0);
        ac.connect(waveshaper, oscillator, 0, 0);
        oscillator->start(0.0f);

        // ensure nodes last until the end of the demo
        _nodes.push_back(oscillator);
        _nodes.push_back(waveshaper);
        const uint32_t delay_time_ms = 1000;
        int n_samples = 44100;
        std::vector<float> curve(n_samples);
        for (int i = -2; i < 3; i++)
        {
            //std::vector<float> curve = makeDistortionCurve(100.0f * i);
            makeDistortionCurveB(curve, n_samples, .25f * i + .5f);
            printf("B i %d curve[0] %f curve[44099] %f oversample none\n", i, curve[0], curve[44099]);
            waveshaper->setCurve(curve);
            Wait(delay_time_ms);
        }
        for (int i = -2; i < 3; i++)
        {
            // std::vector<float> curve = makeDistortionCurve(100.0f * i);
            makeDistortionCurveB(curve, n_samples, .25f * i + .5f);
            printf("B i %d curve[0] %f curve[44099] %f oversample 2x\n", i, curve[0], curve[44099]);
            waveshaper->setCurve(curve);
            waveshaper->setOversample(lab::OverSampleType::_2X);
            Wait(delay_time_ms);
        }
        for (int i = -2; i < 3; i++)
        {
            // std::vector<float> curve = makeDistortionCurve(100.0f * i);
            makeDistortionCurveB(curve, n_samples, .25f * i + .5f);
            printf("B i %d curve[0] %f curve[44099] %f oversample 4X\n", i, curve[0], curve[44099]);
            waveshaper->setCurve(curve);
            waveshaper->setOversample(lab::OverSampleType::_4X);
            Wait(delay_time_ms);
        }
        for (int i = -2; i < 3; i++)
        {
            // std::vector<float> curve = makeDistortionCurve(100.0f * i);
            makeDistortionCurveA(curve, n_samples, .25f * i + .5f);
            printf("A i %d curve[0] %f curve[44099] %f oversample none\n", i, curve[0], curve[44099]);
            waveshaper->setCurve(curve);
            Wait(delay_time_ms);
        }
        
        // ensure the waveshaper node is not being processed before deleting it
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
    }
};
