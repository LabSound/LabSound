#include "../Modules/webaudio/AudioContext.h"
#include "../Modules/webaudio/AudioNode.h"
#include "../Modules/webaudio/AudioParam.h"
#include "SoundBuffer.h"
#include <array>

namespace LabSound {
	
	struct SamplerSound {

		SamplerSound(RefPtr<WebCore::AudioContext> context, std::string path, std::string baseMidiNote, std::string midiNoteLow, std::string midiNoteHigh) {

			audioBuffer = new SoundBuffer(context, path.c_str()); 

			this->context = context;
			this->baseMidiNote = SampledInstrumentNode::getMIDIFromNoteString(baseMidiNote); 
			this->midiNoteLow = SampledInstrumentNode::getMIDIFromNoteString(midiNoteLow);
			this->midiNoteHigh = SampledInstrumentNode::getMIDIFromNoteString(midiNoteHigh);

		}

		const std::string& getPath() const; 

		bool appliesToNote(uint8_t note) {

			if (baseMidiNote == note) {
				return true; 
			} else if (midiNoteLow >= note && midiNoteHigh <= note) {
				return true; 
			}

			else return false; 

		}

		PassRefPtr<AudioBufferSourceNode> startNote(uint8_t midiNoteNumber, SamplerSound &sound) {

			// var semitoneRatio = Math.pow(2, 1/12);
			double pitchRatio = pow(2.0, (midiNoteNumber - sound.baseMidiNote) / 12.0);

			RefPtr<AudioBufferSourceNode> theSample = audioBuffer->create();

			theSample->playbackRate()->setValue(pitchRatio); 

			// Connect the source node to the parsed audio data for playback
			theSample->setBuffer(audioBuffer->audioBuffer.get());

			// Bus the sound to the mixer.
			WebCore::ExceptionCode ec;
			theSample->connect(context->destination(), 0, 0, ec);
			theSample->start(0.0);

			return theSample;

		}

		void stopNote(); 

		RefPtr<WebCore::AudioContext> context;

		RefPtr<SoundBuffer> audioBuffer;

		uint8_t baseMidiNote;
		uint8_t midiNoteLow;
		uint8_t midiNoteHigh; 

	};


	using namespace WebCore;

	class SampledInstrumentNode : public AudioNode {

	public:

		static PassRefPtr<SampledInstrumentNode> create(AudioContext* context, float sampleRate) {
			return adoptRef(new SampledInstrumentNode(context, sampleRate));
		}

		void loadInstrumentFromPath(std::string path);

		void noteOn(float frequency, float amplitude);
		float noteOff(float amplitude); 

		void stopAll(); 

		static double MIDIToFrequency(int MIDINote) {
			return 8.1757989156 * pow(2, MIDINote / 12);
		}

		static float truncate(float d, unsigned int numberOfDecimalsToKeep) {
			return roundf(d * powf(10, numberOfDecimalsToKeep)) / powf(10, numberOfDecimalsToKeep);
		}

		// Ex: F#6. Assumes uppercase note names, hash symbol, and octave. 
		static float getMIDIFromNoteString(std::string noteName) {

			std::array<std::string, 12> midiTranslationArray = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

			// Ocatve is always last character, as an integer 
			std::string octaveString = noteName.erase(noteName.length() - 1, 1);
			int octave = std::stoi(octaveString);

			// Note name is now the first or second character 
			int notePos = -1;
			for (int i = 0; i < 12; ++i) {
				if (noteName == midiTranslationArray[i]) {
					notePos = i;
					break;
				}
			}

			return (octave * 12.0) + notePos;

		}

		static std::string getNoteStringFromMIDI(uint8_t note) {

			std::array<std::string, 12> midiTranslationArray = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

			int octave = int(note / 12) - 1;
			int positionInOctave = note % 12;

			return midiTranslationArray[positionInOctave] + std::to_string(octave);

		}

	private:

		void startVoice(SamplerVoice const &voice, SamplerSound const &sound, const uint8_t midiNoteNumber, const float velocity);
		void stopVoice(SamplerVoice const &voice);

		// Sample Map => Audio Buffers

		// Loop? 
		// ADSR? 
		// Filter? 

		std::vector<SamplerSound*> samples;

		RefPtr<WebCore::AudioContext> context;

		SampledInstrumentNode(WebCore::AudioContext*, float sampleRate);

		// Satisfy the AudioNode interface
		virtual void process(size_t);

		virtual void reset() { /*m_currentSampleFrame = 0;*/ }

		virtual double tailTime() const OVERRIDE{ return 0; }
		virtual double latencyTime() const OVERRIDE{ return 0; }
		virtual bool propagatesSilence() const OVERRIDE;

	};

} // LabSound 