

// For starting the WTF library
#include <wtf/ExportMacros.h>
#include <wtf/MainThread.h>

// webaudio specific headers
#include "AudioBufferSourceNode.h"
#include "AudioContext.h"
#include "BiquadFilterNode.h"
#include "ConvolverNode.h"
#include "ExceptionCode.h"
#include "GainNode.h"
#include "MediaStream.h"
#include "MediaStreamAudioSourceNode.h"
#include "OscillatorNode.h"
#include "PannerNode.h"

// LabSound
#include "RecorderNode.h"
#include "SoundBuffer.h"

// Examples
#include "ToneAndSample.h"
#include "ToneAndSampleRecorded.h"
#include "LiveEcho.h"
#include "ReverbSample.h"
#include "LiveReverbRecording.h"
#include "SampleSpatialization.h"
#include "Rhythm.h"
#include "RhythmFiltered.h"
#include "RhythmTonePanning.h"
#include "DalekVoice.h"

#include <unistd.h>
#include <iostream>

using namespace WebCore;
using LabSound::RecorderNode;

namespace WebCore
{
    class Document { public: };
}


int main(int, char**)
{
    // Initialize threads for the WTF library
    WTF::initializeThreading();
    WTF::initializeMainThread();
    
    // Create an audio context object
    Document d;
    ExceptionCode ec;
    RefPtr<AudioContext> context = AudioContext::create(&d, ec);

    rhythmFiltered(context, 3.0f);
    
    return 0;
}
