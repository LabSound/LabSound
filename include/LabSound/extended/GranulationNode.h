// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#ifndef labsound_granulation_node_h
#define labsound_granulation_node_h

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioParam.h"

namespace lab
{

    class GranulationNode : public AudioScheduledSourceNode
    {
        std::shared_ptr<AudioSetting> m_sourceBus;

        virtual bool propagatesSilence(ContextRenderLock & r) const override;
        virtual double tailTime(ContextRenderLock& r) const override { return 0; }
        virtual double latencyTime(ContextRenderLock& r) const override { return 0; }

        bool RenderGranulation(ContextRenderLock &, AudioBus *, size_t destinationFrameOffset, size_t numberOfFrames);

        // m_virtualReadIndex is a sample-frame index into our buffer representing the current playback position.
        // Since it's floating-point, it has sub-sample accuracy.
        double m_virtualReadIndex {0.0};

    public:

        GranulationNode();
        virtual ~GranulationNode();

        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;

        bool setGrainSource(ContextRenderLock &, std::shared_ptr<AudioBus> sourceBus);
        std::shared_ptr<AudioBus> getBus() const { return m_sourceBus->valueBus(); }
    };
    
} // end namespace lab

#endif // end labsound_granulation_node_h
