#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;

int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    
    SoundBuffer kick(context, "kick.wav");
    SoundBuffer hihat(context, "hihat.wav");
    SoundBuffer snare(context, "snare.wav");
    
    float startTime = 0;
    float eighthNoteTime = 1.0f/4.0f;
    for (int bar = 0; bar < 2; bar++)
    {
        float time = startTime + bar * 8 * eighthNoteTime;
        // Play the bass (kick) drum on beats 1, 5
        kick.play(time);
        kick.play(time + 4 * eighthNoteTime);
        
        // Play the snare drum on beats 3, 7
        snare.play(time + 2 * eighthNoteTime);
        snare.play(time + 6 * eighthNoteTime);
        
        // Play the hi-hat every eighth note.
        for (int i = 0; i < 8; ++i)
        {
            hihat.play(time + i * eighthNoteTime);
        }
    }
    
    const int seconds = 4;
    for (int t = 0; t < seconds; ++t)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    LabSound::finish(context);
    
    return 0;
}