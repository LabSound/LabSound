//
//  LabSound.h
//  LabSound
//
//  Created by Nick Porcino on 2013 10/18.
//
//

#pragma once
#include "AudioContext.h"
#include "AudioListener.h"
#include "PannerNode.h"
#include "SoundBuffer.h"

namespace LabSound {
    
    // OSX will need the following dependencies:
    //
    // Accelerate.framework
    // AudioToolbox.framework
    // AudioUnit.framework
    // CoreAudio.framework
    // libicucore.dylib
    //
    // libpd will also need to be linked if PdNode is used
    //
    // and the following include paths:
    //
    // src/WTF src/WTF/icu src/platform/audio src/platform/graphics src/Modules/webaudio src/shim src/LabSound src/Samples src/libpd/cpp
    
    typedef WTF::RefPtr<WebCore::AudioContext> AudioContextPtr;
    typedef WTF::RefPtr<WebCore::PannerNode> PannerNodePtr;
    
    // when playing a sound from an AudioSoundBuffer, the pointer that comes
    // from play will be one of these.
    typedef WTF::RefPtr<WebCore::AudioBufferSourceNode> AudioSoundBufferPtr;
    
    // init will return an audio context object, which must be assigned to
    // a LabSound::AudioContextPtr.
    //
    WTF::PassRefPtr<WebCore::AudioContext> init();
    
    // returns true for success
    bool connect(WebCore::AudioNode* thisOutput, WebCore::AudioNode* toThisInput);
}
