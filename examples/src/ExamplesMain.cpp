#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ExampleBaseApp.h"

#include "Simple.h"
#include "SimpleRecording.h"
#include "ConvolutionReverb.h"
#include "Rhythm.h"
#include "RhythmAndFilters.h"
#include "PeakCompressor.h"
#include "MicrophoneDalek.h"
#include "MicrophoneLoopback.h"
#include "MicrophoneReverb.h"
#include "Spatialization.h"
#include "Validation.h"
#include "InfiniteFM.h"

ConvolutionReverbApp g_convolutionReverbExample;
SimpleApp g_simpleExample;
SimpleRecordingApp g_simpleRecordingExample;
RhythmApp g_rhythm;
RhythmAndFiltersApp g_rhythmAndFilters;
PeakCompressorApp g_peakCompressor; // Broken
MicrophoneDalekApp g_micrphoneDalek;
MicrophoneLoopbackApp g_microphoneLoopback;
MicrophoneReverbApp g_microphoneReverb;
SpatializationApp g_spatialization;
ValidationApp g_validation;
InfiniteFMApp g_infiniteFM;

int main (int argc, char *argv[])
{
    g_micrphoneDalek.PlayExample();
    return 0;
}
