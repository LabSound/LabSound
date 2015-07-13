// Copyright (c) 2014 Dimitri Diakopolous, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound/extended/SampledInstrumentNode.h"

#include "internal/ConfigMacros.h"

#include <json11/json11.hpp>

#include <string>
#include <fstream>
#include <streambuf>
#include <stdlib.h>

namespace LabSound 
{

	using namespace json11;
	using namespace WebCore;

	struct SamplerSound 
	{

		SamplerSound(std::shared_ptr<GainNode> d, std::string path, std::string root, std::string min, std::string max) : destinationNode(d)
		{
			audioBuffer = new SoundBuffer(path.c_str(), d->sampleRate());
			this->baseMidiNote = MakeMIDINoteFromString(root);
			this->midiNoteLow = MakeMIDINoteFromString(min);
			this->midiNoteHigh = MakeMIDINoteFromString(max);
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
			//@tofix
			// disconnect? 
		}

		// Ex: F#6. Assumes uppercase note names, hash symbol for sharp, and octave. 
		uint8_t MakeMIDINoteFromString(std::string noteName) 
		{
			const std::array<std::string, 12> midiTranslationArray = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

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

		std::string MakeStringFromMIDINote(uint8_t note)
		{
			const std::array<std::string, 12> midiTranslationArray = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

			int octave = int(note / 12) - 1;
			int positionInOctave = note % 12;

			std::string originalNote = midiTranslationArray[positionInOctave];
			std::replace(originalNote.begin(), originalNote.end(), '#', 'S');

			return  (originalNote + std::to_string(octave));
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
		for (const auto & sample : samples)
        {
			// Find note in sample map
			if (sample->AppliesToNote(midiNoteNumber))
            {
				sample->Start(r, midiNoteNumber, amplitude);
			}
		}
	}

	void SampledInstrumentNode::NoteOff(ContextRenderLock & r, const int midiNoteNumber, const float amplitude)
	{
		if (!r.context()) return;
		for (const auto & sample : samples)
        {
			if (sample->AppliesToNote(midiNoteNumber))
            {
				sample->Stop(r, midiNoteNumber, amplitude);
			}
		}
	}

	void SampledInstrumentNode::LoadInstrumentFromJSON(const std::vector<uint8_t> & jsonFile) 
	{
		std::ifstream fileStream((char*)jsonFile.data());

        if (!fileStream.is_open()) 
		{
            LOG_ERROR("JSON file failed to open");
            return;
        }
        
		std::string jsonString;
		fileStream.seekg(0, std::ios::end);
		jsonString.reserve(fileStream.tellg());
		fileStream.seekg(0, std::ios::beg);

		jsonString.assign((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());

		std::string jsonParseErr;
		auto jsonConfig = Json::parse(jsonString, jsonParseErr);

		if (jsonParseErr.empty()) 
		{
			for (const auto & samp : jsonConfig["samples"].array_items()) 
			{
				auto path = samp["sample"].string_value();
				auto root = samp["baseNote"].string_value();
				auto min = samp["lowNote"].string_value();
				auto max = samp["highNote"].string_value();

				// std::cout << "Loading Sample: " << samp.dump() << "\n";
				// std::cout << "Sample Name: " << samp["sample"].string_value() << std::endl;
                samples.emplace_back(std::make_shared<SamplerSound>(gainNode, path, root, min, max));
			}
		}
		
		else 
		{
			LOG_ERROR("JSON Parse Error: %s", jsonParseErr.c_str());
		}

	}

}
