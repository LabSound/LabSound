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
    
    Example examples[] = {
        { Passing::pass, Skip::no,  new ex_devices() },
        { Passing::pass, Skip::yes, new ex_play_file() },
        { Passing::pass, Skip::yes, new ex_simple() },
        { Passing::pass, Skip::yes, new ex_osc_pop() },
        { Passing::pass, Skip::yes, new ex_playback_events() },
        { Passing::pass, Skip::yes, new ex_offline_rendering() },
        { Passing::pass, Skip::yes, new ex_tremolo() },
        { Passing::pass, Skip::yes, new ex_frequency_modulation() },
        { Passing::pass, Skip::yes, new ex_runtime_graph_update() },
        { Passing::pass, Skip::yes, new ex_microphone_loopback() },
        { Passing::pass, Skip::yes, new ex_microphone_reverb() },
        { Passing::pass, Skip::yes, new ex_peak_compressor() },
        { Passing::pass, Skip::yes, new ex_stereo_panning() },
        { Passing::pass, Skip::yes, new ex_hrtf_spatialization() },
        { Passing::pass, Skip::yes, new ex_convolution_reverb() },
        { Passing::pass, Skip::yes, new ex_misc() },
        { Passing::pass, Skip::yes, new ex_dalek_filter() },
        { Passing::pass, Skip::yes, new ex_redalert_synthesis() },
        { Passing::pass, Skip::yes, new ex_wavepot_dsp() },
        { Passing::pass, Skip::yes, new ex_granulation_node() }, // note: node is under development
        { Passing::pass, Skip::yes, new ex_poly_blep() },
        { Passing::fail, Skip::no,  new ex_split_merge() },
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
