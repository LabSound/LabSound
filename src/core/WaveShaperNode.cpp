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
#include <vector>
#include "internal/UpSampler.h" //ouch..how to hide internal from calling application?
#include "internal/DownSampler.h"

namespace lab {
struct OverSamplingArrays
{
    std::shared_ptr<AudioFloatArray> m_tempBuffer;
    std::shared_ptr<AudioFloatArray> m_tempBuffer2;
    std::shared_ptr<UpSampler> m_upSampler;
    std::shared_ptr<DownSampler> m_downSampler;
    std::shared_ptr<UpSampler> m_upSampler2;
    std::shared_ptr<DownSampler> m_downSampler2;
};

static void* createOversamplingArrays()
{
    struct OverSamplingArrays * osa = new struct OverSamplingArrays;
    int renderQuantumSize = 128;  // from https://www.w3.org/TR/webaudio/#render-quantum-size
    osa->m_tempBuffer = std::make_shared<AudioFloatArray>(renderQuantumSize * 2);
    osa->m_tempBuffer2 = std::make_shared<AudioFloatArray>(renderQuantumSize * 4);
    osa->m_upSampler = std::make_shared<UpSampler>(renderQuantumSize);
    osa->m_downSampler = std::make_shared<DownSampler>(renderQuantumSize * 2);
    osa->m_upSampler2 = std::make_shared<UpSampler>(renderQuantumSize * 2);
    osa->m_downSampler2 = std::make_shared<DownSampler>(renderQuantumSize * 4);
    return (void *) osa;
}

AudioNodeDescriptor * WaveShaperNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr, 1};
    return &d;
}
    
WaveShaperNode::WaveShaperNode(AudioContext& ac)
: AudioNode(ac, *desc())
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    initialize();
}
    
WaveShaperNode::WaveShaperNode(AudioContext & ac, AudioNodeDescriptor const & desc)
: AudioNode(ac, desc)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    initialize();
}

WaveShaperNode::~WaveShaperNode()
{
    delete (OverSamplingArrays*) m_oversamplingArrays;
}

void WaveShaperNode::setCurve(std::vector<float> & curve)
{
    std::lock_guard<std::mutex> lock(_curveMutex);
    m_newCurve = curve;
    _newCurveReady = 1;
}

void WaveShaperNode::processCurve(const float* source, float* destination, int framesToProcess)
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
// https://github.com/WebKit/WebKit/blob/main/Source/WebCore/Modules/webaudio/WaveShaperDSPKernel.cpp
void WaveShaperNode::processCurve2x(const float * source, float * destination, int framesToProcess)
{
    OverSamplingArrays * osa = (OverSamplingArrays *) m_oversamplingArrays;
    
    auto hold_buffer_pointer = osa->m_tempBuffer;
    auto hold_sampler = osa->m_upSampler;
    float * tempP = osa->m_tempBuffer->data();

    osa->m_upSampler->process(source, tempP, framesToProcess);

    // Process at 2x up-sampled rate.
    processCurve(tempP, tempP, framesToProcess * 2);

    osa->m_downSampler->process(tempP, destination, framesToProcess * 2);
}

void WaveShaperNode::processCurve4x(const float * source, float * destination, int framesToProcess)
{
    OverSamplingArrays * osa = (OverSamplingArrays *) m_oversamplingArrays;

    auto hold_buffer_pointer = osa->m_tempBuffer;
    auto hold_buffer_pointer2 = osa->m_tempBuffer2;
    auto hold_sampler = osa->m_upSampler;
    auto hold_sampler2 = osa->m_upSampler2;

    
    float * tempP = osa->m_tempBuffer->data();
    float * tempP2 = osa->m_tempBuffer2->data();

    osa->m_upSampler->process(source, tempP, framesToProcess);
    osa->m_upSampler2->process(tempP, tempP2, framesToProcess * 2);

    // Process at 4x up-sampled rate.
    processCurve(tempP2, tempP2, framesToProcess * 4);

    osa->m_downSampler2->process(tempP2, tempP, framesToProcess * 4);
    osa->m_downSampler->process(tempP, destination, framesToProcess * 2);
}

void WaveShaperNode::process(ContextRenderLock & r, int bufferSize)
{
    if (_newCurveReady)
    {
        // this could cause a pop, but setting a curve should be extremely rare
        std::lock_guard<std::mutex> lock(_curveMutex);
        std::swap(m_curve, m_newCurve);
        _newCurveReady = 0;
    }

    AudioBus* destinationBus = output(0)->bus(r);
    if (!isInitialized() || !m_curve.size())
    {
        destinationBus->zero();
        return;
    }

    AudioBus* sourceBus = input(0)->bus(r);
    if (!input(0)->isConnected())
    {
        destinationBus->zero();
        return;
    }

    int srcChannelCount = sourceBus->numberOfChannels();
    int dstChannelCount = destinationBus->numberOfChannels();
    if (srcChannelCount != dstChannelCount)
    {
        output(0)->setNumberOfChannels(r, srcChannelCount);
        destinationBus = output(0)->bus(r);
    }
    if (m_oversample != OverSampleType::NONE && !m_oversamplingArrays)
         m_oversamplingArrays = createOversamplingArrays();

    for (int i = 0; i < srcChannelCount; ++i)
    {
        const float * source = sourceBus->channel(i)->data();
        float * destination = destinationBus->channel(i)->mutableData();
        int framesToProcess = bufferSize;
        switch (m_oversample)
        {
            case OverSampleType::NONE:
                processCurve(source, destination, framesToProcess);
                break;
            case OverSampleType::_2X :
                processCurve2x(source, destination, framesToProcess);
                break;
            case OverSampleType::_4X :
                processCurve4x(source, destination, framesToProcess);
                break;

            default:
                ASSERT_NOT_REACHED();
        }

        //processCurve( sourceBus->channel(i)->data(), destinationBus->channel(i)->mutableData(), bufferSize);
    }
}

}  // namespace lab
