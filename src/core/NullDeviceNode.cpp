// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/NullDeviceNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"
#include "LabSound/core/AudioNodeInput.h"

#include "internal/Assertions.h"

#include <algorithm>

using namespace std;
 
namespace lab {
    
static const size_t offlineRenderSizeQuantum = 4096;    

NullDeviceNode::NullDeviceNode(AudioContext * context, const AudioStreamConfig outputConfig, float lengthSeconds) 
    : m_lengthSeconds(lengthSeconds), m_context(context)
{
    m_numChannels = outputConfig.desired_channels;
    m_renderBus = std::unique_ptr<AudioBus>(new AudioBus(m_numChannels, offlineRenderSizeQuantum));
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
}

NullDeviceNode::~NullDeviceNode()
{
    uninitialize();
}

void NullDeviceNode::initialize()
{
    if (isInitialized())
        return;

    AudioNode::initialize();
}

void NullDeviceNode::uninitialize()
{
    if (!isInitialized())
        return;

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

        m_renderThread = std::thread(&NullDeviceNode::offlineRender, this);
        
        // @tofix - ability to update main thread from here. Currently blocks until complete
        if (m_renderThread.joinable()) m_renderThread.join();

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
    // @fixme
}

void NullDeviceNode::render(AudioBus * src, AudioBus * dst, size_t frames, const SamplingInfo & info)
{
    pull_graph(m_context, input(0).get(), src, dst, frames, info, nullptr);
    // @todo - update `info` here
}

const SamplingInfo NullDeviceNode::getSamplingInfo() const
{
    return {}; // @todo
}

void NullDeviceNode::offlineRender()
{
    LOG("Starting Offline Rendering");

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
    size_t framesToProcess = static_cast<size_t>((m_lengthSeconds * m_context->sampleRate()) / offlineRenderSizeQuantum);

    // @todo - early exit if stop or reset called
    while (framesToProcess > 0)
    {
        SamplingInfo info; // @fixme
        render(0, m_renderBus.get(), offlineRenderSizeQuantum, info);
        framesToProcess--;
    }

    LOG("Stopping Offline Rendering");
}

} // namespace lab
