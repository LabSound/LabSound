/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSound/core/OfflineAudioDestinationNode.h"
#include "LabSound/core/AudioContext.h"

#include "LabSound/extended/Logging.h"

#include "internal/Assertions.h"
#include "internal/AudioBus.h"
#include "internal/HRTFDatabaseLoader.h"

#include <algorithm>

using namespace std;
 
namespace WebCore {
    
const size_t renderQuantumSize = 128;    

OfflineAudioDestinationNode::OfflineAudioDestinationNode(std::shared_ptr<AudioContext> context, AudioBuffer* renderTarget)
    : AudioDestinationNode(context, renderTarget->sampleRate())
    , m_renderTarget(renderTarget)
    , m_startedRendering(false)
{
    m_renderBus = std::unique_ptr<AudioBus>(new AudioBus(renderTarget->numberOfChannels(), renderQuantumSize));
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
    ASSERT(m_renderTarget);
    
    if (!m_renderTarget)
        return;
    
    if (!m_startedRendering) 
	{
        m_startedRendering = true;
        
        LOG("Starting Offline Rendering");
        
        m_renderThread = std::thread(&OfflineAudioDestinationNode::offlineRender, this);
        
        // @tofix: ability to update main thread from here. Currently blocks until complete
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
    
    bool channelsMatch = m_renderBus->numberOfChannels() == m_renderTarget->numberOfChannels();
    ASSERT(channelsMatch);
    if (!channelsMatch)
        return;
        
    bool isRenderBusAllocated = m_renderBus->length() >= renderQuantumSize;
    ASSERT(isRenderBusAllocated);
    if (!isRenderBusAllocated)
        return;
        
    // Synchronize with HRTFDatabaseLoader.
    // The database must be loaded before we can proceed.
    auto loader = HRTFDatabaseLoader::loader();
    ASSERT(loader);
    if (!loader)
        return;
    
    loader->waitForLoaderThreadCompletion();
    
    // Break up the render target into smaller "render quantize" sized pieces.
    size_t framesToProcess = m_renderTarget->length();
    unsigned numberOfChannels = m_renderTarget->numberOfChannels();

    unsigned n = 0;
    while (framesToProcess > 0)
    {
        // Render one quantum (nominally 128 frames)
        render(0, m_renderBus.get(), renderQuantumSize);
        
        size_t framesAvailableToCopy = min(framesToProcess, renderQuantumSize);
        
        for (unsigned channelIndex = 0; channelIndex < numberOfChannels; ++channelIndex)
        {
            const float * source = m_renderBus->channel(channelIndex)->data();
            float * destination = m_renderTarget->getChannelData(channelIndex)->data();
            memcpy(destination + n, source, sizeof(float) * framesAvailableToCopy);
        }
        
        n += framesAvailableToCopy;
        framesToProcess -= framesAvailableToCopy;
    }
}

} // namespace WebCore
