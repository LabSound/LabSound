// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound.h"
#include "AudioContext.h"
#include "DefaultAudioDestinationNode.h"

namespace LabSound {

    std::shared_ptr<LabSound::AudioContext> init() {
        // Initialize threads for the WTF library
        WTF::initializeThreading();
        WTF::initializeMainThread();
        
        // Create an audio context object
        WebCore::ExceptionCode ec;
        std::shared_ptr<LabSound::AudioContext> context = LabSound::AudioContext::create(ec);
        context->setDestinationNode(DefaultAudioDestinationNode::create(context));
        context->initHRTFDatabase();
        context->lazyInitialize();
        return context;
    }
    
    void finish(std::shared_ptr<LabSound::AudioContext> context) {
        context->stop();
    }

    bool connect(WebCore::AudioNode* thisOutput, WebCore::AudioNode* toThisInput) {
        WebCore::ExceptionCode ec = -1;
        thisOutput->connect(toThisInput, 0, 0, ec);
        return ec == -1;
    }

    bool disconnect(WebCore::AudioNode* thisOutput) {
        WebCore::ExceptionCode ec = -1;
        thisOutput->disconnect(0, ec);
        return ec == -1;
    }

	static double MIDIToFrequency(int MIDINote) {
		return 8.1757989156 * pow(2, MIDINote / 12);
	}

	static float truncate(float d, unsigned int numberOfDecimalsToKeep) {
		return roundf(d * powf(10, numberOfDecimalsToKeep)) / powf(10, numberOfDecimalsToKeep);
	}

} // LabSound

