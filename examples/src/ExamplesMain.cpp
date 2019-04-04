// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include "ExampleBaseApp.h"

#include "ConvolutionReverb.h"
#include "EventsApp.h"
#include "Groove.h"
#include "InfiniteFM.h"
#include "LiveGraphUpdate.h"
#include "MicrophoneDalek.h"
#include "MicrophoneLoopback.h"
#include "MicrophoneReverb.h"
#include "PeakCompressor.h"
#include "RedAlert.h"
#include "RenderOffline.h"
#include "Simple.h"
#include "Spatialization.h"
#include "StereoPanning.h"
#include "Tremolo.h"
#include "Validation.h"

ConvolutionReverbApp g_convolutionReverbExample;
EventsApp g_eventsApp;
GrooveApp g_grooveExample;
InfiniteFMApp g_infiniteFM;
LiveGraphUpdateApp g_liveGraphUpdateApp;
MicrophoneDalekApp g_microphoneDalekApp;
MicrophoneLoopbackApp g_microphoneLoopback;
MicrophoneReverbApp g_microphoneReverb;
OfflineRenderApp g_offlineRenderApp;
PeakCompressorApp g_peakCompressor;
RedAlertApp g_redAlert;
SimpleApp g_simpleExample;
SpatializationApp g_spatialization;
StereoPanningApp g_stereoPanning;
TremoloApp g_tremolo;
ValidationApp g_validation;

// Windows users will need to set a valid working directory for the
// LabSoundExamples project, for instance $(ProjectDir)../../assets

constexpr int iterations = 1;

int main (int argc, char *argv[]) try
{
    for (int i = 0; i < iterations; ++i)
        g_eventsApp.PlayExample();

    return 0;
}
catch (const std::exception & e)
{
    std::cerr << "Uncaught fatal exception: " << e.what() << std::endl;
    return 1;
}
