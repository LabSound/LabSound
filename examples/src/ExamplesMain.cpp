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

SimpleApp g_simpleExample;
SimpleRecordingApp g_simpleRecordingExample;
RhythmApp g_rhythm;
RhythmAndFiltersApp g_rhythmAndFilters;
PeakCompressorApp g_peakCompressor;
MicrophoneDalekApp g_micrphoneDalek;
MicrophoneLoopbackApp g_microphoneLoopback;
MicrophoneReverbApp g_microphoneReverb;
SpatializationApp g_spatialization;

ValidationApp g_validation;

int main (int argc, char *argv[])
{
    
    g_validation.PlayExample();
    
    return 0;
}