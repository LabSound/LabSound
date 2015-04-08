// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#ifndef LabSound_H
#define LabSound_H

// WebAudio Public API
#include "LabSound/core/WaveShaperNode.h"
#include "LabSound/core/PannerNode.h"
#include "LabSound/core/OscillatorNode.h"
#include "LabSound/core/OfflineAudioDestinationNode.h"
#include "LabSound/core/MediaStreamAudioSourceNode.h"
#include "LabSound/core/MediaStreamAudioDestinationNode.h"
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

// LabSound Extended API 
#include "LabSound/extended/RealtimeAnalyser.h"
#include "LabSound/extended/ADSRNode.h"
#include "LabSound/extended/ClipNode.h"
#include "LabSound/extended/DiodeNode.h"
#include "LabSound/extended/NoiseNode.h"
#include "LabSound/extended/PdNode.h"
#include "LabSound/extended/PeakCompNode.h"
#include "LabSound/extended/PowerMonitorNode.h"
#include "LabSound/extended/PWMNode.h"
#include "LabSound/extended/SoundBuffer.h"
#include "LabSound/extended/SupersawNode.h"
#include "LabSound/extended/SpatializationNode.h"
#include "LabSound/extended/SpectralMonitorNode.h"
#include "LabSound/extended/STKNode.h"
#include "LabSound/extended/SampledInstrumentNode.h"
#include "LabSound/extended/RecorderNode.h"

namespace LabSound 
{

    std::shared_ptr<WebCore::AudioContext> init();

    void finish(std::shared_ptr<WebCore::AudioContext> context);

}

#endif
