// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef LABSOUND_H
#define LABSOUND_H

// WebAudio Public API
#include "LabSound/core/AudioHardwareDeviceNode.h"
#include "LabSound/core/AudioHardwareInputNode.h"
#include "LabSound/core/NullDeviceNode.h"
#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioListener.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/BiquadFilterNode.h"
#include "LabSound/core/ChannelMergerNode.h"
#include "LabSound/core/ChannelSplitterNode.h"
#include "LabSound/core/ConvolverNode.h"
#include "LabSound/core/DelayNode.h"
#include "LabSound/core/DynamicsCompressorNode.h"
#include "LabSound/core/GainNode.h"
#include "LabSound/core/OscillatorNode.h"
#include "LabSound/core/PannerNode.h"
#include "LabSound/core/SampledAudioNode.h"
#include "LabSound/core/StereoPannerNode.h"
#include "LabSound/core/WaveShaperNode.h"

// LabSound Extended Public API
#include "LabSound/extended/ADSRNode.h"
#include "LabSound/extended/AudioFileReader.h"
#include "LabSound/extended/ClipNode.h"
#include "LabSound/extended/DiodeNode.h"
#include "LabSound/extended/FunctionNode.h"
#include "LabSound/extended/NoiseNode.h"
#include "LabSound/extended/PWMNode.h"
#include "LabSound/extended/PdNode.h"
#include "LabSound/extended/PeakCompNode.h"
#include "LabSound/extended/PingPongDelayNode.h"
#include "LabSound/extended/PowerMonitorNode.h"
#include "LabSound/extended/RealtimeAnalyser.h"
#include "LabSound/extended/RecorderNode.h"
#include "LabSound/extended/SampledInstrumentNode.h"
#include "LabSound/extended/SfxrNode.h"
#include "LabSound/extended/SpatializationNode.h"
#include "LabSound/extended/SpectralMonitorNode.h"
#include "LabSound/extended/SupersawNode.h"

#include <memory>

namespace lab
{
    struct AudioDeviceInfo
    {
        int32_t index {-1};
        std::string identifier; 
        uint32_t num_output_channels;
        uint32_t num_input_channels;
        std::vector<float> supported_samplerates;
        float nominal_samplerate;
        bool is_default;
    };

    // Input and Output
    struct AudioStreamConfig
    {
        int32_t device_index {-1};
        uint32_t desired_channels;
        float desired_samplerate;
    };

    std::vector<AudioDeviceInfo> MakeAudioDeviceList();
    uint32_t GetDefaultOutputAudioDeviceIndex();
    uint32_t GetDefaultInputAudioDeviceIndex();

    std::unique_ptr<AudioContext> MakeRealtimeAudioContext(const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig);
    std::unique_ptr<AudioContext> MakeOfflineAudioContext(const AudioStreamConfig offlineConfig, float recordTimeMilliseconds);

    std::shared_ptr<AudioHardwareInputNode> MakeAudioHardwareInputNode(ContextRenderLock & r);

    char const * const * const AudioNodeNames();
}

#endif
