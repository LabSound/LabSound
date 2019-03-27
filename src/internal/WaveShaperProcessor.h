// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef WaveShaperProcessor_h
#define WaveShaperProcessor_h

#include "internal/AudioDSPKernel.h"
#include "internal/AudioDSPKernelProcessor.h"
#include <memory>
#include <mutex>
#include <vector>

namespace lab
{

// WaveShaperProcessor is an AudioDSPKernelProcessor which uses WaveShaperDSPKernel objects to implement non-linear distortion effects.

class WaveShaperProcessor : public AudioDSPKernelProcessor
{
public:
    WaveShaperProcessor(size_t numberOfChannels);
    virtual ~WaveShaperProcessor();
    virtual AudioDSPKernel * createKernel();
    virtual void process(ContextRenderLock &, const AudioBus * source, AudioBus * destination, size_t framesToProcess);

    // curve is moved into setCurve, and contents will be mutated safely
    void setCurve(std::vector<float> && curve);

    struct Curve
    {
        explicit Curve(std::mutex& m, const std::vector<float> & c)
        : lock(m)
        , curve(c)
        {
        }
        ~Curve() = default;

        std::lock_guard<std::mutex> lock;
        const std::vector<float> & curve;
    };

    // accessing curve is heavyweight because of the need to prevent
    // another thread from overwriting the Curve object while it is in use.
    // the alternative is to copy the curve, but that isn't great either.
    std::unique_ptr<Curve> curve();

private:
    std::mutex m_curveWrite;
    std::vector<float> m_curve;
    std::vector<float> m_newCurve;
};

}  // namespace lab

#endif  // WaveShaperProcessor_h
