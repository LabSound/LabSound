#include "SampledInstrumentNode.h"
#include "LabSound.h"
#include "json11/json11.hpp"
#include <string>
#include <fstream>
#include <streambuf>
#include <WTF/Assertions.h>

#if OS(WINDOWS)
#include <direct.h>
#define getcwd _getcwd
#endif
#if OS(DARWIN)
#include <limits.h>
#define MAX_PATH PATH_MAX
#endif

//

namespace LabSound {

	using namespace json11;

    SampledInstrumentNode::SampledInstrumentNode(std::shared_ptr<AudioContext> context, float sampleRate) : localContext(context) {

		// All samples bus their output to this node... 
        gainNode = std::make_shared<GainNode>(context, context->sampleRate());
		gainNode->gain()->setValue(4.0);
	}

	// Definitely have ADSR... 
	void SampledInstrumentNode::noteOn(float midiNoteNumber, float amplitude) {
	
		for (auto &sample : samples) {

			// Find note in sample map
			if (sample->appliesToNote(midiNoteNumber)) {
				sample->startNote(midiNoteNumber, amplitude);
			}

		}

	}

	void SampledInstrumentNode::loadInstrumentConfiguration(std::string path) {

		char cwd[MAX_PATH];
		getcwd(cwd, MAX_PATH);

		std::string swdString(cwd);

		std::ifstream fileStream(path);
		std::string jsonString;
        
        if ( !fileStream.is_open() ) {
            LOG_ERROR("Instrument JSON failed to open with path %s: ", path.c_str());
            return;
        }
        
		fileStream.seekg(0, std::ios::end);
		jsonString.reserve(fileStream.tellg());
		fileStream.seekg(0, std::ios::beg);

		jsonString.assign((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());

		std::string err;
		auto jsonConfig = Json::parse(jsonString, err);

		if (err.empty()) {

            auto lc = localContext.lock();
			for (auto &samp : jsonConfig["samples"].array_items()) {
				// std::cout << "Loading Sample: " << samp.dump() << "\n";
				// std::cout << "Sample Name: " << samp["sample"].string_value() << std::endl;
                samples.emplace_back(std::make_shared<SamplerSound>(
					lc, gainNode,
					samp["sample"].string_value(),
					samp["baseNote"].string_value(),
					samp["lowNote"].string_value(),
					samp["highNote"].string_value()
					));
			}

		}
		
		else {
			LOG_ERROR("JSON Parse Error: %s", err.c_str());
		}

	}

}