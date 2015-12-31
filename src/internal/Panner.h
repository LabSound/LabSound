// License: BSD 3 Clause
// Copyright (C) 2009, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef Panner_h
#define Panner_h

#include <memory>
#include <LabSound/core/AudioNode.h>

namespace lab 
{

class AudioBus;
class ContextRenderLock;
    
class Panner
{
public:

    Panner(PanningMode mode) : m_panningModel(mode) { }
    virtual ~Panner() { };

    PanningMode panningModel() const { return m_panningModel; }

    virtual void pan(ContextRenderLock& r, double azimuth, double elevation, const AudioBus* inputBus, AudioBus* outputBus, size_t framesToProcess) = 0;

    virtual void reset() = 0;
    virtual double tailTime() const = 0;
    virtual double latencyTime() const = 0;

protected:

    PanningMode m_panningModel;
};

} // lab

#endif 
