#include "SampledInstrumentNode.h"
#include "LabSound.h"
#include "json11.hpp"
#include <string>
#include <fstream>
#include <streambuf>

namespace LabSound {

	using namespace json11;

	SampledInstrumentNode::SampledInstrumentNode(AudioContext* context, float sampleRate) : localContext(context) {

		// All samples bus their output to this node... 
		gainNode = context->createGain(); 
		gainNode->gain()->setValue(4.0); 


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

	void SampledInstrumentNode::loadInstrumentConfiguration(std::string path) {

		std::ifstream fileStream(path);
		std::string jsonString;

		fileStream.seekg(0, std::ios::end);
		jsonString.reserve(fileStream.tellg());
		fileStream.seekg(0, std::ios::beg);

		jsonString.assign((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());

		std::string err;
		auto jsonConfig = Json::parse(jsonString, err);

		if (err.empty()) {

			for (auto &samp : jsonConfig["samples"].array_items()) {
				// std::cout << "Loading Sample: " << samp.dump() << "\n";
				// std::cout << "Sample Name: " << samp["sample"].string_value() << std::endl;
				samples.push_back(new SamplerSound(
					localContext, gainNode.get(),
					samp["sample"].string_value(),
					samp["baseNote"].string_value(),
					samp["lowNote"].string_value(),
					samp["highNote"].string_value()
					));
			}

		}
		
		else {
			std::cout << "JSON Parse Error: " << err << std::endl;
		}

	}

}