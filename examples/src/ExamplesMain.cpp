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

// A simple memory manager for the examples, as they are too large to
// instantiate all at once on the stack on small machines.

template <typename T>
class Example
{
public:
    T* ex;
    Example() { ex = new T; }
    ~Example() { delete ex; }
};

int main(int argc, char *argv[]) try
{   
    Example<ex_simple> simple;
    Example<ex_playback_events> playback_events;
    Example<ex_offline_rendering> offline_rendering;
    Example<ex_tremolo> tremolo;
    Example<ex_frequency_modulation> frequency_mod;
    Example<ex_runtime_graph_update> runtime_graph_update;
    Example<ex_microphone_loopback> microphone_loopback;
    Example<ex_microphone_reverb> microphone_reverb;
    Example<ex_peak_compressor> peak_compressor;
    Example<ex_stereo_panning> stereo_panning;
    Example<ex_hrtf_spatialization> hrtf_spatialization;
    Example<ex_convolution_reverb> convolution_reverb;
    Example<ex_misc> misc;
    Example<ex_dalek_filter> dalek_filter;
    Example<ex_redalert_synthesis> redalert_synthesis;
    Example<ex_wavepot_dsp> wavepot_dsp;
    #if 0
    Coming Soon!
        Example<ex_granulation_node> granulation;
    #endif

    // We can optionally play for a number of iterations as a way of testing lifetime & memory issues.
    for (int i = 0; i < iterations; ++i)
    {
        simple.ex->play(argc, argv);
    }

    return EXIT_SUCCESS;
} 
catch (const std::exception & e) 
{
    std::cerr << "unhandled fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
