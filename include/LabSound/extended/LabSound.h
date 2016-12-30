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
#include "LabSound/core/AudioBufferSourceNode.h"
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
#include "LabSound/extended/SoundBuffer.h"
#include "LabSound/extended/SupersawNode.h"
#include "LabSound/extended/SpatializationNode.h"
#include "LabSound/extended/SpectralMonitorNode.h"
#include "LabSound/extended/SampledInstrumentNode.h"
#include "LabSound/extended/RecorderNode.h"

#include <functional>
#include <string>

namespace lab
{
    std::shared_ptr<AudioContext> MakeAudioContext();
    std::shared_ptr<AudioContext> MakeOfflineAudioContext(const int millisecondsToRun);
    std::shared_ptr<AudioContext> MakeOfflineAudioContext(int numChannels, size_t frames, float sample_rate);
    void CleanupAudioContext(std::shared_ptr<AudioContext> context);
    void AcquireLocksForContext(const std::string id, std::shared_ptr<AudioContext> & ctx, std::function<void(ContextGraphLock & g, ContextRenderLock & r)> callback);
}

#endif
