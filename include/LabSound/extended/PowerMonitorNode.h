// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef POWER_MONITOR_NODE_H
#define POWER_MONITOR_NODE_H

#include "LabSound/core/AudioBasicInspectorNode.h"
#include <vector>
#include <mutex>

namespace lab {
    
    class PowerMonitorNode : public AudioBasicInspectorNode 
    {
    public:

        PowerMonitorNode();

        virtual ~PowerMonitorNode();
        
        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;

        // instantaneous estimation of power
        float db() const { return _db; }

        // Could be better. Power is computed on the most recent frame. If the framesize is greater
        // than the windowSize, then power is returned for the windowed end-of-frame. If framesize
        // is less than windowSize, power is computed only on framesize. This could be a problem
        // if the framesize is very large compared to the sample rate, for example, a 4k framesize
        // on a 44khz samplerate is going to give you usable power measurements 11 times a second.
        // If better resolution is required, it's probably better to use a RecorderNode, and perform
        // analysis on the full data stream pulled from it.
        //
        // The intent of the power monitor node is to provide levels that can be used for a VU meter
        // or a ducking algorithm.
        //
        void windowSize(size_t ws) { _windowSize = ws; }
        size_t windowSize() const { return _windowSize; }
        
    private:

        virtual double tailTime(ContextRenderLock & r) const override { return 0; }      // required for BasicInspector
        virtual double latencyTime(ContextRenderLock & r) const override { return 0; }   // required for BasicInspector

        float _db;
        size_t _windowSize;

    };

}

#endif
