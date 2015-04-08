// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#ifndef LabSound_H
#define LabSound_H

#include "LabSoundConfig.h"

// WebAudio Public API
#include "WaveShaperNode.h"
#include "PannerNode.h"
#include "OscillatorNode.h"
#include "OfflineAudioDestinationNode.h"
#include "MediaStreamAudioSourceNode.h"
#include "MediaStreamAudioDestinationNode.h"
#include "GainNode.h"
#include "DynamicsCompressorNode.h"
#include "DelayNode.h"
#include "ConvolverNode.h"
#include "ChannelSplitterNode.h"
#include "ChannelMergerNode.h"
#include "BiquadFilterNode.h"
#include "AudioScheduledSourceNode.h"
#include "AudioParam.h"
#include "AudioNodeOutput.h"
#include "AudioNodeInput.h"
#include "AudioNode.h"
#include "AudioListener.h"
#include "AudioDestinationNode.h"
#include "AudioContext.h"
#include "AudioBufferSourceNode.h"
#include "AudioBasicProcessorNode.h"
#include "AudioBasicInspectorNode.h"
#include "AnalyserNode.h"

// LabSound Extended API 
#include "RealtimeAnalyser.h"
#include "ADSRNode.h"
#include "ClipNode.h"
#include "DiodeNode.h"
#include "NoiseNode.h"
#include "PdNode.h"
#include "PeakCompNode.h"
#include "PowerMonitorNode.h"
#include "PWMNode.h"
#include "SfxrNode.h"
#include "SoundBuffer.h"
#include "SupersawNode.h"
#include "SpatializationNode.h"
#include "SpectralMonitorNode.h"
#include "STKNode.h"
#include "SampledInstrumentNode.h"
#include "EasyVerbNode.h"
#include "RecorderNode.h"

namespace LabSound 
{

    std::shared_ptr<WebCore::AudioContext> init();

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
