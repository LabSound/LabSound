// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef WaveShaperProcessor_h
#define WaveShaperProcessor_h

#include "internal/AudioDSPKernel.h"
#include "internal/AudioDSPKernelProcessor.h"

namespace lab {

// WaveShaperProcessor is an AudioDSPKernelProcessor which uses WaveShaperDSPKernel objects to implement non-linear distortion effects.

class WaveShaperProcessor : public AudioDSPKernelProcessor {
public:
    WaveShaperProcessor(float sampleRate, size_t numberOfChannels);

    virtual ~WaveShaperProcessor();

    virtual AudioDSPKernel* createKernel();

    virtual void process(ContextRenderLock&, const AudioBus* source, AudioBus* destination, size_t framesToProcess);

    void setCurve(ContextRenderLock&, std::shared_ptr<std::vector<float>>);
    std::shared_ptr<std::vector<float>> curve() { return m_curve; }

private:
    // m_curve represents the non-linear shaping curve.
    std::shared_ptr<std::vector<float>> m_curve;
};

} // namespace lab

#endif // WaveShaperProcessor_h
