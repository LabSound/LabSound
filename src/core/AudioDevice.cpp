// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioSourceProvider.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"

// Non platform-specific helper functions

using namespace lab;


AudioNodeDescriptor * AudioDestinationNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr};
    return &d;
}

namespace lab {

AudioDestinationNode::AudioDestinationNode(
    AudioContext& ac,
    std::shared_ptr<AudioDevice> device)
: AudioNode(ac, *desc())
, _context(&ac)
, _platformAudioDevice(device)
{
    // Node-specific default mixing rules.
    //_self->m_channelCountMode = ChannelCountMode::Explicit;
    _self->m_channelInterpretation = ChannelInterpretation::Speakers;

    int numChannels = _platformAudioDevice->getOutputConfig().desired_channels;
    ContextGraphLock glock(&ac, "AudioDestinationNode");
    AudioNode::setChannelCount(numChannels);

    initialize();
}

#if 0
void AudioDestinationNode::offlineRender(AudioBus* dst, size_t framesToProcess)
{
    static const int offlineRenderSizeQuantum = AudioNode::ProcessingSizeInFrames;

    if (!dst || !framesToProcess || !_context || !_context->isInitialized())
        return;

    bool isRenderBusAllocated = dst->length() >= offlineRenderSizeQuantum;
    ASSERT(isRenderBusAllocated);
    if (!isRenderBusAllocated)
        return;

    LOG_TRACE("offline rendering started");

    while (framesToProcess > 0) {
        _context->update();
        AudioSourceProvider* asp = nullptr;
        render(_context, asp, nullptr, dst, _last_info);

        // Update sampling info
        const int index = 1 - (_last_info.current_sample_frame & 1);
        const uint64_t t = _last_info.current_sample_frame & ~1;
        _last_info.current_sample_frame = t + offlineRenderSizeQuantum + index;
        _last_info.current_time = _last_info.current_sample_frame / static_cast<double>(_last_info.sampling_rate);
        _last_info.epoch[index] += std::chrono::nanoseconds {
                static_cast<uint64_t>(1.e9 * (double) framesToProcess / (double) offlineRenderSizeQuantum)};

        framesToProcess--;
    }
}
#endif

void AudioDestinationNode::initialize()
{
    if (!isInitialized())
        AudioNode::initialize();
}

void AudioDestinationNode::uninitialize()
{
    if (!isInitialized())
        return;
    
    _platformAudioDevice->stop();
    AudioNode::uninitialize();
    _platformAudioDevice.reset();
}

void AudioDestinationNode::reset(ContextRenderLock& r)
{
    AudioNode::reset(r);
    _platformAudioDevice->stop();
    _platformAudioDevice->start();
}

} // lab
