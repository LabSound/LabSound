// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once
#include "LabSoundConfig.h"
#include "../../src/WTF/wtf/RefPtr.h"
#include "../../src/Modules/webaudio/AudioContext.h"
#include "../../src/LabSound/SpatializationNode.h"
#include "../../src/Modules/webaudio/AudioNode.h"
#include "../../src/Modules/webaudio/AudioBufferSourceNode.h"

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
        NodeTypePeakComp,
        NodeTypePowerMonitor,
        NodeTypePWM,
        NodeTypeRecorder,
        NodeTypeSfxr,
        NodeTypeSpatialization,
        NodeTypeSupersaw,

    };
    
    typedef RefPtr<AudioContext> AudioContextPtr;
    typedef RefPtr<SpatializationNode> SpatializationNodePtr;
    
    // init() will return an audio context object, which must be assigned to
    // a LabSound::AudioContextPtr.
    PassRefPtr<AudioContext> init();
    
    // Connect/Disconnect return true on success
    bool connect(AudioNode* thisOutput, AudioNode* toThisInput);
    bool disconnect(AudioNode* thisOutput);

    // When playing a sound from an AudioSoundBuffer, the pointer that comes
    // from play will be one of these.
    typedef RefPtr<AudioBufferSourceNode> AudioSoundBufferPtr;

}
