// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/WaveShaperNode.h"
#include "internal/WaveShaperProcessor.h"

namespace lab
{

WaveShaperNode::WaveShaperNode(float sampleRate) : AudioBasicProcessorNode(sampleRate)
{
    m_processor.reset(new lab::WaveShaperProcessor(sampleRate, 1));
    setNodeType(NodeTypeWaveShaper);
    initialize();
}

void WaveShaperNode::setCurve(ContextRenderLock& r, std::shared_ptr<std::vector<float>> curve)
{
    waveShaperProcessor()->setCurve(r, curve);
}

std::shared_ptr<std::vector<float>> WaveShaperNode::curve()
{
    return waveShaperProcessor()->curve();
}

 WaveShaperProcessor * WaveShaperNode::waveShaperProcessor() 
 {
     return static_cast<WaveShaperProcessor *>(m_processor.get());
 }

} // namespace lab
