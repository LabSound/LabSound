

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

#include "RhythmFiltered.h"

#include <unistd.h>
#include <iostream>

using namespace WebCore;
using LabSound::RecorderNode;
using LabSound::SoundBuffer;

void rhythmFiltered(RefPtr<AudioContext> context, float seconds)
{
    ExceptionCode ec;
    
    //--------------------------------------------------------------
    // Play a rhythm through a low pass filter
    //
    SoundBuffer kick(context, "kick.wav");
    SoundBuffer hihat(context, "hihat.wav");
    SoundBuffer snare(context, "snare.wav");
    
    RefPtr<BiquadFilterNode> filter = context->createBiquadFilter();
    filter->setType(BiquadFilterNode::LOWPASS, ec);
    filter->frequency()->setValue(500.0f);
    filter->connect(context->destination(), 0, 0, ec);
    
    float startTime = 0;
    float eighthNoteTime = 1.0f/4.0f;
    for (int bar = 0; bar < 2; bar++) {
        float time = startTime + bar * 8 * eighthNoteTime;
        // Play the bass (kick) drum on beats 1, 5
        kick.play(filter.get(), time);
        kick.play(filter.get(), time + 4 * eighthNoteTime);
        
        // Play the snare drum on beats 3, 7
        snare.play(filter.get(), time + 2 * eighthNoteTime);
        snare.play(filter.get(), time + 6 * eighthNoteTime);
        
        // Play the hi-hat every eighth note.
        for (int i = 0; i < 8; ++i) {
            hihat.play(filter.get(), time + i * eighthNoteTime);
        }
    }
    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
}
