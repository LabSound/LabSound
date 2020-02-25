// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/GranulationNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"

#include <random>

using namespace lab;

class uniform_random_gen
{
    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_real_distribution<float> dist_full { 0.f, 1.f };
public:
    uniform_random_gen() : rd(), gen(rd()) { }
    float random_float() { return dist_full(gen); } // [0.f, 1.f]
    float random_float(float max) { std::uniform_real_distribution<float> dist_user(0.f, max); return dist_user(gen); }
    float random_float(float min, float max) { std::uniform_real_distribution<float> dist_user(min, max); return dist_user(gen); }
    uint32_t random_uint(uint32_t max) { std::uniform_int_distribution<uint32_t> dist_int(0, max); return dist_int(gen); }
    int32_t random_int(int32_t min, int32_t max) { std::uniform_int_distribution<int32_t> dist_int(min, max); return dist_int(gen); }
};

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

    // @fixme - use of shared_ptr in render/processing chain
    std::shared_ptr<AudioBus> srcBus = getBus();

    if (!out_bus || !srcBus)
        return false;

    const size_t bufferLength = srcBus->length();

    int framesToProcess = static_cast<int>(numberOfFrames);

    float * destination = out_bus->channel(0)->mutableData();

    // @fixme - this vector
    std::vector<float> grain_sum_buffer(framesToProcess, 0.f);

    {
        for (int i = 0; i < grain_pool.size(); ++i)
        {
            grain_pool[i].tick(grain_sum_buffer.data(), grain_sum_buffer.size());
        }

        for (int f = 0; f < framesToProcess; ++f)
        {
            grain_sum_buffer[f] /= static_cast<float>(grain_pool.size());
            destination[f] = grain_sum_buffer[f]; // @todo - memcpy
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

    uniform_random_gen rnd;

    auto sample_to_granulate = m_sourceBus->valueBus();
    const float * sample_ptr = sample_to_granulate->channel(0)->data();
    const size_t sample_length = sample_to_granulate->channel(0)->length();

    // Setup grains
    const float grain_duration_seconds = 0.5f; 
    const uint64_t grain_duration_samples = grain_duration_seconds * r.context()->sampleRate();
    std::vector<float> grain_window(grain_duration_samples, 1.f);
    lab::applyWindow(WindowType::window_blackman_harris, grain_window);

    // Setup window/envelope
    std::shared_ptr<lab::AudioBus> window_bus(new lab::AudioBus(1, grain_duration_samples));
    window_bus->setSampleRate(r.context()->sampleRate());
    float * windowData = window_bus->channel(0)->mutableData();
    for (int sample = 0; sample < grain_window.size(); ++sample)
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
    