// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/PannerNode.h"
#include "LabSound/core/FloatPoint3D.h"

#include "LabSound/extended/SpatializationNode.h"
#include "LabSound/extended/AudioContextLock.h"

namespace lab
{
    
    namespace 
    {
        // http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
        float distanceFromPointToLine(const FloatPoint3D& x0, const FloatPoint3D& x1, const FloatPoint3D& x2, float& t)
        {
            FloatPoint3D x2x1 = x2 - x1;
            FloatPoint3D x1x0 = x1 - x0;
            float l = magnitude(x2x1);
            t = dot(-x1x0, x2x1) / (l * l);
            
            FloatPoint3D x0x1 = x0 - x1;
            FloatPoint3D x0x2 = x0 - x2;
            float d = magnitude(cross(x0x1, x0x2)) / magnitude(x2x1);
            return d;
        }
    }
    
    void Occluders::setOccluder(int id, float x, float y, float z, float radius)
    {
        Occluder o(x, y, z, radius);
        occluders[id] = o;
    }
    
    void Occluders::removeOccluder(int id)
    {
        auto i = occluders.find(id);
        if (i != occluders.end())
            occluders.erase(i);
    }
    
    float Occluders::occlusion(const FloatPoint3D & sourcePos, const FloatPoint3D & listenerPos) const
    {
        float occlusionAttenuation = 1.0f;
        for (auto i : occluders)
        {
            FloatPoint3D occPos(i.second.x, i.second.y, i.second.z);
            
            float t;
            float d = distanceFromPointToLine(occPos, listenerPos, sourcePos, t);
            
            if (t <= 0 || t >= 1)
                continue;

            //printf("param(%f), dist: %f\n", t, d);
            float maxAtten = i.second.maxAttenuation;
            
            if (d <= i.second.innerRadius)
            {
                occlusionAttenuation *= maxAtten;
            }
            else if (d <= i.second.outerRadius)
            {
                float inner = i.second.innerRadius;
                float t = (d - inner) / (i.second.outerRadius - inner);
                occlusionAttenuation *= maxAtten + (1.0f - maxAtten) * t;
            }
        }
        return occlusionAttenuation;
    }
    
    // @tofix - pass in HRTF loader path
    SpatializationNode::SpatializationNode(float sampleRate): lab::PannerNode(sampleRate)
    {
        setNodeType(lab::NodeType::NodeTypeSpatialization);
        initialize();
    }
    
    SpatializationNode::~SpatializationNode()
    {
        
    }
    
    void SpatializationNode::setOccluders(std::shared_ptr<Occluders> o)
    {
        occluders = o;
    }
    
    float SpatializationNode::distanceConeGain(ContextRenderLock& r)
    {
        if (!r.context())
            return 1.f;
        
        float occlusionAttenuation = occluders ? occluders->occlusion(m_position, listener(r)->position()) : 1.0f;
        return occlusionAttenuation * PannerNode::distanceConeGain(r);
    }
}
