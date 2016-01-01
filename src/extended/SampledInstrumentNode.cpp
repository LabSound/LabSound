// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/SampledInstrumentNode.h"

#include "internal/ConfigMacros.h"

#include <string>
#include <fstream>
#include <streambuf>
#include <stdlib.h>

namespace lab 
{

    using namespace lab;

    struct SamplerSound 
    {

        SamplerSound(SampledInstrumentDefinition & def, float sampleRate)
        {
            soundBuf = new SoundBuffer(def.audio, def.extension, sampleRate);
            this->baseMidiNote = def.root;
            this->midiNoteLow = def.min;
            this->midiNoteHigh = def.max;
        }

        bool AppliesToNote(uint8_t note) 
        {
            // Debugging: 
            //std::cout << "Note: " << int(note) << std::endl;
            //std::cout << "Base: " << int(baseMidiNote) << std::endl;
            //std::cout << "Low: " << int(midiNoteLow) << std::endl;
            //std::cout << "High: " << int(midiNoteHigh) << std::endl;
            //std::cout << std::endl << std::endl;
            if (baseMidiNote == note || (note >= midiNoteLow && note <= midiNoteHigh)) 
                return true; 
            else 
                return false; 
        }

        SoundBuffer * soundBuf;

        uint8_t baseMidiNote;
        uint8_t midiNoteLow;
        uint8_t midiNoteHigh; 
    };

    SampledInstrumentNode::SampledInstrumentNode(AudioContext * ctx, float sampleRate) : sampleRate(sampleRate)
    { 
        gainNode = std::make_shared<GainNode>(sampleRate);
        gainNode->gain()->setValue(1.0);

        voices.resize(maxVoiceCount);

        for (size_t i = 0; i < voices.size(); i++)
        {
            voices[i] = std::make_shared<AudioBufferSourceNode>(sampleRate);
            voices[i]->connect(ctx, gainNode.get(), 0, 0);
        }

    }

    SampledInstrumentNode::~SampledInstrumentNode()
    {

    }

    // @tofix - add adsr
    void SampledInstrumentNode::NoteOn(ContextRenderLock & r, const int midiNoteNumber, const float amplitude)
    {
        if (!r.context()) return;

        for (const auto & sample : samples)
        {
            // Find note in sample map
            if (sample->AppliesToNote(midiNoteNumber))
            {
                for (auto & v : voices)
                {    

                    if (v->hasFinished())
                    {
                        v->reset(r);
                    }

                    if (v->playbackState() == 0) 
                    {
                        double pitchRatio = pow(2.0, (midiNoteNumber - sample->baseMidiNote) / 12.0);

                        v->playbackRate()->setValue(pitchRatio); 
                        v->gain()->setValue(amplitude); 

                        // Connect the source node to the parsed audio data for playback
                        v->setBuffer(r, sample->soundBuf->audioBuffer);

                        v->start(r.context()->currentTime());
                        v->stop(r.context()->currentTime() + sample->soundBuf->audioBuffer->length());

                        return;
                    }
                }
            }
        }
    }

    void SampledInstrumentNode::NoteOff(ContextRenderLock & r, const int midiNoteNumber, const float amplitude)
    {
        if (!r.context()) return;
         
        // This logic is wrong. Find in voices... 
        for (const auto & sample : samples)
        {
            if (sample->AppliesToNote(midiNoteNumber))
            {
                //sample->Stop(r, midiNoteNumber, amplitude);
            }
        }
    }
    
    void SampledInstrumentNode::LoadInstrument(std::vector<SampledInstrumentDefinition> & sounds) 
    {
        for (auto & samp : sounds) 
        {
            samples.push_back(std::make_shared<SamplerSound>(samp, sampleRate));
        }

    }

}
