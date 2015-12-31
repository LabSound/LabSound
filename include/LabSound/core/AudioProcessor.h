// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioProcessor_h
#define AudioProcessor_h

#include <stddef.h>

namespace lab 
{
    
class AudioBus;
class ContextRenderLock;
    
// AudioProcessor is an abstract base class representing an audio signal processing object with a single input and a single output,
// where the number of input channels equals the number of output channels. It can be used as one part of a complex DSP algorithm,
// or as the processor for a basic (one input - one output) AudioNode.
class AudioProcessor {
    
public:
    
    AudioProcessor(float sampleRate, unsigned numberOfChannels) : m_numberOfChannels(numberOfChannels), m_sampleRate(sampleRate)
    {
    }

    virtual ~AudioProcessor() { }

    // Full initialization can be done here instead of in the constructor.
    virtual void initialize() = 0;
    virtual void uninitialize() = 0;

    // Processes the source to destination bus.  The number of channels must match in source and destination.
    virtual void process(ContextRenderLock&, const AudioBus* source, AudioBus* destination, size_t framesToProcess) = 0;

    // Resets filter state
    virtual void reset() = 0;

    void setNumberOfChannels(unsigned n) { m_numberOfChannels = n; }
    unsigned numberOfChannels() const { return m_numberOfChannels; }

    bool isInitialized() const { return m_initialized; }

    float sampleRate() const { return m_sampleRate; }

    virtual double tailTime() const = 0;
    virtual double latencyTime() const = 0;

protected:
    bool m_initialized = false;
    unsigned m_numberOfChannels;
    float m_sampleRate;
};

} // namespace lab

#endif // AudioProcessor_h
