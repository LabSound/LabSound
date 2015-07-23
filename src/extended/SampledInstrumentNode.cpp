// Copyright (c) 2014 Dimitri Diakopolous, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound/extended/SampledInstrumentNode.h"

#include "internal/ConfigMacros.h"

#include <string>
#include <fstream>
#include <streambuf>
#include <stdlib.h>

namespace LabSound 
{

	using namespace WebCore;

	struct SamplerSound 
	{

		SamplerSound(std::shared_ptr<GainNode> d, SampledInstrumentDefinition & def) : destinationNode(d)
		{
			audioBuffer = new SoundBuffer(def.audio, def.extension, d->sampleRate());
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

		std::shared_ptr<AudioBufferSourceNode> Start(ContextRenderLock & r, int midiNoteNumber, float amplitude = 1.0) 
		{
            if (!r.context()) throw std::runtime_error("cannot get context");
            
			double pitchRatio = pow(2.0, (midiNoteNumber - baseMidiNote) / 12.0);

			std::shared_ptr<AudioBufferSourceNode> theSample(audioBuffer->create(r, r.context()->sampleRate()));

			theSample->playbackRate()->setValue(pitchRatio); 
			theSample->gain()->setValue(amplitude); 

			// Connect the source node to the parsed audio data for playback
			theSample->setBuffer(r, audioBuffer->audioBuffer);

			theSample->connect(r.context(), destinationNode.get(), 0, 0);
			theSample->start(0.0);

			return theSample;
		}

		void Stop(ContextRenderLock & r, int midiNoteNumber, float amplitude = 0.0)
		{
			//@tofix -- disconnect? 
		}

        std::shared_ptr<GainNode> destinationNode;

		SoundBuffer * audioBuffer;

		uint8_t baseMidiNote;
		uint8_t midiNoteLow;
		uint8_t midiNoteHigh; 
	};

    SampledInstrumentNode::SampledInstrumentNode(float sampleRate) 
	{ 
        gainNode = std::make_shared<GainNode>(sampleRate);
		gainNode->gain()->setValue(1.0);
	}

	SampledInstrumentNode::~SampledInstrumentNode()
	{

	}

	//@tofix: add adsr
	void SampledInstrumentNode::NoteOn(ContextRenderLock & r, const int midiNoteNumber, const float amplitude)
    {
        if (!r.context()) return;

		//Debug: useful for monitoring expected voice usage
		//std::cout << "Size: " << voices.size() << std::endl;

		for (const auto & sample : samples)
        {
			// Find note in sample map
			if (sample->AppliesToNote(midiNoteNumber))
            {
				for (auto & v : voices)
				{	
					if (v->hasFinished())
					{
						v = sample->Start(r, midiNoteNumber, amplitude); // this voice is free -> destruct
						return;
					}
				}

				// create new voice
				voices.push_back(sample->Start(r, midiNoteNumber, amplitude));
				return; 

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
				sample->Stop(r, midiNoteNumber, amplitude);
			}
		}
	}
	
	void SampledInstrumentNode::LoadInstrument(std::vector<SampledInstrumentDefinition> & sounds) 
	{
		for (auto & samp : sounds) 
		{
            samples.push_back(std::make_shared<SamplerSound>(gainNode, samp));
		}

	}

}
