


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

#include "ToneAndSample.h"

#include <unistd.h>
#include <iostream>

using namespace WebCore;
using LabSound::RecorderNode;
using LabSound::SoundBuffer;

void toneAndSample(RefPtr<AudioContext> context, float seconds)
{
    //--------------------------------------------------------------
    // Play a tone and a sample at the same time.
    //
    ExceptionCode ec;
    RefPtr<OscillatorNode> oscillator = context->createOscillator();
    oscillator->connect(context->destination(), 0, 0, ec);
    oscillator->start(0);   // play now
    for (int i = 0; i < 100; ++i)
        usleep(10000);

    SoundBuffer tonbi(context, "tonbi.wav");
    tonbi.play(0.0f);

    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
}
