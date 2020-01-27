// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef audio_hardware_input_node
#define audio_hardware_input_node

#include "LabSound/core/AudioSourceNode.h"
#include "LabSound/core/AudioSourceProvider.h"

namespace lab
{
    class AudioContext;

    class AudioHardwareInputNode : public AudioSourceNode
    {
        // As an audio source, we will never propagate silence.
        virtual bool propagatesSilence(ContextRenderLock & r) const override { return false; }
        AudioSourceProvider * m_audioSourceProvider;
        size_t m_sourceNumberOfChannels {0};

    public:
        AudioHardwareInputNode(AudioSourceProvider *);
        virtual ~AudioHardwareInputNode();

        // AudioNode
        virtual void process(ContextRenderLock &, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock &) override;
    };

}  // namespace lab

#endif
