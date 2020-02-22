
// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#ifndef SUPERSAW_NODE_H
#define SUPERSAW_NODE_H

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"

namespace lab 
{
    class SupersawNode : public AudioScheduledSourceNode 
    {
        class SupersawNodeInternal;
        std::unique_ptr<SupersawNodeInternal> internalNode;

    public:

        SupersawNode();
        virtual ~SupersawNode();
        
        std::shared_ptr<AudioSetting> attack() const;
        std::shared_ptr<AudioSetting> decay() const;
        std::shared_ptr<AudioSetting> sustain() const;
        std::shared_ptr<AudioSetting> release() const;

        std::shared_ptr<AudioSetting> sawCount() const;
        std::shared_ptr<AudioParam> frequency() const;
        std::shared_ptr<AudioParam> detune() const;

        void noteOn(double when);
        void noteOff(ContextRenderLock&, double when);

        void update(ContextRenderLock& r); // call if sawCount is changed. CBB: update automatically

    private:

        virtual void process(ContextRenderLock&, size_t) override;

        virtual void reset(ContextRenderLock&) override { }

        virtual double tailTime(ContextRenderLock & r) const override { return 0; }
        virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

        virtual bool propagatesSilence(ContextRenderLock & r) const override;
    };
}

#endif
