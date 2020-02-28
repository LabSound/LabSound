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

DelayNode::DelayNode(float sampleRate, double maxDelayTime)
    : AudioBasicProcessorNode()
{
    if (maxDelayTime < 0)
        maxDelayTime = 0;  // delay node can't predict the future

    m_processor = std::make_unique<DelayProcessor>(sampleRate, 1, maxDelayTime);

    m_settings.push_back(delayProcessor()->delayTime());

    initialize();
}

std::shared_ptr<AudioSetting> DelayNode::delayTime()
{
    return delayProcessor()->delayTime();
}

DelayProcessor * DelayNode::delayProcessor()
{
    return static_cast<DelayProcessor *>(processor());
}

}  // lab
