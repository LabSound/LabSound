// License: The MIT License
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.
// Copyright (C) 2013, Two Big Ears Ltd (http://twobigears.com)

#ifndef PEAK_COMP_NODE_H
#define PEAK_COMP_NODE_H

// PeakComp ported to LabSound from https://github.com/twobigears/tb_peakcomp

/*
 Stereo L+R peak compressor with variable knee-smooth, attack, release and makeup gain.
 Varun Nair. varun@twobigears.com

 Inspired by:
 http://www.eecs.qmul.ac.uk/~josh/documents/GiannoulisMassbergReiss-dynamicrangecompression-JAES2012.pdf
 https://ccrma.stanford.edu/~jos/filters/Nonlinear_Filter_Example_Dynamic.htm
 */

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/GainNode.h"

namespace lab
{
    
class PeakCompNode : public lab::AudioBasicProcessorNode {
    
    class PeakCompNodeInternal;
    PeakCompNodeInternal * internalNode; // We do not own this!
    
public:

    PeakCompNode(float sampleRate);
    virtual ~PeakCompNode();

    void set(float aT, float aL, float d, float s, float r);
    
    // Threshold given in dB, default 0
    std::shared_ptr<AudioParam> threshold() const;
    
    // Ratio, default 1:1
    std::shared_ptr<AudioParam> ratio() const;
    
    // Attack in ms, default .0001f
    std::shared_ptr<AudioParam> attack() const;
    
    // Release in ms, default .0001f
    std::shared_ptr<AudioParam> release() const;
    
    // Makeup gain in dB, default 0
    std::shared_ptr<AudioParam> makeup() const;
    
    // Knee smoothing (0 = hard, 1 = smooth), default 0
    std::shared_ptr<AudioParam> knee() const;

};
    
}
#endif
