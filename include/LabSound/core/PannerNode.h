/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PannerNode_h
#define PannerNode_h

#include "LabSound/core/AudioListener.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/FloatPoint3D.h"

namespace WebCore {

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

class PannerNode : public AudioNode 
{

public:

    enum 
	{
        EQUALPOWER = 0,
        HRTF = 1,
        SOUNDFIELD = 2,
    };

    enum 
	{
        LINEAR_DISTANCE = 0,
        INVERSE_DISTANCE = 1,
        EXPONENTIAL_DISTANCE = 2,
    };
    
    PannerNode(float sampleRate);
    virtual ~PannerNode();

    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void pullInputs(ContextRenderLock& r, size_t framesToProcess) override;
    virtual void reset(std::shared_ptr<AudioContext>) override;
    virtual void initialize();
    virtual void uninitialize();

    // Listener
    AudioListener* listener(ContextRenderLock&);

    // Panning model
    unsigned short panningModel() const { return m_panningModel; }
    void setPanningModel(unsigned short, ExceptionCode&);

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
    void setDistanceModel(unsigned short, ExceptionCode&);

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

    void getAzimuthElevation(ContextRenderLock& r, double* outAzimuth, double* outElevation);
    float dopplerRate(ContextRenderLock& r);

    // Accessors for dynamically calculated gain values.
    std::shared_ptr<AudioParam> distanceGain() { return m_distanceGain; }
    std::shared_ptr<AudioParam> coneGain() { return m_coneGain; }

    virtual double tailTime() const override;
    virtual double latencyTime() const override;

protected:

    // Returns the combined distance and cone gain attenuation.
    virtual float distanceConeGain(ContextRenderLock& r);   /// @LabSound virtual

    // Notifies any AudioBufferSourceNodes connected to us either directly or indirectly about our existence.
    // This is in order to handle the pitch change necessary for the doppler shift.
    void notifyAudioSourcesConnectedToNode(ContextRenderLock& r, AudioNode*);

    std::unique_ptr<Panner> m_panner;
    unsigned m_panningModel;

    FloatPoint3D m_position;
    FloatPoint3D m_orientation;
    FloatPoint3D m_velocity;

    // Gain
    std::shared_ptr<AudioParam> m_distanceGain;
    std::shared_ptr<AudioParam> m_coneGain;

	DistanceEffect * m_distanceEffect;
    ConeEffect * m_coneEffect;
    float m_lastGain;

    unsigned m_connectionCount;
};

} // namespace WebCore

#endif // PannerNode_h
