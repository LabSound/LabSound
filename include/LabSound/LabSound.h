// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once
#include "LabSoundConfig.h"
#include "AudioContext.h"
#include "AudioListener.h"
#include "SpatializationNode.h"
#include "SoundBuffer.h"

namespace LabSound {

    // Hoist WTF and WebCore within LabSound namespace
    using namespace WTF;
    using namespace WebCore;

    enum NodeType {

        // Base WebAudio nodes
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

        // LabSound nodes
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
    
    typedef WTF::RefPtr<WebCore::AudioContext> AudioContextPtr;
    typedef WTF::RefPtr<SpatializationNode> SpatializationNodePtr;
    
    // init() will return an audio context object, which must be assigned to
    // a LabSound::AudioContextPtr.
    WTF::PassRefPtr<WebCore::AudioContext> init();
    
    // Connect/Disconnect return true on success
    bool connect(WebCore::AudioNode* thisOutput, WebCore::AudioNode* toThisInput);
    bool disconnect(WebCore::AudioNode* thisOutput);

    // When playing a sound from an AudioSoundBuffer, the pointer that comes
    // from play will be one of these.
    typedef WTF::RefPtr<WebCore::AudioBufferSourceNode> AudioSoundBufferPtr;

}
