// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/NullDeviceNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"

#include "internal/Assertions.h"

#include <algorithm>

using namespace lab;

static const size_t offlineRenderSizeQuantum = 128;

NullDeviceNode::NullDeviceNode(AudioContext * context, const AudioStreamConfig outputConfig, float lengthSeconds)
    : m_lengthSeconds(lengthSeconds)
    , m_context(context)
    , outConfig(outputConfig)
    , m_numChannels(outputConfig.desired_channels)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));

    m_renderBus = std::unique_ptr<AudioBus>(new AudioBus(m_numChannels, offlineRenderSizeQuantum));

    m_channelCount = m_numChannels;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Discrete;

    // We need to partially fill the the info struct here so that the context's graph thread
    // has enough initial info to make connections/disconnections appropriately
    info = {};
    info.sampling_rate = outConfig.desired_samplerate;
    info.epoch[0] = info.epoch[1] = std::chrono::high_resolution_clock::now();

    ContextGraphLock glock(context, "NullDeviceNode");
    AudioNode::setChannelCount(glock, m_numChannels);
}

NullDeviceNode::~NullDeviceNode()
{
    uninitialize();
}

void NullDeviceNode::initialize()
{
    if (isInitialized()) return;
    AudioNode::initialize();
}

void NullDeviceNode::uninitialize()
{
    if (!isInitialized()) return;

    if (m_renderThread.joinable())
    {
        m_renderThread.join();
    }

    AudioNode::uninitialize();
}

void NullDeviceNode::start()
{
    if (!m_startedRendering)
    {
        m_startedRendering = true;
        shouldExit = false;

        m_renderThread = std::thread(&NullDeviceNode::offlineRender, this);

        // @tofix - ability to update main thread from here. Currently blocks until complete
        if (m_renderThread.joinable())
        {
            m_renderThread.join();
        }

        LOG("offline rendering completed");

        if (m_context->offlineRenderCompleteCallback)
        {
            m_context->offlineRenderCompleteCallback();
        }

        m_startedRendering = false;
    }
    else
    {
        LOG("NullDeviceNode rendering already started");
    }
}

void NullDeviceNode::stop()
{
    shouldExit = true;
    info = {};
}

void NullDeviceNode::render(AudioBus * src, AudioBus * dst, size_t frames, const SamplingInfo & info)
{
    pull_graph(m_context, input(0).get(), src, dst, frames, info, nullptr);
}

const SamplingInfo NullDeviceNode::getSamplingInfo() const
{
    return info;
}

const AudioStreamConfig NullDeviceNode::getOutputConfig() const
{
    return outConfig;
}

const AudioStreamConfig NullDeviceNode::getInputConfig() const
{
    return {};
}

void NullDeviceNode::offlineRender()
{
    ASSERT(m_renderBus.get());
    if (!m_renderBus.get())
        return;

    bool isRenderBusAllocated = m_renderBus->length() >= offlineRenderSizeQuantum;
    ASSERT(isRenderBusAllocated);
    if (!isRenderBusAllocated)
        return;

    bool isAudioContextInitialized = m_context->isInitialized();
    ASSERT(isAudioContextInitialized);
    if (!isAudioContextInitialized)
        return;

    // Break up the desired length into smaller "render quantum" sized pieces.
    size_t framesToProcess = static_cast<size_t>((m_lengthSeconds * outConfig.desired_samplerate) / offlineRenderSizeQuantum);

    LOG("offline rendering started");

    while (framesToProcess > 0)
    {
        if (shouldExit == true) return;

        render(0, m_renderBus.get(), offlineRenderSizeQuantum, info);

        // Update sampling info
        const int32_t index = 1 - (info.current_sample_frame & 1);
        const uint64_t t = info.current_sample_frame & ~1;
        info.current_sample_frame = t + offlineRenderSizeQuantum + index;
        info.current_time = info.current_sample_frame / static_cast<double>(info.sampling_rate);
        info.epoch[index] = std::chrono::high_resolution_clock::now();

        framesToProcess--;
    }
}