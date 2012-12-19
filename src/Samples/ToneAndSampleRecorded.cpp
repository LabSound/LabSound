

// For starting the WTF library
#include <wtf/ExportMacros.h>
#include "MainThread.h"

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

#include "ToneAndSampleRecorded.h"

#include <unistd.h>
#include <iostream>

using namespace WebCore;
using LabSound::RecorderNode;
using LabSound::SoundBuffer;

void toneAndSampleRecorded(RefPtr<AudioContext> context, float seconds, char const*const path)
{
    //--------------------------------------------------------------
    // Play a tone and a sample at the same time to recorder node
    //
    ExceptionCode ec;
    
    RefPtr<OscillatorNode> oscillator = context->createOscillator();
    oscillator->start(0);   // play now
    
    SoundBuffer tonbi(context, "tonbi.wav");
    
    RefPtr<RecorderNode> recorder = RecorderNode::create(context.get(), 44100);
    recorder->startRecording();
    oscillator->connect(recorder.get(), 0, 0, ec);
    tonbi.play(recorder.get(), 0.0f);
    
    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
    
    recorder->stopRecording();
    recorder->save(path);
}
