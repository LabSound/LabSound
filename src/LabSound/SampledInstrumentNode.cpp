#include "SampledInstrumentNode.h"


namespace LabSound {

	SampledInstrumentNode::SampledInstrumentNode(AudioContext* context, float sampleRate) : localContext(context) {

		samples.push_back(new SamplerSound(context, "bells_C4.wav", "C4", "C4", "B4")); 
		samples.push_back(new SamplerSound(context, "bells_C4.wav", "C5", "C5", "B5"));

	}

	// Definitely have ADSR... 
	void SampledInstrumentNode::noteOn(float midiNoteNumber, float amplitude) {
	
		for (auto &sample : samples) {

			// Find note in sample map
			std::cout << sample->appliesToNote(midiNoteNumber) << std::endl; 
			if (sample->appliesToNote(midiNoteNumber)) {
				sample->startNote(midiNoteNumber);
			}

		}

	}

}