// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioListener_h
#define AudioListener_h

#include "LabSound/core/FloatPoint3D.h"

namespace lab 
{

    // AudioListener maintains the state of the listener in the audio scene as defined in the OpenAL specification.
    class AudioListener 
    {
        // Position / Orientation
        FloatPoint3D m_position = {0, 0, 0};
        FloatPoint3D m_orientation {0, 0, 0};
        FloatPoint3D m_upVector = {0, 1, 0};

        FloatPoint3D m_velocity = {0, 0, 0};

        double m_dopplerFactor = 1.0;
        double m_speedOfSound = 343.3;

    public:

        AudioListener() {}

        // Position
        void setPosition(float x, float y, float z) { setPosition(FloatPoint3D(x, y, z)); }
        void setPosition(const FloatPoint3D &position) { m_position = position; }
        const FloatPoint3D & position() const { return m_position; }

        // Orientation
        void setOrientation(float x, float y, float z, float upX, float upY, float upZ)
        {
            setOrientation(FloatPoint3D(x, y, z));
            setUpVector(FloatPoint3D(upX, upY, upZ));
        }
        void setOrientation(const FloatPoint3D &orientation) { m_orientation = orientation; }
        const FloatPoint3D& orientation() const { return m_orientation; }

        // Up-vector
        void setUpVector(const FloatPoint3D &upVector) { m_upVector = upVector; }
        const FloatPoint3D& upVector() const { return m_upVector; }

        // Velocity
        void setVelocity(float x, float y, float z) { setVelocity(FloatPoint3D(x, y, z)); }
        void setVelocity(const FloatPoint3D &velocity) { m_velocity = velocity; }
        const FloatPoint3D& velocity() const { return m_velocity; }

        // Doppler factor
        void setDopplerFactor(double dopplerFactor) { m_dopplerFactor = dopplerFactor; }
        double dopplerFactor() const { return m_dopplerFactor; }

        // Speed of sound
        void setSpeedOfSound(double speedOfSound) { m_speedOfSound = speedOfSound; }
        double speedOfSound() const { return m_speedOfSound; }
    };

} // lab

#endif // AudioListener_h

