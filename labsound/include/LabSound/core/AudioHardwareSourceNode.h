// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioHardwareSourceNode_h
#define AudioHardwareSourceNode_h

#include "LabSound/core/AudioSourceNode.h"
#include "LabSound/core/AudioSourceProvider.h"

namespace lab {

class AudioContext;
    
class AudioHardwareSourceNode : public AudioSourceNode, public AudioSourceProviderClient
{

public:

    AudioHardwareSourceNode(const float sampleRate, AudioSourceProvider *);
    virtual ~AudioHardwareSourceNode();

    // AudioNode
    virtual void process(ContextRenderLock &, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock &) override;

    // Interface for AudioSourceProviderClient
    virtual void setFormat(ContextRenderLock & r, size_t numberOfChannels, float sourceSampleRate) override;

    AudioSourceProvider * audioSourceProvider() const { return m_audioSourceProvider; }

private:

    // As an audio source, we will never propagate silence.
    virtual bool propagatesSilence(ContextRenderLock & r) const override { return false; }

    AudioSourceProvider * m_audioSourceProvider;

    unsigned m_sourceNumberOfChannels;
};

} // namespace lab

#endif
