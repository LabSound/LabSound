// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef LABSOUND_H
#define LABSOUND_H

#include "LabSoundConfig.h"
#include "SpatializationNode.h"
#include "AudioNode.h"
#include "AudioBufferSourceNode.h"

namespace LabSound 
{

    // init() will return an audio context object
    std::shared_ptr<WebCore::AudioContext> init();
    
    // when done with the context call finish
    void finish(std::shared_ptr<WebCore::AudioContext> context);

	enum NodeType 
	{
        // Core Webaudio Nodes
        NodeTypeUnknown,
        NodeTypeDestination,
        NodeTypeOscillator,
        NodeTypeAudioBufferSource,
        NodeTypeMediaElementAudioSource,
        NodeTypeMediaStreamAudioDestination,
        NodeTypeMediaStreamAudioSource,
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

        // Labsound Extensions
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
}

#endif
