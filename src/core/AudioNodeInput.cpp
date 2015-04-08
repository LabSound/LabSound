/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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
#include "AudioNodeInput.h"

#include "AudioContext.h"
#include "AudioContextLock.h"
#include "AudioNode.h"
#include "AudioNodeOutput.h"
#include <algorithm>

using namespace std;
 
namespace WebCore {
    
    namespace {
        mutex outputsMutex;
    }

AudioNodeInput::AudioNodeInput(AudioNode* node)
: AudioSummingJunction()
, m_node(node)
{
    // Set to mono by default.
    m_internalSummingBus = std::unique_ptr<AudioBus>(new AudioBus(1, AudioNode::ProcessingSizeInFrames));
}

void AudioNodeInput::connect(ContextGraphLock& g,
                             std::shared_ptr<AudioNodeInput> fromInput, std::shared_ptr<AudioNodeOutput> toOutput)
{
    if (!fromInput || !toOutput || !fromInput->node())
        return;
    
    ASSERT(g.context());
    lock_guard<mutex> lock(outputsMutex);

    // return if input is already connected to this output.
    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i)
        if (fromInput->m_outputs[i] == toOutput)
            return;

    toOutput->addInput(fromInput);
    fromInput->addOutput(toOutput);
    
    // Inform context that a connection has been made.
    g.context()->incrementConnectionCount();
}

void AudioNodeInput::disconnect(ContextGraphLock& g,
                                std::shared_ptr<AudioNodeInput> fromInput, std::shared_ptr<AudioNodeOutput> toOutput)
{
    ASSERT(g.context());
    if (!fromInput || !toOutput || !fromInput->node())
        return;
    
    lock_guard<mutex> lock(outputsMutex);
    
    // First try to disconnect from "active" connections.
    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i) {
        if (fromInput->m_outputs[i] == toOutput) {
            fromInput->removeOutput(toOutput);
            toOutput->removeInput(fromInput);
            return;
        }
    }
    
    // Otherwise, try to disconnect from disabled connections.
    auto it2 = fromInput->m_disabledOutputs.find(toOutput);
    if (it2 != fromInput->m_disabledOutputs.end()) {
        fromInput->m_disabledOutputs.erase(it2);
        toOutput->removeInput(fromInput);
        return;
    }

    ASSERT_NOT_REACHED();
}

void AudioNodeInput::disable(ContextGraphLock& g, std::shared_ptr<AudioNodeOutput> output)
{
    if (!output || !node())
        return;

    lock_guard<mutex> lock(outputsMutex);

    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i) {
        if (m_outputs[i] == output) {
            m_disabledOutputs.insert(output);
            m_outputs[i].reset();
            m_renderingStateNeedUpdating = true;
            node()->disableOutputsIfNecessary(g);    // Propagate disabled state to outputs.
        }
    }
}

void AudioNodeInput::enable(ContextGraphLock& g, std::shared_ptr<AudioNodeOutput> output)
{
    if (!output || !node())
        return;

    lock_guard<mutex> lock(outputsMutex);
    
    auto it = m_disabledOutputs.find(output);
    ASSERT(it != m_disabledOutputs.end());

    // Move output from disabled list to active list.
    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i) {
        if (!m_outputs[i]) {
            m_outputs[i] = output;
            m_renderingStateNeedUpdating = true;
            
            // Propagate enabled state to outputs.
            node()->enableOutputsIfNecessary(g);
            break;
        }
    }
    m_disabledOutputs.erase(it);
}

void AudioNodeInput::didUpdate(ContextRenderLock& r)
{
    m_node->checkNumberOfChannelsForInput(r, this);
}

void AudioNodeInput::updateInternalBus(ContextRenderLock& r)
{
    unsigned numberOfInputChannels = numberOfChannels();

    if (numberOfInputChannels == m_internalSummingBus->numberOfChannels())
        return;

    lock_guard<mutex> lock(outputsMutex);
    m_internalSummingBus = std::unique_ptr<AudioBus>(new AudioBus(numberOfInputChannels, AudioNode::ProcessingSizeInFrames));
}

unsigned AudioNodeInput::numberOfChannels() const
{
    // Find the number of channels of the connection with the largest number of channels.
    unsigned maxChannels = 1; // one channel is the minimum allowed

    for (int i = 0; i < SUMMING_JUNCTION_MAX_OUTPUTS; ++i) {
        auto output = m_outputs[i];
        if (output)
            maxChannels = max(maxChannels, output->bus()->numberOfChannels());
    }
    
    return maxChannels;
}

unsigned AudioNodeInput::numberOfRenderingChannels()
{
    // Find the number of channels of the rendering connection with the largest number of channels.
    unsigned maxChannels = 1; // one channel is the minimum allowed

    for (unsigned i = 0; i < numberOfRenderingConnections(); ++i)
        maxChannels = max(maxChannels, renderingOutput(i)->bus()->numberOfChannels());
    
    return maxChannels;
}

AudioBus* AudioNodeInput::bus()
{
    // Handle single connection specially to allow for in-place processing.
    if (numberOfRenderingConnections() == 1)
        return renderingOutput(0)->bus();

    // Multiple connections case (or no connections).
    return internalSummingBus();
}

AudioBus* AudioNodeInput::internalSummingBus()
{
    ASSERT(numberOfRenderingChannels() == m_internalSummingBus->numberOfChannels());

    return m_internalSummingBus.get();
}

void AudioNodeInput::sumAllConnections(ContextRenderLock& r, AudioBus* summingBus, size_t framesToProcess)
{
    // We shouldn't be calling this method if there's only one connection, since it's less efficient.
    ASSERT(numberOfRenderingConnections() > 1);

    ASSERT(summingBus);
    if (!summingBus)
        return;
        
    summingBus->zero();

    for (unsigned i = 0; i < numberOfRenderingConnections(); ++i) {
        auto output = renderingOutput(i);
        ASSERT(output);

        // Render audio from this output.
        AudioBus* connectionBus = output->pull(r, 0, framesToProcess);

        // Sum, with unity-gain.
        summingBus->sumFrom(*connectionBus);
    }
}

AudioBus* AudioNodeInput::pull(ContextRenderLock& r, AudioBus* inPlaceBus, size_t framesToProcess)
{
    updateRenderingState(r);
    
    // Handle single connection case.
    if (numberOfRenderingConnections() == 1) {
        // The output will optimize processing using inPlaceBus if it's able.
        auto output = this->renderingOutput(0);
        return output->pull(r, inPlaceBus, framesToProcess);
    }

    AudioBus* internalSummingBus = this->internalSummingBus();

    if (!numberOfRenderingConnections()) {
        // At least, generate silence if we're not connected to anything.
        // FIXME: if we wanted to get fancy, we could propagate a 'silent hint' here to optimize the downstream graph processing.
        internalSummingBus->zero();
        return internalSummingBus;
    }
    
    // Handle multiple connections case.
    sumAllConnections(r, internalSummingBus, framesToProcess);
    
    return internalSummingBus;
}

} // namespace WebCore
