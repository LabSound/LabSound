// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/DelayNode.h"
#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioProcessor.h"

#include "internal/AudioDSPKernel.h"
#include "internal/DelayProcessor.h"

namespace lab
{

// WebAudio:
// "The maxDelayTime parameter is optional and specifies the
//  maximum delay time in seconds allowed for the delay line."
const double maximumAllowedDelayTime = 128;

DelayNode::DelayNode(float sampleRate, double maxDelayTime) : AudioBasicProcessorNode(sampleRate)
{
    if (maxDelayTime <= 0 || maxDelayTime >= maximumAllowedDelayTime)
    {
         throw std::out_of_range("Delay time exceeds limit of 128 seconds");
    }
    m_processor.reset(new DelayProcessor(sampleRate, 1, maxDelayTime));
    setNodeType(NodeTypeDelay);

    m_params.push_back(delayProcessor()->delayTime());

    initialize();
}

std::shared_ptr<AudioParam> DelayNode::delayTime()
{
    return delayProcessor()->delayTime();
}

DelayProcessor * DelayNode::delayProcessor()
{
    return static_cast<DelayProcessor*>(processor());
}

} // lab
