// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/AudioDSPKernelProcessor.h"
#include "internal/DelayProcessor.h"
#include "internal/DelayDSPKernel.h"

using namespace std;

namespace lab {

DelayProcessor::DelayProcessor(float sampleRate, unsigned numberOfChannels, double maxDelayTime) : 
AudioDSPKernelProcessor(sampleRate, numberOfChannels), m_maxDelayTime(maxDelayTime)
{
    m_delayTime = std::make_shared<AudioParam>("delayTime", 0.0, 0.0, maxDelayTime);
}

DelayProcessor::~DelayProcessor()
{
    if (isInitialized())
        uninitialize();
}

AudioDSPKernel * DelayProcessor::createKernel()
{
    return new DelayDSPKernel(this);
}

} // namespace lab
