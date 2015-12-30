#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ExampleBaseApp.h"

#include "Simple.h"
#include "OfflineRender.h"
#include "ConvolutionReverb.h"
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

SimpleApp g_simpleExample;
OfflineRenderApp g_offlineRenderApp;
ConvolutionReverbApp g_convolutionReverbExample;
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

int main (int argc, char *argv[])
{
    g_rhythm.PlayExample();
    return 0;
}
