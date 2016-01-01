// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef DIODE_NODE_H
#define DIODE_NODE_H

#include "LabSound/core/WaveShaperNode.h"

namespace lab {

    //@tofix - DiodeNode should subclass waveShaper, then the create method will work
    
    class DiodeNode
    {
    public:
        DiodeNode(ContextRenderLock& r, float sampleRate);
        void setDistortion(ContextRenderLock& r, float distortion);
        std::shared_ptr<WaveShaperNode> node() const { return waveShaper; }

    private:
        void setCurve(ContextRenderLock&);

        std::shared_ptr<WaveShaperNode> waveShaper;
        
        // parameters controlling the shape of the curve
        float vb;
        float vl;
        float h;
    };
    
}

#endif
