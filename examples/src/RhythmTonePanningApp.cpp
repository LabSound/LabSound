#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;
using namespace std;

// Demonstrate equal power panning between a rhythm and a tone
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    float sampleRate = context->sampleRate();
    
    std::shared_ptr<OscillatorNode> oscillator;
    std::shared_ptr<GainNode> oscGain;
    std::shared_ptr<GainNode> drumGain;
    
    SoundBuffer kick("kick.wav", sampleRate);
    SoundBuffer hihat("hihat.wav", sampleRate);
    SoundBuffer snare("snare.wav", sampleRate);

    vector<shared_ptr<AudioNode>> notes;
    {
        ContextGraphLock g(context);
        ContextRenderLock r(context);
        oscillator = make_shared<OscillatorNode>(r, sampleRate);
        oscGain = make_shared<GainNode>(sampleRate);
        oscillator->connect(g, r, oscGain.get(), 0, 0, ec);
        oscGain->connect(g,r , context->destination().get(), 0, 0, ec);
        oscGain->gain()->setValue(1.0f);
        oscillator->start(r, 0);
        
        drumGain = make_shared<GainNode>(sampleRate);
        drumGain->connect(g, r, context->destination().get(), 0, 0, ec);
        drumGain->gain()->setValue(1.0f);
        
        float startTime = 0;
        float eighthNoteTime = 1.0f/4.0f;
        for (int bar = 0; bar < 10; bar++)
        {
            float time = startTime + bar * 8 * eighthNoteTime;
            // Play the bass (kick) drum on beats 1, 5
            notes.emplace_back(kick.play(g, r, drumGain, time));
            notes.emplace_back(kick.play(g, r, drumGain, time + 4 * eighthNoteTime));
            
            // Play the snare drum on beats 3, 7
            notes.emplace_back(snare.play(g, r, drumGain, time + 2 * eighthNoteTime));
            notes.emplace_back(snare.play(g, r, drumGain, time + 6 * eighthNoteTime));
            
            // Play the hi-hat every eighth note.
            for (int i = 0; i < 8; ++i)
            {
                notes.emplace_back(hihat.play(g, r, drumGain, time + i * eighthNoteTime));
            }
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