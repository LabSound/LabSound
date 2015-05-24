#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ExampleBaseApp.h"

#include "ConvolutionReverb.h"
#include "InfiniteFM.h"
#include "MicrophoneDalek.h"
#include "MicrophoneLoopback.h"
#include "MicrophoneReverb.h"
#include "PeakCompressor.h"
#include "RedAlert.h"
#include "Rhythm.h"
#include "RhythmAndFilters.h"
#include "Simple.h"
#include "SimpleRecording.h"
#include "Spatialization.h"
#include "Tremolo.h"
#include "Validation.h"
#include "StereoPanning.h"
#include "Groove.h"

ConvolutionReverbApp g_convolutionReverbExample;
MicrophoneDalekApp g_microphoneDalekApp;
MicrophoneLoopbackApp g_microphoneLoopback;
MicrophoneReverbApp g_microphoneReverb;
PeakCompressorApp g_peakCompressor;
RedAlertApp g_redAlert;
RhythmApp g_rhythm;
RhythmAndFiltersApp g_rhythmAndFilters;
SimpleApp g_simpleExample;
SimpleRecordingApp g_simpleRecordingExample;
SpatializationApp g_spatialization;
TremoloApp g_tremolo;
ValidationApp g_validation;
InfiniteFMApp g_infiniteFM;
StereoPanningApp g_stereoPanning;
GrooveApp g_grooveExample;

int main (int argc, char *argv[])
{
    g_microphoneDalekApp.PlayExample();
    return 0;
}
