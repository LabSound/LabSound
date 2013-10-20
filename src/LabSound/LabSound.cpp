//
//  LabSound.cpp
//  LabSound
//
//  Created by Nick Porcino on 2013 10/18.
//
//

#include "LabSound.h"
#include "AudioContext.h"

namespace WebCore
{
    class Document { public: };
}

namespace LabSound {

    PassRefPtr<WebCore::AudioContext> init() {
        // Initialize threads for the WTF library
        WTF::initializeThreading();
        WTF::initializeMainThread();
        
        // Create an audio context object
        WebCore::Document d;
        WebCore::ExceptionCode ec;
        return WebCore::AudioContext::create(&d, ec);
    }

    bool connect(WebCore::AudioNode* thisOutput, WebCore::AudioNode* toThisInput) {
        WebCore::ExceptionCode ec = -1;
        thisOutput->connect(toThisInput, 0, 0, ec);
        return ec == -1;
    }

} // LabSound

