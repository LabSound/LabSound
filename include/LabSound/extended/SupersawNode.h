// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef SUPERSAW_NODE_H
#define SUPERSAW_NODE_H

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"

namespace lab 
{
    class SupersawNode : public AudioNode 
    {
        class SupersawNodeInternal;
        std::unique_ptr<SupersawNodeInternal> internalNode;

    public:

        SupersawNode(ContextRenderLock& r, float sampleRate);
        virtual ~SupersawNode();
        
        std::shared_ptr<AudioParam> attack() const;
        std::shared_ptr<AudioParam> decay() const;
        std::shared_ptr<AudioParam> sustain() const;
        std::shared_ptr<AudioParam> release() const;

        std::shared_ptr<AudioParam> sawCount() const;
        std::shared_ptr<AudioParam> frequency() const;
        std::shared_ptr<AudioParam> detune() const;

        void noteOn(double when);
        void noteOff(ContextRenderLock&, double when);

        void update(ContextRenderLock& r); // call if sawCount is changed. CBB: update automatically

    private:

        virtual void process(ContextRenderLock&, size_t) override;
        virtual void reset(ContextRenderLock&) override { /*m_currentSampleFrame = 0;*/ }
        virtual double tailTime() const override { return 0; }
        virtual double latencyTime() const override { return 0; }
        virtual bool propagatesSilence(double now) const override;

    };
}

#endif
