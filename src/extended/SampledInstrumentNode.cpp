#include "LabSound/extended/SampledInstrumentNode.h"

#include "internal/ConfigMacros.h"

#include <json11/json11.hpp>

#include <string>
#include <fstream>
#include <streambuf>
#include <stdlib.h>

#if OS(WINDOWS)
	#include <direct.h>
	#define getcwd _getcwd
	#define MAX_PATH _MAX_PATH
#endif

#if OS(DARWIN)
	#include <limits.h>
	#include <unistd.h>
	#define MAX_PATH PATH_MAX
#endif

namespace LabSound {

	using namespace json11;

    SampledInstrumentNode::SampledInstrumentNode(float sampleRate) {

		// All samples bus their output to this node... 
        gainNode = std::make_shared<GainNode>(sampleRate);
		gainNode->gain()->setValue(4.0);
        //initialize(); note - not subclassed from AudioNode
	}

	// Definitely have ADSR... 
	void SampledInstrumentNode::noteOn(ContextRenderLock& r, float midiNoteNumber, float amplitude)
    {
        auto ac = r.context();
        if (!ac) return;
        
		for (auto &sample : samples)
        {
			// Find note in sample map
			if (sample->appliesToNote(midiNoteNumber))
            {
				sample->startNote(r, midiNoteNumber, amplitude);
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

			for (auto &samp : jsonConfig["samples"].array_items()) {
				// std::cout << "Loading Sample: " << samp.dump() << "\n";
				// std::cout << "Sample Name: " << samp["sample"].string_value() << std::endl;
                samples.emplace_back(std::make_shared<SamplerSound>(
					gainNode,
					samp["sample"].string_value(),
					samp["baseNote"].string_value(),
					samp["lowNote"].string_value(),
					samp["highNote"].string_value(),
                    44100
					));
			}

		}
		
		else 
		{
			LOG_ERROR("JSON Parse Error: %s", err.c_str());
		}

	}

}
