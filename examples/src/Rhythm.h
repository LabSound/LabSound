// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

struct RhythmApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        
        auto context = lab::MakeAudioContext();
        float sampleRate = context->sampleRate();
        
        SoundBuffer kick("samples/kick.wav", sampleRate);
        SoundBuffer hihat("samples/hihat.wav", sampleRate);
        SoundBuffer snare("samples/snare.wav", sampleRate);
        
        // store the notes to keep them around long enough to play
        std::vector<std::shared_ptr<AudioNode>> notes;
        {
            ContextGraphLock g(context, "RhythmApp");
            ContextRenderLock r(context, "RhythmApp");
            
            float startTime = 0;
            float eighthNoteTime = 1.0f / 4.0f;
            for (int bar = 0; bar < 2; bar++)
            {
                float time = startTime + bar * 8 * eighthNoteTime;
                
                // Play the bass (kick) drum on beats 1, 5
                notes.emplace_back(kick.play(r, time));
                notes.emplace_back(kick.play(r, time + 4 * eighthNoteTime));
                
                // Play the snare drum on beats 3, 7
                notes.emplace_back(snare.play(r, time + 2 * eighthNoteTime));
                notes.emplace_back(snare.play(r, time + 6 * eighthNoteTime));
                
                // Play the hi-hat every eighth note.
                for (int i = 0; i < 8; ++i)
                {
                    notes.emplace_back(hihat.play(r, time + i * eighthNoteTime));
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        lab::CleanupAudioContext(context);
    }
};
