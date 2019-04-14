// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef LABSOUND_H
#define LABSOUND_H

// WebAudio Public API
#include "LabSound/core/WaveShaperNode.h"
#include "LabSound/core/PannerNode.h"
#include "LabSound/core/StereoPannerNode.h"
#include "LabSound/core/OscillatorNode.h"
#include "LabSound/core/OfflineAudioDestinationNode.h"
#include "LabSound/core/AudioHardwareSourceNode.h"
#include "LabSound/core/GainNode.h"
#include "LabSound/core/DynamicsCompressorNode.h"
#include "LabSound/core/DelayNode.h"
#include "LabSound/core/ConvolverNode.h"
#include "LabSound/core/ChannelSplitterNode.h"
#include "LabSound/core/ChannelMergerNode.h"
#include "LabSound/core/BiquadFilterNode.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioListener.h"
#include "LabSound/core/AudioDestinationNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/SampledAudioNode.h"
#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/AnalyserNode.h"

// LabSound Extended Public API
#include "LabSound/extended/RealtimeAnalyser.h"
#include "LabSound/extended/ADSRNode.h"
#include "LabSound/extended/ClipNode.h"
#include "LabSound/extended/DiodeNode.h"
#include "LabSound/extended/FunctionNode.h"
#include "LabSound/extended/NoiseNode.h"
#include "LabSound/extended/PdNode.h"
#include "LabSound/extended/PeakCompNode.h"
#include "LabSound/extended/PowerMonitorNode.h"
#include "LabSound/extended/PWMNode.h"
#include "LabSound/extended/SupersawNode.h"
#include "LabSound/extended/SpatializationNode.h"
#include "LabSound/extended/SpectralMonitorNode.h"
#include "LabSound/extended/SampledInstrumentNode.h"
#include "LabSound/extended/RecorderNode.h"
#include "LabSound/extended/AudioFileReader.h"

#include <functional>
#include <string>

#define LABSOUND_DEFAULT_SAMPLERATE 44100.0f
#define LABSOUND_DEFAULT_CHANNELS (uint32_t) lab::Channels::Stereo

// These are convenience functions with straightforward bodies. 
// Most LabSound samples use them, but they are not strictly required. 
namespace lab
{
    std::shared_ptr<AudioHardwareSourceNode> MakeHardwareSourceNode(ContextRenderLock & r);
    std::unique_ptr<AudioContext> MakeRealtimeAudioContext(uint32_t numChannels, float sample_rate = LABSOUND_DEFAULT_SAMPLERATE);
    std::unique_ptr<AudioContext> MakeOfflineAudioContext(uint32_t numChannels, float recordTimeMilliseconds);
    std::unique_ptr<AudioContext> MakeOfflineAudioContext(uint32_t numChannels, float recordTimeMilliseconds, float sample_rate);
}

#endif
