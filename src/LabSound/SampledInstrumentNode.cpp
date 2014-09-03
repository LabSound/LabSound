#include "SampledInstrumentNode.h"


namespace LabSound {

	SampledInstrumentNode::SampledInstrumentNode(WebCore::AudioContext*, float sampleRate) : context(context) {

		samples.push_back(new SamplerSound(context, "bells_C4.wav", "C4", "C4", "B4")); 
		samples.push_back(new SamplerSound(context, "bells_C4.wav", "C5", "C5", "B5"));

	}

	// Definitely have ADSR 
	void SampledInstrumentNode::noteOn(float midiNoteNumber, float amplitude) {
	
		for (auto &sample : samples) {

			// Find note in sample map
			if (sample.appliesToNote(midiNoteNumber)) {

				for (int j = voices.size(); --j >= 0;) {
					SamplerVoice const voice = voices[j];
					if ((voice.getCurrentlyPlayingNote() == midiNoteNumber))
						stopVoice(voice, true);
				}

				// Round robin alternative approach? 
				SamplerVoice freeVoice = findFreeVoice(mySound, shouldStealNotes);

				startVoice(freeVoice, mySound, midiChannel, midiNoteNumber, velocity);
		}

	}

	void SampledInstrumentNode::startVoice(SamplerVoice const &voice, SamplerSound const &sound, const uint8_t midiNoteNumber, const float velocity) {

	}

	void SampledInstrumentNode::stopVoice(SamplerVoice const &voice) {

	}

}