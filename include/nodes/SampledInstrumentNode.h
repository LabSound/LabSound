#pragma once

#include "AudioContext.h"
#include "AudioNode.h"
#include "AudioParam.h"
#include "AudioBufferSourceNode.h"
#include "ADSRNode.h"
#include "SoundBuffer.h"
#include "WTF/RefPtr.h"

#include <iostream> 
#include <array>
#include <string>
#include <algorithm>

namespace LabSound {

	using namespace WebCore;

	struct SamplerSound {

		SamplerSound(
            std::shared_ptr<AudioContext> context,
			RefPtr<GainNode> destination,
			std::string path, 
			std::string baseMidiNote, 
			std::string midiNoteLow, 
			std::string midiNoteHigh) {

			audioBuffer = new SoundBuffer(context, path.c_str()); 

			this->baseMidiNote = getMIDIFromNoteString(baseMidiNote);
			this->midiNoteLow = getMIDIFromNoteString(midiNoteLow);
			this->midiNoteHigh = getMIDIFromNoteString(midiNoteHigh);

			this->destinationNode = destination;

		}

		bool appliesToNote(uint8_t note) {

			//std::cout << "Note: " << int(note) << std::endl;
			//std::cout << "Base: " << int(baseMidiNote) << std::endl;
			//std::cout << "Low: " << int(midiNoteLow) << std::endl;
			//std::cout << "High: " << int(midiNoteHigh) << std::endl;
			//std::cout << std::endl << std::endl;

			if (baseMidiNote == note) {
				return true; 
			} else if (note >= midiNoteLow && note <= midiNoteHigh) {
				return true; 
			}

			else return false; 

		}

		PassRefPtr<AudioBufferSourceNode> startNote(uint8_t midiNoteNumber, float amplitude = 1.0) {

			// var semitoneRatio = Math.pow(2, 1/12);
			double pitchRatio = pow(2.0, (midiNoteNumber - baseMidiNote) / 12.0);

			RefPtr<AudioBufferSourceNode> theSample = audioBuffer->create();

			theSample->playbackRate()->setValue(pitchRatio); 
			theSample->gain()->setValue(amplitude); 

			// Connect the source node to the parsed audio data for playback
			theSample->setBuffer(audioBuffer->audioBuffer.get());

			// Bus the sound to the output destination .
			WebCore::ExceptionCode ec;
			theSample->connect(destinationNode.get(), 0, 0, ec);
			theSample->start(0.0);

			return theSample;

		}

		// Ex: F#6. Assumes uppercase note names, hash symbol, and octave. 
		uint8_t getMIDIFromNoteString(std::string noteName) {

			std::array<std::string, 12> midiTranslationArray = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

			// Ocatve is always last character, as an integer 
			std::string octaveString = noteName.substr(noteName.length() - 1, 1);
			int octave = std::stoi(octaveString);

			std::string noteString = noteName.erase(noteName.length() - 1, 1); 

			// Uppercase the incoming note 
			std::transform(noteString.begin(), noteString.end(), noteString.begin(), ::toupper);

			// IF we don't use # notation, convert S to #
			std::replace(noteString.begin(), noteString.end(), 'S', '#'); // replace all 'x' to 'y'

			// Note name is now the first or second character 
			int notePos = -1;
			for (int i = 0; i < 12; ++i) {
				if (noteString == midiTranslationArray[i]) {
					notePos = i;
					break;
				}
			}

			return uint8_t((octave * 12.0) + notePos);

		}

		std::string getNoteStringFromMIDI(uint8_t note) {

			std::array<std::string, 12> midiTranslationArray = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

			int octave = int(note / 12) - 1;
			int positionInOctave = note % 12;

			std::string originalNote = midiTranslationArray[positionInOctave];

			std::replace(originalNote.begin(), originalNote.end(), '#', 'S'); // replace all 'x' to 'y'

			return  originalNote + std::to_string(octave);

		}

		void stopNote(); 

		RefPtr<GainNode> destinationNode;

		SoundBuffer *audioBuffer;

		uint8_t baseMidiNote;
		uint8_t midiNoteLow;
		uint8_t midiNoteHigh; 

	};

	class SampledInstrumentNode {

	public:

        static PassRefPtr<SampledInstrumentNode> create(std::shared_ptr<AudioContext> context, float sampleRate) {
			return adoptRef(new SampledInstrumentNode(context, sampleRate));
		}

		void loadInstrumentConfiguration(std::string path);

		void noteOn(float frequency, float amplitude);
		float noteOff(float amplitude); 

		void stopAll(); 

		RefPtr<GainNode> gainNode;

	private:

		// void startVoice(SamplerVoice const &voice, SamplerSound const &sound, const uint8_t midiNoteNumber, const float velocity);
		// void stopVoice(SamplerVoice const &voice);

		// Sample Map => Audio Buffers

		// Loop? 
		// Filter? 

		// Blech
		std::vector<SamplerSound*> samples;
        std::weak_ptr<WebCore::AudioContext> localContext;

        SampledInstrumentNode(std::shared_ptr<AudioContext>, float sampleRate);

		// Satisfy the AudioNode interface
		//virtual void process(size_t) {}
		//virtual void reset() {}
		//virtual double tailTime() const OVERRIDE { return 0; }
		//virtual double latencyTime() const OVERRIDE { return 0; }
		//virtual bool propagatesSilence() const OVERRIDE { return 1; }

	};

} // LabSound 