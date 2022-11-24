// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

// Windows users will need to set a valid working directory for the
// LabSoundExamples project, for instance $(ProjectDir)../../assets

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
    #define _CRT_SECURE_NO_WARNINGS
#endif

#define USE_LIVE
#include "Examples.hpp"

static constexpr int iterations = 1;

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
        { Passing::pass, Skip::yes, new ex_play_file() },
        { Passing::pass, Skip::yes, new ex_simple() },
        { Passing::pass, Skip::yes, new ex_osc_pop() },
        { Passing::pass, Skip::yes, new ex_playback_events() },
        { Passing::pass, Skip::yes, new ex_offline_rendering() },
        { Passing::pass, Skip::yes, new ex_tremolo() },
        { Passing::pass, Skip::yes, new ex_frequency_modulation() },
        { Passing::pass, Skip::yes, new ex_runtime_graph_update() },
        { Passing::fail, Skip::yes,  new ex_microphone_loopback() },
        { Passing::fail, Skip::yes, new ex_microphone_reverb() },
        { Passing::pass, Skip::yes, new ex_peak_compressor() },
        { Passing::pass, Skip::yes, new ex_stereo_panning() },
        { Passing::fail, Skip::yes, new ex_hrtf_spatialization() },
        { Passing::fail, Skip::yes, new ex_convolution_reverb() },
        { Passing::fail, Skip::yes, new ex_misc() },
        { Passing::fail, Skip::yes, new ex_dalek_filter() },
        { Passing::pass, Skip::yes, new ex_redalert_synthesis() },
        { Passing::pass, Skip::yes, new ex_wavepot_dsp() },
        { Passing::fail, Skip::yes, new ex_granulation_node() },
        { Passing::pass, Skip::yes, new ex_poly_blep() }
    };

    // We can optionally play for a number of iterations as a way of testing lifetime & memory issues.
    for (int i = 0; i < iterations; ++i)
    {
        for (auto& example : examples)
            if (example.skip == Skip::no)
                example.example->play(argc, argv);
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
