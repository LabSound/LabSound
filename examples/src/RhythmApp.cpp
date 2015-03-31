#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;
using namespace std;

int main(int, char**)
{
    auto context = LabSound::init();
    float sampleRate = context->sampleRate();
    
    SoundBuffer kick("kick.wav", sampleRate);
    SoundBuffer hihat("hihat.wav", sampleRate);
    SoundBuffer snare("snare.wav", sampleRate);
    
    vector<shared_ptr<AudioNode>> notes;    // store the notes to keep them around long enough to play
    {
        ContextGraphLock g(context, "RhythmApp");
        ContextRenderLock r(context, "RhythmApp");
        
        float startTime = 0;
        float eighthNoteTime = 1.0f/4.0f;
        for (int bar = 0; bar < 2; bar++)
        {
            float time = startTime + bar * 8 * eighthNoteTime;
            // Play the bass (kick) drum on beats 1, 5
            notes.emplace_back(kick.play(g, r, time));
            notes.emplace_back(kick.play(g, r, time + 4 * eighthNoteTime));
            
            // Play the snare drum on beats 3, 7
            notes.emplace_back(snare.play(g, r, time + 2 * eighthNoteTime));
            notes.emplace_back(snare.play(g, r, time + 6 * eighthNoteTime));
            
            // Play the hi-hat every eighth note.
            for (int i = 0; i < 8; ++i)
            {
                notes.emplace_back(hihat.play(g, r, time + i * eighthNoteTime));
            }
        }
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    LabSound::finish(context);
    return 0;
}
