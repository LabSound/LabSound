// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef WaveShaperNode_h
#define WaveShaperNode_h

#include "LabSound/core/AudioBasicProcessorNode.h"

namespace lab
{
   class WaveShaperProcessor;

class WaveShaperNode : public AudioBasicProcessorNode
{
    WaveShaperProcessor * waveShaperProcessor();
public:
    WaveShaperNode();
    virtual ~WaveShaperNode() = default;    

    // setCurve will take ownership of curve
    void setCurve(std::vector<float> && curve);
    std::vector<float> & curve();
};

} // namespace lab

#endif
