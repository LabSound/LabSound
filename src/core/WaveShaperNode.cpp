// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/WaveShaperNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "internal/Assertions.h"
#include "internal/AudioDSPKernel.h"
#include "internal/AudioDSPKernelProcessor.h"
#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

namespace lab
{


// WaveShaperProcessor is an AudioDSPKernelProcessor which uses WaveShaperDSPKernel objects to implement non-linear distortion effects.

class WaveShaperProcessor : public AudioDSPKernelProcessor
{
public:
    WaveShaperProcessor();
    virtual ~WaveShaperProcessor() = default;

    virtual AudioDSPKernel * createKernel();
    virtual void process(ContextRenderLock &, const AudioBus * source, AudioBus * destination, int framesToProcess);

    // curve is moved into setCurve, and contents will be mutated safely
    /// @TODO use some other mechanism to achieve lockfree operation
    void setCurve(std::vector<float> && curve);

    struct Curve
    {
        explicit Curve(std::mutex & m, const std::vector<float> & c)
            : lock(m)
            , curve(c)
        {
        }
        ~Curve() = default;

        std::lock_guard<std::mutex> lock;
        const std::vector<float> & curve;
    };

    // accessing curve is heavyweight because of the need to prevent
    // another thread from overwriting the Curve object while it is in use.
    // the alternative is to copy the curve, but that isn't great either.
    std::unique_ptr<Curve> curve();

private:
    std::mutex m_curveWrite;
    std::vector<float> m_curve;
    std::vector<float> m_newCurve;
};


// WaveShaperDSPKernel is an AudioDSPKernel and is responsible for non-linear distortion on one channel.

class WaveShaperDSPKernel : public AudioDSPKernel
{
public:
    explicit WaveShaperDSPKernel(WaveShaperProcessor* processor)
        : AudioDSPKernel(processor)
    {
    }

    // AudioDSPKernel
    virtual void process(ContextRenderLock&,
        const float* source, float* dest, int framesToProcess) override;
    virtual void reset() override {}

    virtual double tailTime(ContextRenderLock& r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock& r) const override { return 0; }

protected:
    WaveShaperProcessor* waveShaperProcessor() { return static_cast<WaveShaperProcessor*>(processor()); }
};




WaveShaperProcessor::WaveShaperProcessor()
    : AudioDSPKernelProcessor()
{
}

AudioDSPKernel * WaveShaperProcessor::createKernel()
{
    return new WaveShaperDSPKernel(this);
}

void WaveShaperProcessor::setCurve(std::vector<float> && curve)
{
    std::lock_guard<std::mutex> lock(m_curveWrite);
    std::swap(m_newCurve, curve);
}

void WaveShaperProcessor::process(ContextRenderLock & r, const AudioBus * source, AudioBus * destination, int framesToProcess)
{
    if (!isInitialized() || !r.context())
    {
        destination->zero();
        return;
    }

    /// @todo - make this lock free. Setting a curve might cause an audio glitch.
    if (m_newCurve.size())
    {
        std::lock_guard<std::mutex> lock(m_curveWrite);
        std::swap(m_curve, m_newCurve);
        m_newCurve.clear();
    }


//    kernels are not getting allocated for the diode
  //      just a thought what if setting the channel count set the kernels right away instead of
    //    going through OO Mumbo jumbo

    // For each channel of our input, process using the corresponding WaveShaperDSPKernel into the output channel.
    for (unsigned i = 0; i < m_kernels.size(); ++i)
    {
        m_kernels[i]->process(r, source->channel(i)->data(), destination->channel(i)->mutableData(), framesToProcess);
    }
}

std::unique_ptr<WaveShaperProcessor::Curve> WaveShaperProcessor::curve()
{
    return std::unique_ptr<WaveShaperProcessor::Curve>(new WaveShaperProcessor::Curve(m_curveWrite, m_curve));
}




void WaveShaperDSPKernel::process(ContextRenderLock &, const float * source, float * destination, int framesToProcess)
{
    ASSERT(source && destination && waveShaperProcessor());

    // Curve object locks the curve during processing
    std::unique_ptr<WaveShaperProcessor::Curve> c = waveShaperProcessor()->curve();

    if (c->curve.size() == 0)
    {
        // Act as "straight wire" pass-through if no curve is set.
        memcpy(destination, source, sizeof(float) * framesToProcess);
        return;
    }

    float const * curveData = c->curve.data();
    int curveLength = static_cast<int>(c->curve.size());

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
        int index = static_cast<size_t>((curveLength * (input + 1)) / 2);

        // Clip index to the input range of the curve.
        // This takes care of input outside of nominal range -1 -> +1
        index = std::max(index, 0);
        index = std::min(index, curveLength - 1);
        destination[i] = curveData[index];
    }
}



WaveShaperNode::WaveShaperNode(AudioContext& ac)
    : AudioNode(ac)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

    m_processor.reset(new WaveShaperProcessor());
    initialize();
}

void WaveShaperNode::setCurve(std::vector<float> && curve)
{
    waveShaperProcessor()->setCurve(std::move(curve));
}

#if 0
std::vector<float> & WaveShaperNode::curve()
{
    return waveShaperProcessor()->curve();
}
#endif

WaveShaperProcessor * WaveShaperNode::waveShaperProcessor()
{
    return static_cast<WaveShaperProcessor *>(m_processor.get());
}


void WaveShaperNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * destinationBus = output(0)->bus(r);

    if (!isInitialized() || !m_processor)
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

    int srcChannelCount = input(0)->numberOfChannels(r);
    int dstChannelCount = destinationBus->numberOfChannels();
    if (srcChannelCount != dstChannelCount)
        output(0)->setNumberOfChannels(r, srcChannelCount);

    // process entire buffer
    m_processor->process(r, sourceBus, destinationBus, bufferSize);
}


void WaveShaperNode::reset(ContextRenderLock &)
{
    if (m_processor)
        m_processor->reset();
}


void WaveShaperNode::initialize()
{
    if (isInitialized())
        return;

    ASSERT(m_processor);
    m_processor->initialize();

    AudioNode::initialize();
}

void WaveShaperNode::uninitialize()
{
    if (!isInitialized())
        return;

    ASSERT(m_processor);
    m_processor->uninitialize();

    AudioNode::uninitialize();
}


double WaveShaperNode::tailTime(ContextRenderLock & r) const
{
    return m_processor->tailTime(r);
}

double WaveShaperNode::latencyTime(ContextRenderLock & r) const
{
    return m_processor->latencyTime(r);
}


}  // namespace lab
