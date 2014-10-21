// webaudio specific headers
#include "LabSound/LabSoundIncludes.h"

#include "ToneAndSample.h"

#include <unistd.h>
#include <iostream>

using namespace WebCore;
using LabSound::SoundBuffer;

void toneAndSample(RefPtr<AudioContext> context, float seconds) {
    //--------------------------------------------------------------
    // Play a tone and a sample at the same time.
    //
    ExceptionCode ec;
    RefPtr<OscillatorNode> oscillator = context->createOscillator();
    oscillator->connect(context->destination(), 0, 0, ec);
    oscillator->start(0);   // play now
    oscillator->frequency()->setValue(440.f);
    oscillator->setType(1, ec);
    for (int i = 0; i < 100; ++i)
        usleep(10000);

    //SoundBuffer tonbi(context, "tonbi.wav");
    //tonbi.play(0.0f);

    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
}
