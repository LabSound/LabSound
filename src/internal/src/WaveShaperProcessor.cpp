// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/AudioContextLock.h"

#include "internal/WaveShaperProcessor.h"
#include "internal/WaveShaperDSPKernel.h"
#include "internal/Assertions.h"

namespace lab {
    
WaveShaperProcessor::WaveShaperProcessor(float sampleRate, size_t numberOfChannels)
    : AudioDSPKernelProcessor(sampleRate, numberOfChannels)
{
}

WaveShaperProcessor::~WaveShaperProcessor()
{
    if (isInitialized())
        uninitialize();
}

AudioDSPKernel* WaveShaperProcessor::createKernel()
{
    return new WaveShaperDSPKernel(this);
}

void WaveShaperProcessor::setCurve(ContextRenderLock& r, std::shared_ptr<std::vector<float>> curve)
{
    // can't rewrite the curve whilst rendering
    ASSERT(r.context());
    m_curve = curve;
}

void WaveShaperProcessor::process(ContextRenderLock& r, const AudioBus* source, AudioBus* destination, size_t framesToProcess)
{
    if (!isInitialized() || !r.context()) {
        destination->zero();
        return;
    }
    
    bool channelCountMatches = source->numberOfChannels() == destination->numberOfChannels() &&
                               source->numberOfChannels() == m_kernels.size();
    
    if (!channelCountMatches)
        return;

    // For each channel of our input, process using the corresponding WaveShaperDSPKernel into the output channel.
    for (unsigned i = 0; i < m_kernels.size(); ++i)
        m_kernels[i]->process(r, source->channel(i)->data(),
                                 destination->channel(i)->mutableData(), framesToProcess);
}


} // namespace lab
