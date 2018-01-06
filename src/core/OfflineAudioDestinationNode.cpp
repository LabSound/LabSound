// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/OfflineAudioDestinationNode.h"
#include "LabSound/core/AudioContext.h"

#include "LabSound/extended/Logging.h"

#include "internal/Assertions.h"
#include "internal/AudioBus.h"

#include <algorithm>

using namespace std;
 
namespace lab {
    
const size_t renderQuantumSize = 256;    

OfflineAudioDestinationNode::OfflineAudioDestinationNode(AudioContext * context, const float lengthSeconds, const uint32_t numChannels, const float sampleRate)
    : AudioDestinationNode(context, sampleRate),
    m_sampleRate(sampleRate),
    m_numChannels(numChannels),
    m_lengthSeconds(lengthSeconds),
    ctx(context)
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
        
        LOG("Starting Offline Rendering");
        
        m_renderThread = std::thread(&OfflineAudioDestinationNode::offlineRender, this);
        
        // @tofix - ability to update main thread from here. Currently blocks until complete
        if (m_renderThread.joinable())
            m_renderThread.join();
        
        LOG("Stopping Offline Rendering");
        
        if (m_context->offlineRenderCompleteCallback)
            m_context->offlineRenderCompleteCallback();
    }
}

void OfflineAudioDestinationNode::offlineRender()
{
    ASSERT(m_renderBus.get());
    if (!m_renderBus.get())
        return;

    bool isRenderBusAllocated = m_renderBus->length() >= renderQuantumSize;
    ASSERT(isRenderBusAllocated);
    if (!isRenderBusAllocated)
        return;
    
    bool isAudioContextInitialized = ctx->isInitialized();
    ASSERT(isAudioContextInitialized);
    if (!isAudioContextInitialized) return;

    // Break up the render target into smaller "render quantize" sized pieces.
    size_t framesToProcess = (m_lengthSeconds * m_sampleRate) / renderQuantumSize;

    unsigned n = 0;
    while (framesToProcess > 0)
    {
        render(0, m_renderBus.get(), renderQuantumSize);
        size_t framesAvailableToCopy = min(framesToProcess, renderQuantumSize);
        n += framesAvailableToCopy;
        framesToProcess -= framesAvailableToCopy;
    }
}

} // namespace lab
