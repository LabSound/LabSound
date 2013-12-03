// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once
#include "LabSoundConfig.h"
#include "AudioContext.h"
#include "AudioListener.h"
#include "SpatializationNode.h"
#include "SoundBuffer.h"

namespace LabSound {

    // hoist WTF and WebCore within LabSound
    using namespace WTF;
    using namespace WebCore;

    enum NodeType {
        // These are mirrored from AudioNode::NodeType
        NodeTypeUnknown,
        NodeTypeDestination,
        NodeTypeOscillator,
        NodeTypeAudioBufferSource,
        NodeTypeMediaElementAudioSource,
        NodeTypeMediaStreamAudioDestination,
        NodeTypeMediaStreamAudioSource,
        NodeTypeJavaScript,
        NodeTypeBiquadFilter,
        NodeTypePanner,
        NodeTypeConvolver,
        NodeTypeDelay,
        NodeTypeGain,
        NodeTypeChannelSplitter,
        NodeTypeChannelMerger,
        NodeTypeAnalyser,
        NodeTypeDynamicsCompressor,
        NodeTypeWaveShaper,
        NodeTypeEnd,

        // These are new LabSound nodes
        NodeTypeADSR,
        NodeTypeClip,
        NodeTypeDiode,
        NodeTypeNoise,
        NodeTypePd,
        NodeTypeRecorder,
        NodeTypeSfxr,
        NodeTypeSpatialization,
        NodeTypeSupersaw,
        NodeTypePWM,
    };

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
    typedef WTF::RefPtr<SpatializationNode> SpatializationNodePtr;
    
    // when playing a sound from an AudioSoundBuffer, the pointer that comes
    // from play will be one of these.
    typedef WTF::RefPtr<WebCore::AudioBufferSourceNode> AudioSoundBufferPtr;
    
    // init will return an audio context object, which must be assigned to
    // a LabSound::AudioContextPtr.
    //
    WTF::PassRefPtr<WebCore::AudioContext> init();
    
    // returns true for success
    bool connect(WebCore::AudioNode* thisOutput, WebCore::AudioNode* toThisInput);
    bool disconnect(WebCore::AudioNode* thisOutput);
}
