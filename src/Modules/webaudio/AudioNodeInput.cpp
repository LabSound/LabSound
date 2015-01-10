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
#include "AudioNode.h"
#include "AudioNodeOutput.h"
#include <algorithm>

using namespace std;
 
namespace WebCore {

AudioNodeInput::AudioNodeInput(AudioNode* node)
: AudioSummingJunction(node->context().lock())
, m_node(node)
{
    // Set to mono by default.
    m_internalSummingBus = std::unique_ptr<AudioBus>(new AudioBus(1, AudioNode::ProcessingSizeInFrames));
}

void AudioNodeInput::connect(std::shared_ptr<AudioNodeInput> fromInput, std::shared_ptr<AudioNodeOutput> toOutput)
{
    ASSERT(!fromInput->context().expired());
    auto ac = fromInput->context().lock();
    ASSERT(ac->isGraphOwner());
    
    if (!fromInput || !toOutput || !fromInput->node())
        return;

    // Check if input is already connected to this output.
    if (fromInput->m_outputs.find(toOutput) != fromInput->m_outputs.end())
        return;

    toOutput->addInput(fromInput);
    fromInput->m_outputs.insert(toOutput);          /// @dp m_outputs shouldn't be visible. An accessor would allow elimination of changedOutputs
    AudioNodeInput::changedOutputs(fromInput);

    // Sombody has just connected to us, so count it as a reference.
    fromInput->node()->ref(AudioNode::RefTypeConnection);
}

void AudioNodeInput::disconnect(std::shared_ptr<AudioNodeInput> fromInput, std::shared_ptr<AudioNodeOutput> toOutput)
{
    ASSERT(!fromInput->context().expired());
    auto ac = fromInput->context().lock();
    ASSERT(ac->isGraphOwner());
    
    if (!fromInput || !toOutput || !fromInput->node())
        return;
    
    // First try to disconnect from "active" connections.
    auto it = fromInput->m_outputs.find(toOutput);
    if (it != fromInput->m_outputs.end()) {
        fromInput->m_outputs.erase(it);             /// @dp m_outputs shouldn't be visible. An accessor would allow elimination of changedOutputs
        AudioNodeInput::changedOutputs(fromInput);
        toOutput->removeInput(fromInput);
        fromInput->node()->deref(AudioNode::RefTypeConnection);
        return;
    }
    
    // Otherwise, try to disconnect from disabled connections.
    auto it2 = fromInput->m_disabledOutputs.find(toOutput);
    if (it2 != fromInput->m_disabledOutputs.end()) {
        fromInput->m_disabledOutputs.erase(it2);
        toOutput->removeInput(fromInput);
        fromInput->node()->deref(AudioNode::RefTypeConnection);
        return;
    }

    ASSERT_NOT_REACHED();
}

void AudioNodeInput::disable(std::shared_ptr<AudioNodeInput> self, std::shared_ptr<AudioNodeOutput> output)
{
    ASSERT(!self->context().expired());
    auto ac = self->context().lock();
    ASSERT(ac->isGraphOwner());

    ASSERT(output && self->node());
    if (!output || !self->node())
        return;

    auto it = self->m_outputs.find(output);
    ASSERT(it != self->m_outputs.end());
    
    self->m_disabledOutputs.insert(output);
    self->m_outputs.erase(it);
    changedOutputs(self);

    // Propagate disabled state to outputs.
    self->node()->disableOutputsIfNecessary();
}

void AudioNodeInput::enable(std::shared_ptr<AudioNodeInput> self, std::shared_ptr<AudioNodeOutput> output)
{
    if (!output || !self->node())
        return;
    
    ASSERT(!self->context().expired());
    auto ac = self->context().lock();
    ASSERT(ac->isGraphOwner());

    auto it = self->m_disabledOutputs.find(output);
    ASSERT(it != self->m_disabledOutputs.end());

    // Move output from disabled list to active list.
    self->m_outputs.insert(output);
    self->m_disabledOutputs.erase(it);
    AudioNodeInput::changedOutputs(self);

    // Propagate enabled state to outputs.
    self->node()->enableOutputsIfNecessary();
}

void AudioNodeInput::didUpdate()
{
    node()->checkNumberOfChannelsForInput(this);
}

void AudioNodeInput::updateInternalBus()
{
    ASSERT(!context().expired());
    auto ac = context().lock();
    ASSERT(ac->isAudioThread() && ac->isGraphOwner());

    unsigned numberOfInputChannels = numberOfChannels();

    if (numberOfInputChannels == m_internalSummingBus->numberOfChannels())
        return;

    m_internalSummingBus = std::unique_ptr<AudioBus>(new AudioBus(numberOfInputChannels, AudioNode::ProcessingSizeInFrames));
}

unsigned AudioNodeInput::numberOfChannels() const
{
    // Find the number of channels of the connection with the largest number of channels.
    unsigned maxChannels = 1; // one channel is the minimum allowed

    for (auto i : m_outputs) {
        maxChannels = max(maxChannels, i->bus()->numberOfChannels());
    }
    
    return maxChannels;
}

unsigned AudioNodeInput::numberOfRenderingChannels()
{
    ASSERT(!context().expired());
    auto ac = context().lock();
    ASSERT(ac->isAudioThread());

    // Find the number of channels of the rendering connection with the largest number of channels.
    unsigned maxChannels = 1; // one channel is the minimum allowed

    for (unsigned i = 0; i < numberOfRenderingConnections(); ++i)
        maxChannels = max(maxChannels, renderingOutput(i)->bus()->numberOfChannels());
    
    return maxChannels;
}

AudioBus* AudioNodeInput::bus()
{
    ASSERT(!context().expired());
    auto ac = context().lock();
    ASSERT(ac->isAudioThread());

    // Handle single connection specially to allow for in-place processing.
    if (numberOfRenderingConnections() == 1)
        return renderingOutput(0)->bus();

    // Multiple connections case (or no connections).
    return internalSummingBus();
}

AudioBus* AudioNodeInput::internalSummingBus()
{
    ASSERT(!context().expired());
    auto ac = context().lock();
    ASSERT(ac->isAudioThread());

    ASSERT(numberOfRenderingChannels() == m_internalSummingBus->numberOfChannels());

    return m_internalSummingBus.get();
}

void AudioNodeInput::sumAllConnections(AudioBus* summingBus, size_t framesToProcess)
{
    ASSERT(!context().expired());
    auto ac = context().lock();
    ASSERT(ac->isAudioThread());

    // We shouldn't be calling this method if there's only one connection, since it's less efficient.
    ASSERT(numberOfRenderingConnections() > 1);

    ASSERT(summingBus);
    if (!summingBus)
        return;
        
    summingBus->zero();

    for (unsigned i = 0; i < numberOfRenderingConnections(); ++i) {
        AudioNodeOutput* output = renderingOutput(i);
        ASSERT(output);

        // Render audio from this output.
        AudioBus* connectionBus = output->pull(0, framesToProcess);

        // Sum, with unity-gain.
        summingBus->sumFrom(*connectionBus);
    }
}

AudioBus* AudioNodeInput::pull(AudioBus* inPlaceBus, size_t framesToProcess)
{
    ASSERT(!context().expired());
    auto ac = context().lock();
    ASSERT(ac->isAudioThread());

    // Handle single connection case.
    if (numberOfRenderingConnections() == 1) {
        // The output will optimize processing using inPlaceBus if it's able.
        AudioNodeOutput* output = this->renderingOutput(0);
        return output->pull(inPlaceBus, framesToProcess);
    }

    AudioBus* internalSummingBus = this->internalSummingBus();

    if (!numberOfRenderingConnections()) {
        // At least, generate silence if we're not connected to anything.
        // FIXME: if we wanted to get fancy, we could propagate a 'silent hint' here to optimize the downstream graph processing.
        internalSummingBus->zero();
        return internalSummingBus;
    }

    // Handle multiple connections case.
    sumAllConnections(internalSummingBus, framesToProcess);
    
    return internalSummingBus;
}

} // namespace WebCore
