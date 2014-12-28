#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;

// Demonstrate equal power panning between a rhythm and a tone
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    
    auto oscillator = OscillatorNode::create(context, context->sampleRate());
    
    SoundBuffer kick(context, "kick.wav");
    SoundBuffer hihat(context, "hihat.wav");
    SoundBuffer snare(context, "snare.wav");
    
    auto oscGain = GainNode::create(context, context->sampleRate());
    oscillator->connect(oscGain.get(), 0, 0, ec);
    oscGain->connect(context->destination().get(), 0, 0, ec);
    oscGain->gain()->setValue(1.0f);
    oscillator->start(0);
    
    auto drumGain = GainNode::create(context, context->sampleRate());
    drumGain->connect(context->destination().get(), 0, 0, ec);
    drumGain->gain()->setValue(1.0f);
    
    float startTime = 0;
    float eighthNoteTime = 1.0f/4.0f;
    for (int bar = 0; bar < 10; bar++)
    {
        float time = startTime + bar * 8 * eighthNoteTime;
        // Play the bass (kick) drum on beats 1, 5
        kick.play(drumGain.get(), time);
        kick.play(drumGain.get(), time + 4 * eighthNoteTime);
        
        // Play the snare drum on beats 3, 7
        snare.play(drumGain.get(), time + 2 * eighthNoteTime);
        snare.play(drumGain.get(), time + 6 * eighthNoteTime);
        
        // Play the hi-hat every eighth note.
        for (int i = 0; i < 8; ++i)
        {
            hihat.play(drumGain.get(), time + i * eighthNoteTime);
        }
    }
    
    // Update gain at 10ms intervals
    const int seconds = 10;
    for (float i = 0; i < seconds; i += 0.01f)
    {
        float t1 = i / seconds;
        float t2 = 1.0f - t1;
        float gain1 = cosf(t1 * 0.5f * M_PI);
        float gain2 = cosf(t2 * 0.5f * M_PI);
        oscGain->gain()->setValue(gain1);
        drumGain->gain()->setValue(gain2);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    LabSound::finish(context);
    
    return 0;
}