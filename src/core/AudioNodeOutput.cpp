// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioParam.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/AudioBus.h"

#include <mutex>

using namespace std;

namespace lab 
{
    
namespace 
{
   std::mutex outputMutex;
}

AudioNodeOutput::AudioNodeOutput(AudioNode* node, unsigned numberOfChannels)
    : m_node(node)
    , m_numberOfChannels(numberOfChannels)
    , m_desiredNumberOfChannels(numberOfChannels)
    , m_isInPlace(false)
    , m_renderingFanOutCount(0)
    , m_renderingParamFanOutCount(0)
    , m_inPlaceBus(0)
{
    ASSERT(numberOfChannels <= AudioContext::maxNumberOfChannels);
    
    m_internalBus.reset(new AudioBus(numberOfChannels, AudioNode::ProcessingSizeInFrames));
}

AudioNodeOutput::~AudioNodeOutput()
{

}

void AudioNodeOutput::setNumberOfChannels(ContextRenderLock& r, unsigned numberOfChannels)
{
    ASSERT(r.context());
    ASSERT(numberOfChannels <= AudioContext::maxNumberOfChannels);
    
    if (m_numberOfChannels == numberOfChannels)
        return;
    
    m_desiredNumberOfChannels = numberOfChannels;
    m_internalBus.reset(new AudioBus(numberOfChannels, AudioNode::ProcessingSizeInFrames));
}

void AudioNodeOutput::updateInternalBus()
{
    if (numberOfChannels() == m_internalBus->numberOfChannels())
        return;

    m_internalBus.reset(new AudioBus(numberOfChannels(), AudioNode::ProcessingSizeInFrames));
}

void AudioNodeOutput::updateRenderingState(ContextRenderLock& r)
{
    if (m_numberOfChannels != m_desiredNumberOfChannels)
    {
        ASSERT(r.context());
        m_numberOfChannels = m_desiredNumberOfChannels;
        updateInternalBus();
        propagateChannelCount(r);
    }
    m_renderingFanOutCount = fanOutCount();
    m_renderingParamFanOutCount = paramFanOutCount();
}

void AudioNodeOutput::propagateChannelCount(ContextRenderLock& r)
{
    if (isChannelCountKnown())
    {
        ASSERT(r.context());
        
        // Announce to any nodes we're connected to that we changed our channel count for its input.
        for (auto in : m_inputs)
        {
            auto connectionNode = in->node();
            connectionNode->checkNumberOfChannelsForInput(r, in.get());
        }
    }
}

AudioBus* AudioNodeOutput::pull(ContextRenderLock& r, AudioBus* inPlaceBus, size_t framesToProcess)
{
    ASSERT(r.context());
    ASSERT(m_renderingFanOutCount > 0 || m_renderingParamFanOutCount > 0);
    
    // Causes our AudioNode to process if it hasn't already for this render quantum.
    // We try to do in-place processing (using inPlaceBus) if at all possible,
    // but we can't process in-place if we're connected to more than one input (fan-out > 1).
    // In this case pull() is called multiple times per rendering quantum, and the processIfNecessary() call below will
    // cause our node to process() only the first time, caching the output in m_internalOutputBus for subsequent calls.    

    updateRenderingState(r);
    
    m_isInPlace = inPlaceBus && inPlaceBus->numberOfChannels() == numberOfChannels() && (m_renderingFanOutCount + m_renderingParamFanOutCount) == 1;
    
    // Setup the actual destination bus for processing when our node's process() method gets called in processIfNecessary() below.
    m_inPlaceBus = m_isInPlace ? inPlaceBus : 0;
    
    node()->processIfNecessary(r, framesToProcess);
    return bus(r);
}

AudioBus* AudioNodeOutput::bus(ContextRenderLock& r) const
{
    ASSERT(r.context()); // only legal during rendering
    return m_isInPlace ? m_inPlaceBus : m_internalBus.get();
}

unsigned AudioNodeOutput::fanOutCount()
{
    return m_inputs.size();
}

unsigned AudioNodeOutput::paramFanOutCount()
{
    return m_params.size();
}

unsigned AudioNodeOutput::renderingFanOutCount() const
{
    return m_renderingFanOutCount;
}

unsigned AudioNodeOutput::renderingParamFanOutCount() const
{
    return m_renderingParamFanOutCount;
}

void AudioNodeOutput::addInput(ContextGraphLock& g, std::shared_ptr<AudioNodeInput> input)
{
    if (!input)
        return;
    
    m_inputs.emplace_back(input);
    input->setDirty();
}

void AudioNodeOutput::removeInput(ContextGraphLock& g, std::shared_ptr<AudioNodeInput> input)
{
    if (!input)
        return;
    
    for (std::vector<std::shared_ptr<AudioNodeInput>>::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
        if (input == *i) {
            input->setDirty();
            i = m_inputs.erase(i);
            if (i == m_inputs.end())
                break;
        }
    }
}

void AudioNodeOutput::disconnectAllInputs(ContextGraphLock& g, std::shared_ptr<AudioNodeOutput> self)
{
    ASSERT(g.context());
    
    // AudioNodeInput::disconnect() changes m_inputs by calling removeInput().
    while (self->m_inputs.size()) 
    {
        auto ptr = self->m_inputs.rbegin();
        AudioNodeInput::disconnect(g, *ptr, self);
    }
}

void AudioNodeOutput::addParam(ContextGraphLock& g, std::shared_ptr<AudioParam> param)
{
    if (!param)
        return;

    m_params.insert(param);
}

void AudioNodeOutput::removeParam(ContextGraphLock& g, std::shared_ptr<AudioParam> param)
{
    if (!param)
        return;

    auto it = m_params.find(param);
    if (it != m_params.end())
        m_params.erase(it);
}

void AudioNodeOutput::disconnectAllParams(ContextGraphLock& g, std::shared_ptr<AudioNodeOutput> self)
{
    // AudioParam::disconnect() changes m_params by calling removeParam().
    while (self->m_params.size()) {
        auto param = self->m_params.begin();
        (*param)->disconnect(g, *param, self);
    }
}

void AudioNodeOutput::disconnectAll(ContextGraphLock& g, std::shared_ptr<AudioNodeOutput> self)
{
    self->disconnectAllInputs(g, self);
    self->disconnectAllParams(g, self);
}

} // namespace lab
