//
//  PeakCompApp.cpp
//  PeakCompApp
//
//  Created by Nick Porcino on 2013 12/16.
//  Copyright (c) 2013 LabSound. All rights reserved.
//

#include "PeakCompApp.h"

#include "LabSound.h"
#include "LabSoundIncludes.h"

#include <chrono>
#include <thread>

using namespace LabSound;

int main(int, char**) {
    RefPtr<AudioContext> context = LabSound::init();

    //--------------------------------------------------------------
    // Play a rhythm
    //
    SoundBuffer kick(context, "kick.wav");
    SoundBuffer hihat(context, "hihat.wav");
    SoundBuffer snare(context, "snare.wav");

    ExceptionCode ec;
    RefPtr<BiquadFilterNode> filter = context->createBiquadFilter();
    filter->setType(BiquadFilterNode::LOWPASS, ec);
    filter->frequency()->setValue(4000.0f);

    RefPtr<PeakCompNode> peakComp = PeakCompNode::create(context.get(), 44100);
    connect(filter.get(), peakComp.get());
    connect(peakComp.get(), context->destination());

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
    const float seconds = 10.0f;
    for (float i = 0; i < seconds; i += 0.01f) {
		std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    
    return 0;
}

