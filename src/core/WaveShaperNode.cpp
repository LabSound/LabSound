// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/WaveShaperNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/extended/Registry.h"
#include "internal/Assertions.h"
#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

namespace lab {

AudioNodeDescriptor * WaveShaperNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr};
    return &d;
}
    
WaveShaperNode::WaveShaperNode(AudioContext& ac)
: AudioNode(ac, *desc())
{
    initialize();
}
    
WaveShaperNode::WaveShaperNode(AudioContext & ac, AudioNodeDescriptor const & desc)
: AudioNode(ac, desc)
{
    initialize();
}

WaveShaperNode::~WaveShaperNode()
{
    if (m_newCurve)
    {
        // @TODO mutex
        delete m_newCurve;
    }
}

void WaveShaperNode::setCurve(std::vector<float> & curve)
{
    std::vector<float>* new_curve = new std::vector<float>();
    *new_curve = curve;
    if (m_newCurve)
    {
        // @TODO mutex
        std::vector<float>* x = nullptr;
        std::swap(x, m_newCurve);
        delete x;
    }
    m_newCurve = new_curve;
}

void WaveShaperNode::processBuffer(ContextRenderLock&, const float* source, float* destination, int framesToProcess)
{
    float const* curveData = m_curve.data();
    int curveLength = static_cast<int>(m_curve.size());

    ASSERT(curveData);

    if (!curveData || !curveLength)
    {
        memcpy(destination, source, sizeof(float) * framesToProcess);
        return;
    }

    // Apply waveshaping curve.
    for (int i = 0; i < framesToProcess; ++i)
    {
        const float input = source[i];

        // Calculate an index based on input -1 -> +1 with 0 being at the center of the curve data.
        int index = static_cast<int>( (curveLength * (input + 1)) / 2 );

        // Clip index to the input range of the curve.
        // This takes care of input outside of nominal range -1 -> +1
        index = std::max(index, 0);
        index = std::min(index, curveLength - 1);
        destination[i] = curveData[index];
    }
}

void WaveShaperNode::process(ContextRenderLock & r, int bufferSize)
{
    if (m_newCurve)
    {
        // @TODO mutex
        std::vector<float>* x = nullptr;
        std::swap(x, m_newCurve);
        m_curve = *x;
        delete x;
    }

    AudioBus* destinationBus = _self->output.get();
    if (!isInitialized() || !m_curve.size())
    {
        destinationBus->zero();
        return;
    }

    auto sourceBus = summedInput();
    if (!sourceBus)
    {
        destinationBus->zero();
        return;
    }

    int srcChannelCount = sourceBus->numberOfChannels();
    int dstChannelCount = destinationBus->numberOfChannels();
    for (int i = 0; i < dstChannelCount; ++i)
    {
        if (i < srcChannelCount)
            processBuffer(r, sourceBus->channel(i)->data(), destinationBus->channel(i)->mutableData(), bufferSize);
        else
            destinationBus->channel(i)->zero();
    }
}

}  // namespace lab
