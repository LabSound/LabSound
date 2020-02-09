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
                                                 
    ex_simple simple;                            
    ex_playback_events playback_events;          
    ex_offline_rendering offline_rendering;      
    ex_tremolo tremolo;                          
    ex_frequency_modulation frequency_mod;       
    ex_runtime_graph_update runtime_graph_update;
    ex_microphone_loopback microphone_loopback;  
    ex_microphone_reverb microphone_reverb;      
    ex_peak_compressor peak_compressor;          
    ex_stereo_panning stereo_panning;            
    ex_hrtf_spatialization hrtf_spatialization;  
    ex_convolution_reverb convolution_reverb;    
    ex_misc misc;                                
    ex_dalek_filter dalek_filter;                
    ex_redalert_synthesis redalert_synthesis;    
    ex_wavepot_dsp wavepot_dsp;                  

    // We can optionally play for a number of iterations as a way of testing lifetime & memory issues.
    for (int i = 0; i < iterations; ++i)
    {
        offline_rendering.play(argc, argv);
    }

    return EXIT_SUCCESS;
} 
catch (const std::exception & e) 
{
    std::cerr << "unhandled fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
