// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef DelayNode_h
#define DelayNode_h

#include "LabSound/core/AudioBasicProcessorNode.h"

namespace lab 
{

class AudioParam;
class DelayProcessor;

// TempoSync is commonly used by subclasses of DelayNode
enum TempoSync
{
    TS_32,
    TS_16T,
    TS_32D,
    TS_16,
    TS_8T,
    TS_16D,
    TS_8,
    TS_4T,
    TS_8D,
    TS_4,
    TS_2T,
    TS_4D,
    TS_2,
    TS_2D,
};

class DelayNode : public AudioBasicProcessorNode
{
    DelayProcessor * delayProcessor();
public:
    DelayNode(float sampleRate, double maxDelayTime);
    std::shared_ptr<AudioParam> delayTime();
};

} // namespace lab

#endif // DelayNode_h
