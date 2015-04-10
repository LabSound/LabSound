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
    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i)
        if (m_connectedOutputs[i].lock() == o)
            return true;

    return false;
}

size_t AudioSummingJunction::numberOfRenderingConnections() const {
    size_t count = 0;
    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i) {
        if (!m_renderingOutputs[i].expired())
            ++count;
    }
    return count;
}
    
void AudioSummingJunction::junctionConnectOutput(std::shared_ptr<AudioNodeOutput> o)
{
    if (!o)
        return;
    
    std::lock_guard<std::mutex> lock(junctionMutex);

    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i)
        if (m_connectedOutputs[i].lock() == o)
            return;

    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i)
        if (m_connectedOutputs[i].expired()) {
            m_connectedOutputs[i] = o;
            m_renderingStateNeedUpdating = true;
            return;
        }

    std::cerr << "Summing junction couldn't add output" << std::endl;
}

void AudioSummingJunction::junctionDisconnectOutput(std::shared_ptr<AudioNodeOutput> o)
{
    if (!o)
        return;
    
    std::lock_guard<std::mutex> lock(junctionMutex);

    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i)
        if (m_connectedOutputs[i].lock() == o) {
            m_connectedOutputs[i].reset();
            m_renderingStateNeedUpdating = true;
            break;
        }
}
    
void AudioSummingJunction::updateRenderingState(ContextRenderLock& r)
{
    if (r.context() && m_renderingStateNeedUpdating && canUpdateState()) {

        // Copy from m_outputs to m_renderingOutputs.
        for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i) {
            auto output = m_connectedOutputs[i];
            if (!output.expired()) {
                m_renderingOutputs[i] = output;
                output.lock()->updateRenderingState(r);
            }
            else {
                m_renderingOutputs[i].reset();
            }
        }

        didUpdate(r);
        m_renderingStateNeedUpdating = false;
    }
}

} // namespace WebCore
