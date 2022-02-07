// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/WaveShaperNode.h"
#include "LabSound/core/AudioBus.h"
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

WaveShaperNode::WaveShaperNode(AudioContext & ac)
    : AudioNode(ac, *desc())
{
    addInput("in");
    addOutput(1, AudioNode::ProcessingSizeInFrames);
    initialize();
    _curveId = 0;
    _newCurveId = 0;
}

WaveShaperNode::WaveShaperNode(AudioContext & ac, AudioNodeDescriptor const & desc)
    : AudioNode(ac, desc)
{
    addInput("in");
    addOutput(1, AudioNode::ProcessingSizeInFrames);
    initialize();
    _curveId = 0;
    _newCurveId = 0;
}


WaveShaperNode::~WaveShaperNode()
{
}

void WaveShaperNode::setCurve(std::vector<float> & curve)
{
    std::lock_guard<std::mutex> guard(_curveMutex);
    _newCurve = curve;
    _newCurveId++;
}

void WaveShaperNode::process(ContextRenderLock & r, int bufferSize)
{
    if (_newCurveId > _curveId)
    {
        // the lock only occurs on setting a new curve, so any glitching should be acceptable
        // because setting a new curve should be very rare
        std::lock_guard<std::mutex> guard(_curveMutex);
        std::swap(_curve, _newCurve);
        _curveId = _newCurveId;
    }

    AudioBus * destinationBus = outputBus(r);
    AudioBus * sourceBus = inputBus(r, 0);
    if (!destinationBus || !sourceBus || !_curve.size())
    {
        if (destinationBus)
            destinationBus->zero();
        return;
    }

    int srcChannelCount = sourceBus->numberOfChannels();
    int dstChannelCount = destinationBus->numberOfChannels();
    if (srcChannelCount != dstChannelCount)
    {
        destinationBus->setNumberOfChannels(r, srcChannelCount);
    }

    for (int i = 0; i < srcChannelCount; ++i)
    {
        const float * source = sourceBus->channel(i)->data();
        float * destination = destinationBus->channel(i)->mutableData();
        int framesToProcess = bufferSize;

        float const * curveData = _curve.data();
        int curveLength = static_cast<int>(_curve.size());

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
            int index = static_cast<size_t>((curveLength * (input + 1)) / 2);

            // Clip index to the input range of the curve.
            // This takes care of input outside of nominal range -1 -> +1
            index = std::max(index, 0);
            index = std::min(index, curveLength - 1);
            destination[i] = curveData[index];
        }
    }
}

}  // namespace lab
