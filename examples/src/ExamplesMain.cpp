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
    
    const bool NoInput = false;
    const bool UseInput = true;
    
    Example examples[] = {
        { Passing::pass, Skip::no, new ex_devices(device, NoInput) },
        { Passing::pass, Skip::no, new ex_play_file(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_simple(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_osc_pop(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_playback_events(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_offline_rendering(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_tremolo(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_frequency_modulation(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_runtime_graph_update(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_microphone_loopback(device, UseInput) },
        { Passing::pass, Skip::yes, new ex_microphone_reverb(device, UseInput) },
        { Passing::pass, Skip::yes, new ex_peak_compressor(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_stereo_panning(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_hrtf_spatialization(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_convolution_reverb(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_misc(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_dalek_filter(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_redalert_synthesis(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_wavepot_dsp(device, NoInput) },
        { Passing::pass, Skip::yes, new ex_granulation_node(device, NoInput) }, // note: node is under development
        { Passing::pass, Skip::yes, new ex_poly_blep(device, NoInput) }
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
    
    return EXIT_SUCCESS;
} 
catch (const std::exception & e) 
{
    std::cerr << "unhandled fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
