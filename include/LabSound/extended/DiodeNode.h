// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef DIODE_NODE_H
#define DIODE_NODE_H

#include "LabSound/core/WaveShaperNode.h"

namespace lab {

    //@tofix - DiodeNode should subclass waveShaper, then the create method will work
    // params:
    // settings: distortion
    //
    class DiodeNode : public WaveShaperNode
    {
        void _precalc();

        std::shared_ptr<WaveShaperNode> waveShaper;
        std::shared_ptr<AudioSetting> _distortion;
        std::shared_ptr<AudioSetting> _vb; // curve shape control
        std::shared_ptr<AudioSetting> _vl; // curve shape control

    public:

        DiodeNode();
        void setDistortion(float distortion = 1.0);

        std::shared_ptr<AudioSetting> distortion() const { return _distortion; };
        std::shared_ptr<AudioSetting> vb() const { return _vb; };
        std::shared_ptr<AudioSetting> vl() const { return _vl; }
    };
    
}

#endif
