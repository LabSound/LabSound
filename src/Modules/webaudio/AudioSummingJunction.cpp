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

#include "LabSoundConfig.h"
#include "AudioSummingJunction.h"

#include "AudioContext.h"
#include "AudioContextLock.h"
#include "AudioNodeOutput.h"
#include <algorithm>

using namespace std;

namespace WebCore {
    
LabSound::concurrent_queue<std::shared_ptr<AudioSummingJunction>> m_dirtySummingJunctions;

    namespace {
        mutex junctionMutex;
    }
    

void AudioSummingJunction::markSummingJunctionDirty(std::shared_ptr<AudioSummingJunction> summingJunction)
{
    if (summingJunction)
        m_dirtySummingJunctions.push(summingJunction);
}

void AudioSummingJunction::handleDirtyAudioSummingJunctions(ContextRenderLock& r)
{
    ASSERT(r.context());
    std::shared_ptr<AudioSummingJunction> asj;
    while (m_dirtySummingJunctions.try_pop(asj))
        asj->updateRenderingState(r);
}

AudioSummingJunction::AudioSummingJunction()
: m_renderingStateNeedUpdating(false)
{
}

AudioSummingJunction::~AudioSummingJunction()
{
}
    
size_t AudioSummingJunction::numberOfConnections() const {
    size_t count = 0;
    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i) {
        if (m_outputs[i])
            ++count;
    }
    return count;
}
    
size_t AudioSummingJunction::numberOfRenderingConnections() const {
    size_t count = 0;
    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i) {
        if (m_renderingOutputs[i])
            ++count;
    }
    return count;
}
    
void AudioSummingJunction::addOutput(std::shared_ptr<AudioNodeOutput> o) {
    if (!o)
        return;
    
    lock_guard<mutex> lock(junctionMutex);
    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i)
        if (m_outputs[i] == o)
            return;

    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i)
        if (!m_outputs[i]) {
            m_outputs[i] = o;
            m_renderingStateNeedUpdating = true;
            return;
        }

    ASSERT(0 == "Couldn't add output");
}

void AudioSummingJunction::removeOutput(std::shared_ptr<AudioNodeOutput> o) {
    if (!o)
        return;
    
    lock_guard<mutex> lock(junctionMutex);
    bool modified = false;
    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i)
        if (m_outputs[i] == o) {
            m_outputs[i].reset();
            modified = true;
            break;
        }
    
    if (modified)
        m_renderingStateNeedUpdating = true;
}
    
void AudioSummingJunction::updateRenderingState(ContextRenderLock& r)
{
    if (m_renderingStateNeedUpdating && canUpdateState()) {
        ASSERT(r.context());

        // Copy from m_outputs to m_renderingOutputs.
        for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i) {
            auto output = m_outputs[i];
            if (output) {
                m_renderingOutputs[i] = output;
                output->updateRenderingState(r);
            }
        }

        didUpdate(r);
        m_renderingStateNeedUpdating = false;
    }
}

} // namespace WebCore
