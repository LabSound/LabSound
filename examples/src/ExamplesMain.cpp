// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

// Windows users will need to set a valid working directory for the
// LabSoundExamples project, for instance $(ProjectDir)../../assets

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
    #define _CRT_SECURE_NO_WARNINGS
#endif

// #define USE_LIVE
#include "Examples.hpp"

static constexpr int iterations = 1;

int main (int argc, char *argv[]) try
{   
                                                  // [refactoring status 2.06.2020]
    ex_simple simple;                             // ok
    ex_playback_events playback_events;           // ok
    ex_offline_rendering offline_rendering;       // crash
    ex_tremolo tremolo;                           // silence
    ex_frequency_modulation frequency_mod;        // silence
    ex_runtime_graph_update runtime_graph_update; // silence
    ex_microphone_loopback microphone_loopback;   // silence
    ex_microphone_reverb microphone_reverb;       // silence
    ex_peak_compressor peak_compressor;           // ok
    ex_stereo_panning stereo_panning;             // ok
    ex_hrtf_spatialization hrtf_spatialization;   // crash
    ex_convolution_reverb convolution_reverb;     // loud + clips
    ex_misc misc;                                 // crash
    ex_dalek_filter dalek_filter;                 // ok
    ex_redalert_synthesis redalert_synthesis;     // silence
    ex_wavepot_dsp wavepot_dsp;                   // silence

    // We can optionally play for a number of iterations as a way of testing lifetime & memory issues.
    for (int i = 0; i < iterations; ++i)
    {
        hrtf_spatialization.play(argc, argv);
    }

    return EXIT_SUCCESS;
} 
catch (const std::exception & e) 
{
    std::cerr << "unhandled fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
