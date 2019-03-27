// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"

using namespace std;

namespace lab {

AudioNode::AudioNode() = default;
AudioNode::~AudioNode() = default;

void AudioNode::initialize()
{
    m_isInitialized = true;
}

void AudioNode::uninitialize()
{
    m_isInitialized = false;
}

void AudioNode::addInput(std::unique_ptr<AudioNodeInput> input)
{
    m_inputs.emplace_back(std::move(input));
}

void AudioNode::addOutput(std::unique_ptr<AudioNodeOutput> output)
{
    m_outputs.emplace_back(std::move(output));
}

// safe without a Render lock because vector is immutable
std::shared_ptr<AudioNodeInput> AudioNode::input(size_t i)
{
    if (i < m_inputs.size()) return m_inputs[i];
    return nullptr;
}

// safe without a Render lock because vector is immutable
std::shared_ptr<AudioNodeOutput> AudioNode::output(size_t i)
{
    if (i < m_outputs.size()) return m_outputs[i];
    return nullptr;
}

size_t AudioNode::channelCount()
{
    return m_channelCount;
}

void AudioNode::setChannelCount(ContextGraphLock & g, size_t channelCount)
{
    if (!g.context())
    {
        throw std::invalid_argument("No context specified");
    }

    if (channelCount <= AudioContext::maxNumberOfChannels)
    {
        if (m_channelCount != channelCount)
        {
            m_channelCount = channelCount;
            if (m_channelCountMode != ChannelCountMode::Max)
                updateChannelsForInputs(g);
        }
        return;
    }

    throw std::logic_error("Should not be reached");
}

void AudioNode::setChannelCountMode(ContextGraphLock& g, ChannelCountMode mode)
{
    if (mode >= ChannelCountMode::End || !g.context())
    {
        throw std::invalid_argument("No context specified");
    }
    else
    {
        if (m_channelCountMode != mode)
        {
            m_channelCountMode = mode;
            updateChannelsForInputs(g);
        }
    }
}

void AudioNode::updateChannelsForInputs(ContextGraphLock& g)
{
    for (auto input : m_inputs)
    {
        input->changedOutputs(g);
    }
}

void AudioNode::processIfNecessary(ContextRenderLock & r, size_t framesToProcess)
{
    if (!isInitialized()) return;

    auto ac = r.context();

    if (!ac) return;

    // Ensure that we only process once per rendering quantum.
    // This handles the "fanout" problem where an output is connected to multiple inputs.
    // The first time we're called during this time slice we process, but after that we don't want to re-process,
    // instead our output(s) will already have the results cached in their bus;
    double currentTime = ac->currentTime();
    if (m_lastProcessingTime != currentTime)
    {
        m_lastProcessingTime = currentTime; // important to first update this time to accomodate feedback loops in the rendering graph

        pullInputs(r, framesToProcess);

        bool silentInputs = inputsAreSilent(r);
        if (!silentInputs)
        {
            m_lastNonSilentTime = (ac->currentSampleFrame() + framesToProcess) / static_cast<double>(ac->sampleRate());
        }

        // if this node is supposed to copy silence through, and is itself silent
        if (silentInputs && propagatesSilence(r))
        {
            silenceOutputs(r);
        }
        else
        {
            process(r, framesToProcess);

            float new_schedule = 0.f;

            if (m_disconnectSchedule >= 0)
            {
                for (auto out : m_outputs)
                    for (unsigned i = 0; i < out->bus(r)->numberOfChannels(); ++i)
                    {
                        float scale = m_disconnectSchedule;
                        float * sample = out->bus(r)->channel(i)->mutableData();
                        size_t numSamples = out->bus(r)->channel(i)->length();
                        for (size_t s = 0; s < numSamples; ++s)
                        {
                            sample[s] *= scale;
                            scale *= 0.98f;
                        }
                        new_schedule = scale;
                    }

                m_disconnectSchedule = new_schedule;
            }

            new_schedule = 1.f;
            if (m_connectSchedule < 1)
            {
                for (auto out : m_outputs)
                    for (unsigned i = 0; i < out->bus(r)->numberOfChannels(); ++i)
                    {
                        float scale = m_connectSchedule;
                        float * sample = out->bus(r)->channel(i)->mutableData();
                        size_t numSamples = out->bus(r)->channel(i)->length();
                        for (size_t s = 0; s < numSamples; ++s)
                        {
                            sample[s] *= scale;
                            scale = 1.f - ((1.f - scale) * 0.98f);
                        }
                        new_schedule = scale;
                    }

                m_connectSchedule = new_schedule;
            }

            unsilenceOutputs(r);
        }
    }
}

void AudioNode::checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input)
{
    ASSERT(r.context());
    for (auto & in : m_inputs)
    {
        if (in.get() == input)
        {
            input->updateInternalBus(r);
            break;
        }
    }
}

bool AudioNode::propagatesSilence(ContextRenderLock & r) const
{
    ASSERT(r.context());

    return m_lastNonSilentTime + latencyTime(r) + tailTime(r) < r.context()->currentTime(); // dimitri use of latencyTime() / tailTime()
}

void AudioNode::pullInputs(ContextRenderLock& r, size_t framesToProcess)
{
    ASSERT(r.context());

    // Process all of the AudioNodes connected to our inputs.
    for (auto & in : m_inputs)
    {
        in->pull(r, 0, framesToProcess);
    }
}

bool AudioNode::inputsAreSilent(ContextRenderLock& r)
{
    for (auto & in : m_inputs)
    {
        if (!in->bus(r)->isSilent())
        {
            return false;
        }
    }
    return true;
}

void AudioNode::silenceOutputs(ContextRenderLock& r)
{
    for (auto out : m_outputs)
    {
        out->bus(r)->zero();
    }
}

void AudioNode::unsilenceOutputs(ContextRenderLock& r)
{
    for (auto out : m_outputs)
    {
        out->bus(r)->clearSilentFlag();
    }
}

std::shared_ptr<AudioParam> AudioNode::getParam(char const * const str)
{
    for (auto & p : m_params)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

std::shared_ptr<AudioSetting> AudioNode::getSetting(char const * const str)
{
    for (auto & p : m_settings)
    {
        if (!strcmp(str, p->name().c_str()))
            return p;
    }
    return {};
}

std::vector<std::string> AudioNode::params() const
{
    std::vector<std::string> ret;   
    for (auto & p : m_params)
    {
        ret.push_back(p->name());
    }
    return ret;
}

std::vector<std::string> AudioNode::settings() const
{
    std::vector<std::string> ret;   
    for (auto & p : m_settings)
    {
        ret.push_back(p->name());
    }
    return ret;
}



} // namespace lab
