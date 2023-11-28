// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSourceProvider.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/DenormalDisabler.h"

// Non platform-specific helper functions

using namespace lab;



void lab::pull_graph(
        AudioContext * ctx,
        AudioNodeInput * required_inlet,
        AudioBus * src, AudioBus * dst,
        int frames,
        const SamplingInfo & info,
        AudioSourceProvider * optional_hardware_input)
{
    // The audio system might still be invoking callbacks during shutdown,
    // so bail out if called without a context
    if (!ctx) 
        return;

    // bail if shutting down.
    auto ac = ctx->audioContextInterface().lock();
    if (!ac)
        return;

    ASSERT(required_inlet);

    ContextRenderLock renderLock(ctx, "lab::pull_graph");
    if (!renderLock.context()) return;  // return if couldn't acquire lock

    if (!ctx->isInitialized())
    {
        if (dst)
            dst->zero();
        return;
    }

    // Denormals can slow down audio processing.
    // Use an RAII object to protect all AudioNodes processed within this scope.

    /// @TODO under what circumstance do they arise?
    /// If they come from input data such as loaded WAV files, they should be corrected
    /// at source. If they can result from signal processing; again, where? The
    /// signal processing should not produce denormalized values.

    DenormalDisabler denormalDisabler;

    // Let the context take care of any business at the start of each render quantum.
    ctx->handlePreRenderTasks(renderLock);

    // Prepare the local audio input provider for this render quantum.
    if (optional_hardware_input && src)
    {
        optional_hardware_input->set(src);
    }

    // process the graph by pulling the inputs, which will recurse the entire processing graph.
    AudioBus * renderedBus = required_inlet->pull(renderLock, dst, frames);

    if (dst) {
        if (!renderedBus)
        {
            dst->zero();
        }
        else if (renderedBus != dst)
        {
            // in-place processing was not possible - so copy
            dst->copyFrom(*renderedBus);
        }
    }

    // Process nodes which need extra help because they are not connected to anything,
    // but still have work to do
    ctx->processAutomaticPullNodes(renderLock, frames);

    // Let the context take care of any business at the end of each render quantum.
    ctx->handlePostRenderTasks(renderLock);
}

namespace lab {


AudioNodeDescriptor * AudioDestinationNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr, 2};
    return &d;
}

AudioDestinationNode::AudioDestinationNode(
    AudioContext& ac,
    std::shared_ptr<AudioDevice> device)
: AudioNode(ac, {nullptr, nullptr, (int) device->getOutputConfig().desired_channels})
, _context(&ac)
, _platformAudioDevice(device)
{
    // This is the "final node" in the chain. It will pull on all others from this input.
    addInput(std::make_unique<AudioNodeInput>(this));

    // Node-specific default mixing rules.
    _self->m_channelCountMode = ChannelCountMode::Explicit;
    _self->m_channelInterpretation = ChannelInterpretation::Speakers;

    // Info is provided by the backend every frame, but some nodes need to be constructed
    // with a valid sample rate before the first frame so we make our best guess here
    _last_info = {};
    _last_info.sampling_rate = _platformAudioDevice->getOutputConfig().desired_samplerate;

    initialize();
}

void AudioDestinationNode::render(AudioSourceProvider* provider,
        AudioBus * src, AudioBus * dst,
        int frames,
        const SamplingInfo & info)
{
    ProfileScope selfProfile(_self->totalTime);
    ProfileScope profile(_self->graphTime);
    pull_graph(_context, input(0).get(), src, dst, frames, info, provider);
    _last_info = info;
    profile.finalize();
    selfProfile.finalize();
}

void AudioDestinationNode::offlineRender(AudioBus * dst, int framesToProcess)
{
    static const int offlineRenderSizeQuantum = AudioNode::ProcessingSizeInFrames;

    if (!dst || !framesToProcess || !_context || !_context->isInitialized())
        return;

    bool isRenderBusAllocated = dst->length() >= offlineRenderSizeQuantum;
    ASSERT(isRenderBusAllocated);
    if (!isRenderBusAllocated)
        return;

    LOG_TRACE("offline rendering started");

    while (framesToProcess > 0)
    {
        _context->update();
        AudioSourceProvider* asp = nullptr;
        render(asp, 0, dst, offlineRenderSizeQuantum, _last_info);

        // Update sampling info
        const int index = 1 - (_last_info.current_sample_frame & 1);
        const uint64_t t = _last_info.current_sample_frame & ~1;
        _last_info.current_sample_frame = t + offlineRenderSizeQuantum + index;
        _last_info.current_time = _last_info.current_sample_frame / static_cast<double>(_last_info.sampling_rate);
        _last_info.epoch[index] += std::chrono::nanoseconds {
                static_cast<uint64_t>(1.e9 * (double) framesToProcess / (double) offlineRenderSizeQuantum)};

        framesToProcess -= offlineRenderSizeQuantum;
    }
}

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

void AudioDestinationNode::reset(ContextRenderLock &)
{
    _platformAudioDevice->stop();
    _platformAudioDevice->start();
}

}
