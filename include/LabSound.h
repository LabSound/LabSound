// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef LABSOUND_H
#define LABSOUND_H

#include "LabSoundConfig.h"
#include "SpatializationNode.h"
#include "AudioNode.h"
#include "AudioBufferSourceNode.h"

namespace LabSound {

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
        NodeTypeSpectralMonitor,
        NodeTypeSupersaw,
		NodeTypeSTK, 

    };
    
    // init() will return an audio context object
    std::shared_ptr<AudioContext> init();
    
    // when done with the context call finish
    void finish(std::shared_ptr<LabSound::AudioContext> context);
}

#endif
