// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include "ExampleBaseApp.h"

#include "Simple.h"
#include "LiveGraphUpdate.h"
#include "RenderOffline.h"
#include "ConvolutionReverb.h"

/*


#include "MicrophoneDalek.h"
#include "MicrophoneLoopback.h"
#include "MicrophoneReverb.h"
#include "Rhythm.h"
#include "RhythmAndFilters.h"
#include "PeakCompressor.h"
#include "StereoPanning.h"
#include "Spatialization.h"
#include "Tremolo.h"
#include "RedAlert.h"
#include "InfiniteFM.h"
#include "Groove.h"
#include "Validation.h"
*/

SimpleApp g_simpleExample;
LiveGraphUpdateApp g_liveGraphUpdateApp;
OfflineRenderApp g_offlineRenderApp;
ConvolutionReverbApp g_convolutionReverbExample;

/*

MicrophoneDalekApp g_microphoneDalekApp;
MicrophoneLoopbackApp g_microphoneLoopback;
MicrophoneReverbApp g_microphoneReverb;
PeakCompressorApp g_peakCompressor;
RedAlertApp g_redAlert;
RhythmApp g_rhythm;
RhythmAndFiltersApp g_rhythmAndFilters;
SpatializationApp g_spatialization;
TremoloApp g_tremolo;
ValidationApp g_validation;
InfiniteFMApp g_infiniteFM;
StereoPanningApp g_stereoPanning;
GrooveApp g_grooveExample;
*/

// Windows users will need to set a valid working directory for the LabSoundExamples project, for instance $(ProjectDir)../../assets
int main (int argc, char *argv[])
{
    try
    {
        g_convolutionReverbExample.PlayExample();
    }
    catch (const std::exception & e)
    {
        std::cout << "Uncaught fatal exception: " << e.what() << std::endl;
    }

    return 0;
}
