// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/OfflineAudioDestinationNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"

#include "internal/Assertions.h"

#include <algorithm>

using namespace std;
 
namespace lab {
    
const size_t renderQuantumSize = 128;    

OfflineAudioDestinationNode::OfflineAudioDestinationNode(AudioContext * context, const float sampleRate, const float lengthSeconds, const uint32_t numChannels) : AudioDestinationNode(context, sampleRate),
    m_numChannels(numChannels),
    m_lengthSeconds(lengthSeconds) 
{
    m_renderBus = std::unique_ptr<AudioBus>(new AudioBus(numChannels, renderQuantumSize));
}

OfflineAudioDestinationNode::~OfflineAudioDestinationNode()
{
    uninitialize();
}

void OfflineAudioDestinationNode::initialize()
{
    if (isInitialized())
        return;

    AudioNode::initialize();
}

void OfflineAudioDestinationNode::uninitialize()
{
    if (!isInitialized())
        return;

    if (m_renderThread.joinable())
    {
       m_renderThread.join();
    }

    AudioNode::uninitialize();
}

void OfflineAudioDestinationNode::startRendering()
{
    if (!m_startedRendering) 
    {
        m_startedRendering = true;

        m_renderThread = std::thread(&OfflineAudioDestinationNode::offlineRender, this);
        
        // @tofix - ability to update main thread from here. Currently blocks until complete
        if (m_renderThread.joinable())
            m_renderThread.join();

        if (m_context->offlineRenderCompleteCallback)
            m_context->offlineRenderCompleteCallback();

        m_startedRendering = false;
    }
    else
    {
        LOG("Offline rendering has already started");
    }
}

void OfflineAudioDestinationNode::offlineRender()
{
    LOG("Starting Offline Rendering");

    ASSERT(m_renderBus.get());
    if (!m_renderBus.get())
        return;

    bool isRenderBusAllocated = m_renderBus->length() >= renderQuantumSize;
    ASSERT(isRenderBusAllocated);
    if (!isRenderBusAllocated)
        return;
    
    bool isAudioContextInitialized = m_context->isInitialized();
    ASSERT(isAudioContextInitialized);
    if (!isAudioContextInitialized) 
        return;

    // Break up the render target into smaller "render quantize" sized pieces.
    size_t framesToProcess = (m_lengthSeconds * m_context->sampleRate()) / renderQuantumSize;

    while (framesToProcess > 0)
    {
        render(0, m_renderBus.get(), renderQuantumSize);
        framesToProcess -= 1;
    }

    LOG("Stopping Offline Rendering");
}

} // namespace lab
