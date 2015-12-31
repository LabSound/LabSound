// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/AudioDSPKernel.h"
#include "internal/AudioDSPKernelProcessor.h"

namespace lab {
    
    AudioDSPKernel::AudioDSPKernel(AudioDSPKernelProcessor* kernelProcessor) : m_kernelProcessor(kernelProcessor) , m_sampleRate(kernelProcessor->sampleRate())
    {
    }
    
    AudioDSPKernel::AudioDSPKernel(float sampleRate) : m_kernelProcessor(0), m_sampleRate(sampleRate)
    {
    }
    
}
