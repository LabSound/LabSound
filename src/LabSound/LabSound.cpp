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

    
    std::timed_mutex mutex;
    std::thread* soundThread = nullptr;
    std::shared_ptr<LabSound::AudioContext> mainContext;
    
    const int updateRate_ms = 10;

    static void update() {
        while (true) {
            std::chrono::milliseconds sleepDuration(updateRate_ms);
            std::this_thread::sleep_for(sleepDuration);
            if (mainContext) {
                ContextGraphLock g(mainContext, "LabSound::update");
                mainContext->update(g);
            }
            else {
                // thread is finished
                break;
            }
        }
        printf("LabSound Audio thread finished\n");
    }
    
    std::shared_ptr<LabSound::AudioContext> init() {
        // Initialize threads for the WTF library
        WTF::initializeThreading();
        WTF::initializeMainThread();
        
        // Create an audio context object with the default audio destination
        ExceptionCode ec;
        mainContext = LabSound::AudioContext::create(ec);
        mainContext->setDestinationNode(std::make_shared<DefaultAudioDestinationNode>(mainContext));
        mainContext->initHRTFDatabase();
        mainContext->lazyInitialize();

        soundThread = new std::thread(update);
        soundThread->join();
        
        return mainContext;
    }

    
    void finish(std::shared_ptr<LabSound::AudioContext> context) {
        
        // stop sound thread
        mainContext.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(updateRate_ms * 2));
        
        for (int i = 0; i < 10; ++i) {
            ContextGraphLock g(context, "LabSound::finish");
            if (!g.context()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            else {
                context->stop(g);
                context->deleteMarkedNodes();
                context->uninitialize(g);
                return;
            }
        }
        std::cerr << "LabSound could not acquire lock for shutdown" << std::endl;
    }

    bool connect(ContextGraphLock& g, WebCore::AudioNode* thisOutput, WebCore::AudioNode* toThisInput) {
        ExceptionCode ec = NO_ERR;
        thisOutput->connect(g.context(), toThisInput, 0, 0, ec);
        return ec == NO_ERR;
    }

    bool disconnect(ContextGraphLock& g, WebCore::AudioNode* thisOutput) {
        ExceptionCode ec = NO_ERR;
        thisOutput->disconnect(g.context(), 0, ec);
        return ec == NO_ERR;
    }

} // LabSound

