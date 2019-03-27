// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/WaveShaperNode.h"
#include "internal/WaveShaperProcessor.h"

namespace lab
{

WaveShaperNode::WaveShaperNode()
{
    m_processor.reset(new WaveShaperProcessor(1));
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

} // namespace lab
