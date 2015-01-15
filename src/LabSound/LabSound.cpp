// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound.h"
#include "AudioContext.h"
#include "AudioContextLock.h"
#include "ExceptionCodes.h"
#include "DefaultAudioDestinationNode.h"
#include "WTF/MainThread.h"
#include <chrono>
#include <thread>
#include <iostream>

namespace LabSound {

    std::shared_ptr<LabSound::AudioContext> init() {
        // Initialize threads for the WTF library
        WTF::initializeThreading();
        WTF::initializeMainThread();
        
        // Create an audio context object
        ExceptionCode ec;
        std::shared_ptr<LabSound::AudioContext> context = LabSound::AudioContext::create(ec);
        context->setDestinationNode(std::make_shared<DefaultAudioDestinationNode>(context));
        context->initHRTFDatabase();
        context->lazyInitialize();
        return context;
    }


    void update(std::shared_ptr<LabSound::AudioContext> context) {

    }
    
    void finish(std::shared_ptr<LabSound::AudioContext> context) {
        for (int i = 0; i < 10; ++i) {
            ContextGraphLock g(context);
            if (!g.context()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            else {
                context->stop(g);
                return;
            }
        }
        std::cerr << "LabSound could not acquire lock for shutdown" << std::endl;
    }

    bool connect(ContextGraphLock& g, ContextRenderLock& r, WebCore::AudioNode* thisOutput, WebCore::AudioNode* toThisInput) {
        ExceptionCode ec = NO_ERR;
        thisOutput->connect(g, r, toThisInput, 0, 0, ec);
        return ec == NO_ERR;
    }

    bool disconnect(ContextGraphLock& g, ContextRenderLock& r, WebCore::AudioNode* thisOutput) {
        ExceptionCode ec = NO_ERR;
        thisOutput->disconnect(g, r, 0, ec);
        return ec == NO_ERR;
    }

	static double MIDIToFrequency(int MIDINote) {
		return 8.1757989156 * pow(2, MIDINote / 12);
	}

	static float truncate(float d, unsigned int numberOfDecimalsToKeep) {
		return roundf(d * powf(10, numberOfDecimalsToKeep)) / powf(10, numberOfDecimalsToKeep);
	}

} // LabSound

