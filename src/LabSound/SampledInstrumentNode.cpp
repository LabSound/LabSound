#include "SampledInstrumentNode.h"


namespace LabSound {

	SampledInstrumentNode::SampledInstrumentNode(AudioContext* context, float sampleRate) : localContext(context) {

		samples.push_back(new SamplerSound(context, "cello_pluck_As0.wav", "As0", "C0", "Cs1")); 
		samples.push_back(new SamplerSound(context, "cello_pluck_Ds1.wav", "Ds1", "D1", "Fs1"));
		samples.push_back(new SamplerSound(context, "cello_pluck_Gs1.wav", "Gs1", "G1", "B1"));
		samples.push_back(new SamplerSound(context, "cello_pluck_Cs2.wav", "Cs2", "C2", "E2"));
		samples.push_back(new SamplerSound(context, "cello_pluck_Fs2.wav", "Fs2", "F2", "A2"));
		samples.push_back(new SamplerSound(context, "cello_pluck_B2.wav", "B2", "As2", "D3"));
		samples.push_back(new SamplerSound(context, "cello_pluck_E3.wav", "E3", "Ds3", "G3"));
		samples.push_back(new SamplerSound(context, "cello_pluck_A3.wav", "A3", "Gs3", "A3"));

	}

	// Definitely have ADSR... 
	void SampledInstrumentNode::noteOn(float midiNoteNumber, float amplitude) {
	
		for (auto &sample : samples) {

			// Find note in sample map
			if (sample->appliesToNote(midiNoteNumber)) {
				sample->startNote(midiNoteNumber);
			}

		}

	}

}