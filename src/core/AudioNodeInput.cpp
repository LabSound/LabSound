// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioBus.h"
#include "internal/Assertions.h"

#include <algorithm>
#include <mutex>

using namespace std;
 
namespace lab 
{

AudioNodeInput::AudioNodeInput(AudioNode* node) : AudioSummingJunction(), m_node(node)
{
    // Set to mono by default.
    m_internalSummingBus = std::unique_ptr<AudioBus>(new AudioBus(1, AudioNode::ProcessingSizeInFrames));
}

AudioNodeInput::~AudioNodeInput()
{

}

void AudioNodeInput::connect(ContextGraphLock& g, std::shared_ptr<AudioNodeInput> junction, std::shared_ptr<AudioNodeOutput> toOutput)
{
    if (!junction || !toOutput || !junction->node())
        return;
    
    // return if input is already connected to this output.
    if (junction->isConnected(toOutput))
        return;

    toOutput->addInput(g, junction);
    junction->junctionConnectOutput(toOutput);
    
    // Inform context that a connection has been made.
    ASSERT(g.context());
    g.context()->incrementConnectionCount();
}

void AudioNodeInput::disconnect(ContextGraphLock& g, std::shared_ptr<AudioNodeInput> junction, std::shared_ptr<AudioNodeOutput> toOutput)
{
    ASSERT(g.context());
    if (!junction || !junction->node() || !toOutput)
        return;
    
    if (junction->isConnected(toOutput)) {
        junction->junctionDisconnectOutput(toOutput);
        toOutput->removeInput(g, junction);
    }
}

void AudioNodeInput::didUpdate(ContextRenderLock& r)
{
    m_node->checkNumberOfChannelsForInput(r, this);
}

void AudioNodeInput::updateInternalBus(ContextRenderLock& r)
{
    unsigned numberOfInputChannels = numberOfChannels(r);

    if (numberOfInputChannels == m_internalSummingBus->numberOfChannels())
        return;

    m_internalSummingBus = std::unique_ptr<AudioBus>(new AudioBus(numberOfInputChannels, AudioNode::ProcessingSizeInFrames));
}

unsigned AudioNodeInput::numberOfChannels(ContextRenderLock& r) const
{
    ChannelCountMode mode = node()->channelCountMode();
    
    if (mode == ChannelCountMode::Explicit)
    {
        return node()->channelCount();
    }
   
    // Find the number of channels of the connection with the largest number of channels.
    unsigned maxChannels = 1; // one channel is the minimum allowed

    int c = numberOfRenderingConnections(r);
    for (int i = 0; i < c; ++i)
    {
        auto output = renderingOutput(r, i);
        if (output)
        {
            maxChannels = max(maxChannels, output->bus(r)->numberOfChannels());
        }
    }

    if (mode == ChannelCountMode::ClampedMax)
    {
        maxChannels = min(maxChannels, static_cast<unsigned>(node()->channelCount()));
    }
    
    return maxChannels;
}

AudioBus* AudioNodeInput::bus(ContextRenderLock& r)
{
    // Handle single connection specially to allow for in-place processing.
    // note: The webkit sources check for max, but I can't see how that's correct
    
    // @tofix - did I miss part of the merge?
    if (numberOfRenderingConnections(r) == 1) // && node()->channelCountMode() == ChannelCountMode::Max)
    {
        return renderingOutput(r, 0)->bus(r);
    }

    // Multiple connections case (or no connections).
    return internalSummingBus(r);
}

AudioBus* AudioNodeInput::internalSummingBus(ContextRenderLock& r)
{
    return m_internalSummingBus.get();
}

void AudioNodeInput::sumAllConnections(ContextRenderLock& r, AudioBus* summingBus, size_t framesToProcess)
{
    // We shouldn't be calling this method if there's only one connection, since it's less efficient.
    int c = numberOfRenderingConnections(r);
    ASSERT(c > 1);

    ASSERT(summingBus);
    if (!summingBus)
        return;
        
    summingBus->zero();

    for (int i = 0; i < c; ++i)
    {
        auto output = renderingOutput(r, i);
        if (output)
        {
            // Render audio from this output.
            AudioBus* connectionBus = output->pull(r, 0, framesToProcess);
            
            // Sum, with unity-gain.
            summingBus->sumFrom(*connectionBus);
        }
    }
}

AudioBus* AudioNodeInput::pull(ContextRenderLock& r, AudioBus* inPlaceBus, size_t framesToProcess)
{
    
    updateRenderingState(r);
    
    int c = numberOfRenderingConnections(r);
    
    // Handle single connection case.
    if (c == 1)
    {
        // The output will optimize processing using inPlaceBus if it's able.
        auto output = renderingOutput(r, 0);
        if (output)
        {
             return output->pull(r, inPlaceBus, framesToProcess);
        }
        c = 0; // invoke the silence case
    }

    AudioBus* internalSummingBus = this->internalSummingBus(r);

    if (c == 0)
    {
        // At least, generate silence if we're not connected to anything.
        // FIXME: if we wanted to get fancy, we could propagate a 'silent hint' here to optimize the downstream graph processing.
        internalSummingBus->zero();
        return internalSummingBus;
    }
    
    // Handle multiple connections case
	sumAllConnections(r, internalSummingBus, framesToProcess);
    
    return internalSummingBus;
}

} // namespace lab
