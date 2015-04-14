/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
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

#include "LabSound/core/AudioSummingJunction.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioContext.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"

#include <algorithm>
#include <iostream>

namespace WebCore 
{
    
LabSound::concurrent_queue<std::shared_ptr<AudioSummingJunction>> s_dirtySummingJunctions;

    namespace 
	{
        std::mutex junctionMutex;
    }
    

void AudioSummingJunction::handleDirtyAudioSummingJunctions(ContextRenderLock& r)
{
    ASSERT(r.context());
    std::shared_ptr<AudioSummingJunction> asj;
    while (s_dirtySummingJunctions.try_pop(asj))
        asj->updateRenderingState(r);
}

AudioSummingJunction::AudioSummingJunction()
: m_renderingStateNeedUpdating(false) {}

AudioSummingJunction::~AudioSummingJunction() {}
    
bool AudioSummingJunction::isConnected(std::shared_ptr<AudioNodeOutput> o) const
{
    std::lock_guard<std::mutex> lock(junctionMutex);
    
    for (auto i : m_connectedOutputs)
        if (i.lock() == o)
            return true;

    return false;
}

size_t AudioSummingJunction::numberOfRenderingConnections(ContextRenderLock&) const {
    size_t count = 0;
    for (auto i : m_renderingOutputs) {
        if (!i.expired())
            ++count;
    }
    return count;
}
    
void AudioSummingJunction::junctionConnectOutput(std::shared_ptr<AudioNodeOutput> o)
{
    if (!o)
        return;
    
    std::lock_guard<std::mutex> lock(junctionMutex);

    for (std::vector<std::weak_ptr<AudioNodeOutput>>::iterator i = m_connectedOutputs.begin(); i != m_connectedOutputs.end(); ++i)
        if (i->expired())
            i = m_connectedOutputs.erase(i);
    
    for (auto i : m_connectedOutputs)
        if (i.lock() == o)
            return;

    m_connectedOutputs.push_back(o);
    m_renderingStateNeedUpdating = true;
}

void AudioSummingJunction::junctionDisconnectOutput(std::shared_ptr<AudioNodeOutput> o)
{
    if (!o)
        return;
    
    std::lock_guard<std::mutex> lock(junctionMutex);

    for (std::vector<std::weak_ptr<AudioNodeOutput>>::iterator i = m_connectedOutputs.begin(); i != m_connectedOutputs.end(); ++i)
        if (!i->expired() && i->lock() == o) {
            m_connectedOutputs.erase(i);
            m_renderingStateNeedUpdating = true;
            break;
        }
}
    
void AudioSummingJunction::updateRenderingState(ContextRenderLock& r)
{
    if (r.context() && m_renderingStateNeedUpdating && canUpdateState()) {
        std::lock_guard<std::mutex> lock(junctionMutex);
        
        // Copy from m_outputs to m_renderingOutputs.
        m_renderingOutputs.clear();
        for (std::vector<std::weak_ptr<AudioNodeOutput>>::iterator i = m_connectedOutputs.begin(); i != m_connectedOutputs.end(); ++i)
            if (!i->expired()) {
                m_renderingOutputs.push_back(*i);
                i->lock()->updateRenderingState(r);
            }
    }
    
    didUpdate(r);
    m_renderingStateNeedUpdating = false;
}

} // namespace WebCore
