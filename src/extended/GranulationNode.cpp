// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/GranulationNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Util.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"

using namespace lab;

GranulationNode::GranulationNode() : AudioScheduledSourceNode(), m_sourceBus(std::make_shared<AudioSetting>("sourceBus", "SBUS", AudioSetting::Type::Bus))
{
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));
    initialize();
}

GranulationNode::~GranulationNode()
{
    uninitialize();
}

void GranulationNode::reset(ContextRenderLock&)
{

}

bool GranulationNode::RenderGranulation(ContextRenderLock & r, AudioBus * out_bus, size_t destinationFrameOffset, size_t numberOfFrames)
{
    if (!r.context())
        return false;

    // Sanity check destinationFrameOffset, numberOfFrames.
    const size_t destinationLength = out_bus->length();

    bool isLengthGood = destinationLength <= 4096 && numberOfFrames <= 4096;
    ASSERT(isLengthGood);
    if (!isLengthGood)
        return false;

    bool isOffsetGood = destinationFrameOffset <= destinationLength && destinationFrameOffset + numberOfFrames <= destinationLength;
    ASSERT(isOffsetGood);
    if (!isOffsetGood)
        return false;

    if (!out_bus) return false;

    float * mono_destination = out_bus->channel(0)->mutableData();

    // @fixme - this vector
    std::vector<float> grain_sum_buffer(numberOfFrames, 0.f);

    {
        for (int i = 0; i < grain_pool.size(); ++i)
        {
            grain_pool[i].tick(grain_sum_buffer.data(), grain_sum_buffer.size());
        }

        for (int f = 0; f < numberOfFrames; ++f)
        {
            grain_sum_buffer[f] /= static_cast<float>(grain_pool.size());
            mono_destination[f] = grain_sum_buffer[f]; // @todo - memcpy
        }

        // Optional grain culling when they're "done"
        // if (!grain_pool.empty())
        // {
        //     auto it = std::remove_if(std::begin(grain_pool), std::end(grain_pool), [this](const grain & g)
        //     {
        //         return g.in_use;
        //     });
        //     grain_pool.erase(it, std::end(grain_pool));
        // }
    }

    out_bus->clearSilentFlag();

    return true;
}

bool GranulationNode::setGrainSource(ContextRenderLock & r, std::shared_ptr<AudioBus> buffer)
{
    ASSERT(m_sourceBus);

    m_sourceBus->setBus(buffer.get());
    output(0)->setNumberOfChannels(r, buffer ? buffer->numberOfChannels() : 0);

    UniformRandomGenerator rnd;

    auto sample_to_granulate = m_sourceBus->valueBus();
    const float * sample_ptr = sample_to_granulate->channel(0)->data();
    const size_t sample_length = sample_to_granulate->channel(0)->length();

    // Setup grains
    const float grain_duration_seconds = 0.5f; 
    const uint64_t grain_duration_samples = grain_duration_seconds * r.context()->sampleRate();
    std::vector<float> grain_window(grain_duration_samples, 1.f);
    lab::ApplyWindowFunctionInplace(WindowFunction::rectangle, grain_window.data(), grain_window.size());

    // Setup window/envelope
    window_bus.reset(new lab::AudioBus(1, grain_duration_samples));
    window_bus->setSampleRate(r.context()->sampleRate());
    float * windowData = window_bus->channel(0)->mutableData();
    for (uint32_t sample = 0; sample < grain_window.size(); ++sample)
    {
        windowData[sample] = grain_window[sample];
    }

    // Number of grains... 
    for (int i = 0; i < 128; ++i)
    {
        grain_pool.emplace_back(grain(sample_to_granulate, window_bus, r.context()->sampleRate(), rnd.random_float(), grain_duration_seconds, 1.0));
    }

    return true;
}

void GranulationNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    AudioBus * outputBus = output(0)->bus(r);

    if (!isInitialized() || !outputBus->numberOfChannels()) 
    {
        outputBus->zero();
        return;
    }

    size_t quantumFrameOffset;
    size_t bufferFramesToProcess;
    updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, bufferFramesToProcess);

    if (!bufferFramesToProcess)
    {
        outputBus->zero();
        return;
    }

    if (!RenderGranulation(r, outputBus, quantumFrameOffset, bufferFramesToProcess))
    {
        outputBus->zero();
        return;
    }
    outputBus->clearSilentFlag();
}
    
bool GranulationNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}
    