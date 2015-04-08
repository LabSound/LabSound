//
//  AudioDSPKernel.cpp
//  LabSound
//
// License is MIT: http://opensource.org/licenses/MIT

#include "internal/AudioDSPKernel.h"
#include "internal/AudioDSPKernelProcessor.h"

namespace WebCore {
    
    AudioDSPKernel::AudioDSPKernel(AudioDSPKernelProcessor* kernelProcessor) : m_kernelProcessor(kernelProcessor) , m_sampleRate(kernelProcessor->sampleRate())
    {
    }
    
    AudioDSPKernel::AudioDSPKernel(float sampleRate) : m_kernelProcessor(0), m_sampleRate(sampleRate)
    {
    }
    
}
