// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef SAMPLED_INSTRUMENT_NODE
#define SAMPLED_INSTRUMENT_NODE

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioBufferSourceNode.h"

#include "LabSound/extended/ADSRNode.h"
#include "LabSound/extended/SoundBuffer.h"
#include "LabSound/extended/AudioContextLock.h"

#include <iostream> 
#include <array>
#include <string>
#include <algorithm>

namespace lab 
{
    struct SamplerSound;

    // Ex: F#6. Assumes uppercase note names, hash symbol for sharp, and octave. 
    inline uint8_t MakeMIDINoteFromString(std::string noteName) 
    {
        const std::array<std::string, 12> midiTranslationArray = {{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }};

        // Ocatve is always last character, as an integer 
        std::string octaveString = noteName.substr(noteName.length() - 1, 1);
        int octave = std::stoi(octaveString);

        std::string noteString = noteName.erase(noteName.length() - 1, 1); 

        std::transform(noteString.begin(), noteString.end(), noteString.begin(), ::toupper);

        // IF we don't use # notation, convert S to #
        std::replace(noteString.begin(), noteString.end(), 'S', '#');

        // Note name is now the first or second character 
        int notePos = -1;
        for (int i = 0; i < 12; ++i) 
        {
            if (noteString == midiTranslationArray[i])
            {
                notePos = i;
                break;
            }
        }

        return uint8_t((octave * 12.0) + notePos);
    }

    inline std::string MakeStringFromMIDINote(uint8_t note)
    {
        const std::array<std::string, 12> midiTranslationArray = {{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }};

        int octave = int(note / 12) - 1;
        int positionInOctave = note % 12;

        std::string originalNote = midiTranslationArray[positionInOctave];
        std::replace(originalNote.begin(), originalNote.end(), '#', 'S');

        return  (originalNote + std::to_string(octave));
    }

    struct SampledInstrumentDefinition
    {
        std::vector<uint8_t> audio;
        std::string extension;
        uint8_t root;
        uint8_t min;
        uint8_t max;
    };

    // This class is a little but subversive of the typical LabSound node pattern. 
    // Instead of inheriting from a node and injecting samples into an audio buffer,
    // it internally creates ad-hoc AudioBufferSourceNode(s) and keeps them around
    // in the graph so long as our gain node has been connected. 
    class SampledInstrumentNode 
    {
        std::vector<std::shared_ptr<SamplerSound>> samples;
        std::shared_ptr<GainNode> gainNode;
        std::vector<std::shared_ptr<AudioBufferSourceNode>> voices;
        const float maxVoiceCount = 24;
        float sampleRate;
    public:
        SampledInstrumentNode(AudioContext * ctx, float sampleRate);
        ~SampledInstrumentNode();

        void LoadInstrument(std::vector<SampledInstrumentDefinition> & sounds);

        void NoteOn(ContextRenderLock & r, const int midiNoteNumber, const float amplitude);
        void NoteOff(ContextRenderLock & r, const int midiNoteNumber, const float amplitude);

        void KillAllNotes(); 

        GainNode * GetOutputNode() { return gainNode.get(); }
    };

}

#endif
