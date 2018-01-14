// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef WaveShaperProcessor_h
#define WaveShaperProcessor_h

#include "internal/AudioDSPKernel.h"
#include "internal/AudioDSPKernelProcessor.h"

namespace lab {

// WaveShaperProcessor is an AudioDSPKernelProcessor which uses WaveShaperDSPKernel objects to implement non-linear distortion effects.

class WaveShaperProcessor : public AudioDSPKernelProcessor 
{

public:

    WaveShaperProcessor(size_t numberOfChannels);
    virtual ~WaveShaperProcessor();
    virtual AudioDSPKernel* createKernel();
    virtual void process(ContextRenderLock&, const AudioBus* source, AudioBus* destination, size_t framesToProcess);
    void setCurve(const std::vector<float> & curve);
    std::vector<float> & curve() { return m_curve; }

private:

    std::vector<float> m_curve;
    std::vector<float> m_newCurve;
};

} // namespace lab

#endif // WaveShaperProcessor_h
