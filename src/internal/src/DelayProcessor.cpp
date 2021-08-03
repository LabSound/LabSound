// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/DelayProcessor.h"
#include "internal/AudioDSPKernelProcessor.h"
#include "internal/DelayDSPKernel.h"

using namespace std;

namespace lab
{


DelayProcessor::DelayProcessor(float sampleRate, double maxDelayTime, std::shared_ptr<AudioSetting> t)
    : AudioDSPKernelProcessor()
    , m_maxDelayTime(maxDelayTime)
    , m_sampleRate(sampleRate)
    , m_delayTime(t)
{
}

DelayProcessor::~DelayProcessor()
{
    if (isInitialized())
        uninitialize();
}

AudioDSPKernel * DelayProcessor::createKernel()
{
    return new DelayDSPKernel(this, m_sampleRate);
}

}  // namespace lab
