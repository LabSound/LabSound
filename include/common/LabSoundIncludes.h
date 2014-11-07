#pragma once

#ifndef LabSoundIncludes_h
#define LabSoundIncludes_h

// LabSound Nodes
#include "LabSound/ADSRNode.h"
#include "LabSound/ClipNode.h"
#include "LabSound/DiodeNode.h"
#include "LabSound/NoiseNode.h"
#include "LabSound/PdNode.h"
#include "LabSound/PeakCompNode.h"
#include "LabSound/PowerMonitorNode.h"
#include "LabSound/PWMNode.h"
#include "LabSound/SfxrNode.h"
#include "LabSound/SoundBuffer.h"
#include "LabSound/SupersawNode.h"
#include "LabSound/SpatializationNode.h"
#include "LabSound/SpectralMonitorNode.h"

// WebAudio Nodes
#include "LabSound/WaveShaperNode.h"
#include "LabSound/RealtimeAnalyser.h"
#include "LabSound/PannerNode.h"
#include "LabSound/OscillatorNode.h"
#include "LabSound/OfflineAudioDestinationNode.h"
#include "LabSound/MediaStreamAudioSourceNode.h"
#include "LabSound/MediaStreamAudioDestinationNode.h"
#include "LabSound/MediaElementAudioSourceNode.h"
#include "LabSound/GainNode.h"
#include "LabSound/DynamicsCompressorNode.h"
#include "LabSound/DelayNode.h"
#include "LabSound/ConvolverNode.h"
#include "LabSound/ChannelSplitterNode.h"
#include "LabSound/ChannelMergerNode.h"
#include "LabSound/BiquadFilterNode.h"
#include "LabSound/AudioScheduledSourceNode.h"

#include "LabSound/AudioProcessor.h"
#include "LabSound/AudioParamTimeline.h"
#include "LabSound/AudioParam.h"
#include "LabSound/AudioNodeOutput.h"
#include "LabSound/AudioNodeInput.h"
#include "LabSound/AudioNode.h"
#include "LabSound/AudioListener.h"
#include "LabSound/AudioDestinationNode.h"
#include "LabSound/AudioContext.h"
#include "LabSound/AudioBufferSourceNode.h"
#include "LabSound/AudioBufferCallback.h"
#include "LabSound/AudioBuffer.h"
#include "LabSound/AudioBasicProcessorNode.h"
#include "LabSound/AudioBasicInspectorNode.h"
#include "LabSound/AsyncAudioDecoder.h"
#include "LabSound/AnalyserNode.h"

// WIP Nodes
#include "../../src/LabSound/STKNode.h"
#include "../../src/LabSound/SampledInstrumentNode.h"
#include "../../src/LabSound/EasyVerbNode.h"

#endif