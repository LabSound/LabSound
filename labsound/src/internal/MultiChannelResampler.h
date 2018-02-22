// License: BSD 3 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef MultiChannelResampler_h
#define MultiChannelResampler_h

#include "internal/SincResampler.h"

#include <vector>
#include <memory>

namespace lab {
    
class AudioBus;
class ContextRenderLock;
    
class MultiChannelResampler {
public:   
    MultiChannelResampler(double scaleFactor, unsigned numberOfChannels);
    
    // Process given AudioSourceProvider for streaming applications.
    void process(ContextRenderLock&, AudioSourceProvider*, AudioBus* destination, size_t framesToProcess);

private:
    // FIXME: the mac port can have a more highly optimized implementation based on CoreAudio
    // instead of SincResampler. For now the default implementation will be used on all ports.
    // https://bugs.webkit.org/show_bug.cgi?id=75118
    
    // Each channel will be resampled using a high-quality SincResampler.
    std::vector<std::unique_ptr<SincResampler> > m_kernels;
    
    unsigned m_numberOfChannels;
};

} // namespace lab

#endif // MultiChannelResampler_h
