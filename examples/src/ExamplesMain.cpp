// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#define USE_LIVE
#include "Examples.hpp"

/*
 Note for Airpods ~ airpods only present a mono output if the
 default input device is set to airpods
 - if you change the input device to the mac book internal microphone,
   then the airpods work in stereo; strange behavior.
    https://discussions.apple.com/thread/252088121
*/
int main(int argc, char *argv[]) try
{
    enum Passing { pass, fail };
    enum Skip { yes, no };
    struct Example {
        Passing passing;
        Skip skip;
        labsound_example* example;
    };
    
    AudioStreamConfig _inputConfig;
    AudioStreamConfig _outputConfig;
    auto config = GetDefaultAudioDeviceConfiguration(true);
    _inputConfig = config.first;
    _outputConfig = config.second;
    std::shared_ptr<lab::AudioDevice_RtAudio> device(new lab::AudioDevice_RtAudio(_inputConfig, _outputConfig));
    auto context = std::make_shared<lab::AudioContext>(false, true);
    auto destinationNode = std::make_shared<lab::AudioDestinationNode>(*context.get(), device);
    device->setDestinationNode(destinationNode);
    context->setDestinationNode(destinationNode);
    
    const bool NoInput = false;
    const bool UseInput = true;
    
    Example examples[] = {

        { Passing::pass, Skip::yes, new ex_devices(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_play_file(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_simple(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_osc_pop(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_playback_events(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_offline_rendering(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_tremolo(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_frequency_modulation(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_runtime_graph_update(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_microphone_loopback(context, UseInput) },
        { Passing::pass, Skip::yes, new ex_microphone_reverb(context, UseInput) },
        { Passing::pass, Skip::yes, new ex_peak_compressor(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_stereo_panning(context, NoInput) },
        { Passing::pass, Skip::no,  new ex_hrtf_spatialization(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_convolution_reverb(context, NoInput) }, // note: exhibits severe popping
        { Passing::pass, Skip::yes, new ex_misc(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_dalek_filter(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_redalert_synthesis(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_wavepot_dsp(context, NoInput) },
        { Passing::pass, Skip::yes, new ex_granulation_node(context, NoInput) }, // note: node is under development
        { Passing::pass, Skip::yes, new ex_poly_blep(context, NoInput) },
        { Passing::fail, Skip::yes, new ex_split_merge(context, NoInput) },
        { Passing::fail, Skip::yes,  new ex_waveshaper(context, NoInput)},
    };

    static constexpr int iterations = 1;
    for (int i = 0; i < iterations; ++i)
    {
        for (auto& example : examples)
            if (example.skip == Skip::no) {
                example.example->play(argc, argv);
            }
    }

    for (auto& example : examples) {
        delete example.example;
    }
    
    // device, context, and rendernode are circularly referenced, so break the cycle manually.
    destinationNode.reset();
    device->setDestinationNode(destinationNode);
    context->setDestinationNode(destinationNode);
    return EXIT_SUCCESS;
} 
catch (const std::exception & e) 
{
    std::cerr << "unhandled fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
