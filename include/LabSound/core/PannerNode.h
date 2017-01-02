// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef PannerNode_h
#define PannerNode_h

#include "LabSound/core/AudioListener.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/FloatPoint3D.h"

namespace lab {

// PannerNode is an AudioNode with one input and one output.
// It positions a sound in 3D space, with the exact effect dependent on the panning model.
// It has a position and an orientation in 3D space which is relative to the position and orientation of the context's AudioListener.
// A distance effect will attenuate the gain as the position moves away from the listener.
// A cone effect will attenuate the gain as the orientation moves away from the listener.
// All of these effects follow the OpenAL specification very closely.
    
class DistanceEffect;
class ConeEffect;
class AudioBus;
class Panner;
class HRTFDatabaseLoader;
    
class PannerNode : public AudioNode 
{

public:

    enum 
    {
        LINEAR_DISTANCE = 0,
        INVERSE_DISTANCE = 1,
        EXPONENTIAL_DISTANCE = 2,
    };
    
    PannerNode(float sampleRate, const std::string & searchPath = "");
    virtual ~PannerNode();

    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void pullInputs(ContextRenderLock& r, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;
    virtual void initialize() override;
    virtual void uninitialize() override;

    // Listener
    std::shared_ptr<AudioListener> listener(ContextRenderLock & r);

    // Panning model
    PanningMode panningModel() const { return m_panningModel; }
    void setPanningModel(PanningMode m);

    // Position
    FloatPoint3D position() const { return m_position; }
    void setPosition(float x, float y, float z) { m_position = FloatPoint3D(x, y, z); }

    // Orientation
    FloatPoint3D orientation() const { return m_position; }
    void setOrientation(float x, float y, float z) { m_orientation = FloatPoint3D(x, y, z); }

    // Velocity
    FloatPoint3D velocity() const { return m_velocity; }
    void setVelocity(float x, float y, float z) { m_velocity = FloatPoint3D(x, y, z); }

    // Distance parameters
    unsigned short distanceModel();
    void setDistanceModel(unsigned short);

    float refDistance();
    void setRefDistance(float refDistance);

    float maxDistance();
    void setMaxDistance(float maxDistance);

    float rolloffFactor();
    void setRolloffFactor(float rolloffFactor);

    // Sound cones - angles in degrees
    float coneInnerAngle() const;
    void setConeInnerAngle(float angle);

    float coneOuterAngle() const;
    void setConeOuterAngle(float angle);

    float coneOuterGain() const;
    void setConeOuterGain(float angle);

    void getAzimuthElevation(ContextRenderLock & r, double * outAzimuth, double * outElevation);
    float dopplerRate(ContextRenderLock & r);

    // Accessors for dynamically calculated gain values.
    std::shared_ptr<AudioParam> distanceGain() { return m_distanceGain; }
    std::shared_ptr<AudioParam> coneGain() { return m_coneGain; }

    virtual double tailTime() const override;
    virtual double latencyTime() const override;

protected:

    std::shared_ptr<HRTFDatabaseLoader> m_hrtfDatabaseLoader;
    
    // Returns the combined distance and cone gain attenuation.
    virtual float distanceConeGain(ContextRenderLock & r);

    // Notifies any AudioBufferSourceNodes connected to us either directly or indirectly about our existence.
    // This is in order to handle the pitch change necessary for the doppler shift.
    // @tofix - broken?
    void notifyAudioSourcesConnectedToNode(ContextRenderLock & r, AudioNode *);

    std::unique_ptr<Panner> m_panner;

    PanningMode m_panningModel;

    FloatPoint3D m_position;
    FloatPoint3D m_orientation;
    FloatPoint3D m_velocity;

    std::shared_ptr<AudioParam> m_distanceGain;
    std::shared_ptr<AudioParam> m_coneGain;

    std::unique_ptr<DistanceEffect> m_distanceEffect;
    std::unique_ptr<ConeEffect> m_coneEffect;

    float m_lastGain = -1.0f;
    unsigned m_connectionCount = 0;
};

} // namespace lab

#endif // PannerNode_h
