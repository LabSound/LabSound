// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef SPECTRAL_MONITOR_NODE_H
#define SPECTRAL_MONITOR_NODE_H

#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/AudioContext.h"

#include <vector>
#include <mutex>

namespace lab 
{
    // params:
    // settings: windowSize
    class SpectralMonitorNode : public AudioBasicInspectorNode 
    {
        class SpectralMonitorNodeInternal;
        SpectralMonitorNodeInternal* internalNode = nullptr;
    public:

        SpectralMonitorNode();
        virtual ~SpectralMonitorNode();

        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;

        void spectralMag(std::vector<float>& result);
        void windowSize(size_t ws);
        size_t windowSize() const;

    private:

        virtual double tailTime(ContextRenderLock & r) const override { return 0; }
        virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
    };
}

#endif
