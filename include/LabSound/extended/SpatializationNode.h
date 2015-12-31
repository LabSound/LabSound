// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef SPATIALIZATION_NODE_H
#define SPATIALIZATION_NODE_H

#include "LabSound/core/PannerNode.h"
#include <map>

namespace lab {

    struct Occluder 
    {
        float x, y, z;
        float innerRadius;
        float outerRadius;
        float maxAttenuation;
        
        Occluder() { }
        
        Occluder(float x, float y, float z, float radius)
        : x(x), y(y), z(z)
        , outerRadius(radius)
        , innerRadius(radius * 0.75f)
        , maxAttenuation(0) { }
        
        Occluder(const Occluder& rhs)
        : x(rhs.x), y(rhs.y), z(rhs.z)
        , innerRadius(rhs.innerRadius), outerRadius(rhs.outerRadius)
        , maxAttenuation(rhs.maxAttenuation) { }
        
        Occluder& operator=(const Occluder& rhs) {
            x = rhs.x; y = rhs.y; z = rhs.z;
            innerRadius = rhs.innerRadius; outerRadius = rhs.outerRadius;
            maxAttenuation = rhs.maxAttenuation;
            return *this; }
    };
    
    class Occluders 
    {

    public:
        void setOccluder(int id, float x, float y, float z, float radius);
        
        void removeOccluder(int id);
        
        float occlusion(const FloatPoint3D & sourcePos, const FloatPoint3D & listenerPos) const;

    private:

        std::map<int, Occluder> occluders;

    };
    
    typedef std::shared_ptr<Occluders> OccludersPtr;

    class SpatializationNode : public PannerNode 
    {

    public:
        SpatializationNode(float sampleRate);
        virtual ~SpatializationNode();

        void setOccluders(OccludersPtr);
        
    private:

        virtual float distanceConeGain(ContextRenderLock& r);
        
        std::shared_ptr<Occluders> occluders;
    };
    
}

#endif
