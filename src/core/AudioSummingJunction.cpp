// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioSummingJunction.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioContext.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"

#include <algorithm>
#include <iostream>

namespace lab 
{
    
lab::concurrent_queue<std::shared_ptr<AudioSummingJunction>> s_dirtySummingJunctions;

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

AudioSummingJunction::AudioSummingJunction() : m_renderingStateNeedUpdating(false)
{
    
}

AudioSummingJunction::~AudioSummingJunction()
{
    
}
    
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
    
void AudioSummingJunction::changedOutputs(ContextGraphLock&)
{
    if (!m_renderingStateNeedUpdating && canUpdateState())
    {
        m_renderingStateNeedUpdating = true;
    }
}
    
void AudioSummingJunction::updateRenderingState(ContextRenderLock& r)
{
    if (r.context() && m_renderingStateNeedUpdating && canUpdateState())
    {
        std::lock_guard<std::mutex> lock(junctionMutex);
        
        // Copy from m_outputs to m_renderingOutputs.
        m_renderingOutputs.clear();
        for (std::vector<std::weak_ptr<AudioNodeOutput>>::iterator i = m_connectedOutputs.begin(); i != m_connectedOutputs.end(); ++i)
            if (!i->expired())
            {
                m_renderingOutputs.push_back(*i);
                i->lock()->updateRenderingState(r);
            }

        didUpdate(r);
        m_renderingStateNeedUpdating = false;
    }
}

} // namespace lab
