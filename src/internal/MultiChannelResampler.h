// License: BSD 3 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef MultiChannelResampler_h
#define MultiChannelResampler_h

#include "internal/SincResampler.h"

#include <memory>
#include <vector>

namespace lab
{

class AudioBus;
class ContextRenderLock;

class MultiChannelResampler
{
public:
    MultiChannelResampler(double scaleFactor, unsigned numberOfChannels);

    // Process given AudioSourceProvider for streaming applications.
    void process(ContextRenderLock &, AudioSourceProvider *, AudioBus * destination, int framesToProcess);

private:
    // Each channel will be resampled using a high-quality SincResampler.
    std::vector<std::unique_ptr<SincResampler>> m_kernels;

    int m_numberOfChannels;
};

}  // namespace lab

#endif  // MultiChannelResampler_h
