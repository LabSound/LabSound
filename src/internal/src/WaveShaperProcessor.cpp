// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/AudioContextLock.h"

#include "internal/WaveShaperProcessor.h"
#include "internal/WaveShaperDSPKernel.h"
#include "internal/Assertions.h"

namespace lab {
    
WaveShaperProcessor::WaveShaperProcessor(size_t numberOfChannels) : AudioDSPKernelProcessor(numberOfChannels)
{

}

WaveShaperProcessor::~WaveShaperProcessor()
{
    if (isInitialized()) uninitialize();
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

void WaveShaperProcessor::process(ContextRenderLock& r, const AudioBus* source, AudioBus* destination, size_t framesToProcess)
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
    
    const bool channelCountMatches = source->numberOfChannels() == destination->numberOfChannels() && source->numberOfChannels() == m_kernels.size();
    
    if (!channelCountMatches)
        return;

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


} // namespace lab
