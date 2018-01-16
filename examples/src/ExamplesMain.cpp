// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include "ExampleBaseApp.h"

#include "Simple.h"
#include "PeakCompressor.h"
#include "LiveGraphUpdate.h"
#include "RenderOffline.h"
#include "ConvolutionReverb.h"
#include "RedAlert.h"
#include "Groove.h"
#include "InfiniteFM.h"
#include "Tremolo.h"
#include "StereoPanning.h"
#include "Spatialization.h"
#include "Validation.h"
#include "MicrophoneDalek.h"
#include "MicrophoneLoopback.h"
#include "MicrophoneReverb.h"

SimpleApp g_simpleExample;
PeakCompressorApp g_peakCompressor;
LiveGraphUpdateApp g_liveGraphUpdateApp;
OfflineRenderApp g_offlineRenderApp;
ConvolutionReverbApp g_convolutionReverbExample;
RedAlertApp g_redAlert;
GrooveApp g_grooveExample;
InfiniteFMApp g_infiniteFM;
TremoloApp g_tremolo;
StereoPanningApp g_stereoPanning;
SpatializationApp g_spatialization;
ValidationApp g_validation;
MicrophoneDalekApp g_microphoneDalekApp;
MicrophoneLoopbackApp g_microphoneLoopback;
MicrophoneReverbApp g_microphoneReverb;

// Windows users will need to set a valid working directory for the LabSoundExamples project, for instance $(ProjectDir)../../assets
int main (int argc, char *argv[])
{
    try
    {
        for (int i = 0; i < 48; ++i)
        {
            g_liveGraphUpdateApp.PlayExample();
        }

    }
    catch (const std::exception & e)
    {
        std::cout << "Uncaught fatal exception: " << e.what() << std::endl;
    }

    return 0;
}
