
#include "LabSound/LabSound.h"
#include "LabSound/AudioContext.h"
#include "LabSound/GainNode.h"
#include "LabSound/OscillatorNode.h"
#include "LabSound/SoundBuffer.h"

using namespace LabSound;

void rhythmTonePanning(RefPtr<AudioContext> context, float seconds)
{
    ExceptionCode ec;
    
    //--------------------------------------------------------------
    // Demonstrate panning between a rhythm and a tone
    //
    RefPtr<OscillatorNode> oscillator = context->createOscillator();
    
    SoundBuffer kick(context, "kick.wav");
    SoundBuffer hihat(context, "hihat.wav");
    SoundBuffer snare(context, "snare.wav");
    
    RefPtr<GainNode> oscGain = context->createGain();
    oscillator->connect(oscGain.get(), 0, 0, ec);
    oscGain->connect(context->destination(), 0, 0, ec);
    oscGain->gain()->setValue(1.0f);
    oscillator->start(0);
    
    RefPtr<GainNode> drumGain = context->createGain();
    drumGain->connect(context->destination(), 0, 0, ec);
    drumGain->gain()->setValue(1.0f);
    
    float startTime = 0;
    float eighthNoteTime = 1.0f/4.0f;
    for (int bar = 0; bar < 10; bar++) {
        float time = startTime + bar * 8 * eighthNoteTime;
        // Play the bass (kick) drum on beats 1, 5
        kick.play(drumGain.get(), time);
        kick.play(drumGain.get(), time + 4 * eighthNoteTime);
        
        // Play the snare drum on beats 3, 7
        snare.play(drumGain.get(), time + 2 * eighthNoteTime);
        snare.play(drumGain.get(), time + 6 * eighthNoteTime);
        
        // Play the hi-hat every eighth note.
        for (int i = 0; i < 8; ++i) {
            hihat.play(drumGain.get(), time + i * eighthNoteTime);
        }
    }
    
    // update gain at 10ms intervals
    for (float i = 0; i < seconds; i += 0.01f) {
        float t1 = i / seconds;
        float t2 = 1.0f - t1;
        float gain1 = cosf(t1 * 0.5f * M_PI);
        float gain2 = cosf(t2 * 0.5f * M_PI);
        oscGain->gain()->setValue(gain1);
        drumGain->gain()->setValue(gain2);
        usleep(10000);
    }
}
