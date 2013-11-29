// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "PannerNode.h"
#include <map>

namespace LabSound {
    
    class Occluder {
    public:
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
    
    class Occluders {
    public:
        void setOccluder(int id,
                         float x, float y, float z,
                         float radius);
        
        void removeOccluder(int id);
        
        float occlusion(const WebCore::FloatPoint3D& sourcePos, const WebCore::FloatPoint3D& listenerPos) const;

    private:
        std::map<int, Occluder> occluders;
    };
    
    typedef std::shared_ptr<Occluders> OccludersPtr;

    class SpatializationNode : public WebCore::PannerNode {
    public:
        static WTF::PassRefPtr<SpatializationNode> create(WebCore::AudioContext* context, float sampleRate)
        {
            return adoptRef(new SpatializationNode(context, sampleRate));
        }
        
        void setOccluders(OccludersPtr);
        
    private:
        SpatializationNode(WebCore::AudioContext* context, float sampleRate);
        virtual ~SpatializationNode();

        virtual float distanceConeGain();
        
        std::shared_ptr<Occluders> occluders;
    };
    
}

