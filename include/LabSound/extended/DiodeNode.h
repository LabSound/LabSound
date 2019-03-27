// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef DIODE_NODE_H
#define DIODE_NODE_H

#include "LabSound/core/WaveShaperNode.h"

namespace lab {

    //@tofix - DiodeNode should subclass waveShaper, then the create method will work
    // params:
    // settings: distortion
    //
    class DiodeNode
    {
        void setCurve();
        std::shared_ptr<WaveShaperNode> waveShaper;
        std::shared_ptr<AudioSetting> _distortion;

        float vb, vl;  // parameters controlling the shape of the curve
    public:

        DiodeNode();
        void setDistortion(float distortion = 1.0);
        std::shared_ptr<WaveShaperNode> node() const { return waveShaper; }
    };
    
}

#endif
