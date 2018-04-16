
// License: BSD 2 Clause
// Copyright (C) 2018, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioListener.h"

namespace lab
{
    AudioListener::AudioListener()
    : m_dopplerFactor(std::make_shared<AudioParam>("dopplerFactor", 1.f, 0.01f, 100.f))
    , m_speedOfSound(std::make_shared<AudioParam>("speedOfSound", 343.f, 1.f, 10000.f))
    , m_upX(std::make_shared<AudioParam>("upX", 0.f, -1.f, 1.f))
    , m_upY(std::make_shared<AudioParam>("upY", 1.f, -1.f, 1.f))
    , m_upZ(std::make_shared<AudioParam>("upZ", 0.f, -1.f, 1.f))
    , m_forwardX(std::make_shared<AudioParam>("forwardX", 0.f, -1.f, 1.f))
    , m_forwardY(std::make_shared<AudioParam>("forwardY", 0.f, -1.f, 1.f))
    , m_forwardZ(std::make_shared<AudioParam>("forwardZ", -1.f, -1.f, 1.f))
    , m_velocityX(std::make_shared<AudioParam>("velocityX", 0.f, -1000.f, 1000.f))
    , m_velocityY(std::make_shared<AudioParam>("velocityY", 0.f, -1000.f, 1000.f))
    , m_velocityZ(std::make_shared<AudioParam>("velocityZ", 0.f, -1000.f, 1000.f))
    , m_positionX(std::make_shared<AudioParam>("positionX", 0.f, -1.e6f, 1.e6f))
    , m_positionY(std::make_shared<AudioParam>("positionY", 0.f, -1.e6f, 1.e6f))
    , m_positionZ(std::make_shared<AudioParam>("positionZ", 0.f, -1.e6f, 1.e6f))
    {
    }

    void AudioListener::setForward(const FloatPoint3D& fwd)
    {
        m_forwardX->setValue(fwd.x);
        m_forwardY->setValue(fwd.y);
        m_forwardZ->setValue(fwd.z);
    }

    void AudioListener::setUpVector(const FloatPoint3D &upVector) 
    { 
        m_upX->setValue(upVector.x);
        m_upY->setValue(upVector.y);
        m_upZ->setValue(upVector.z);
    }

    void AudioListener::setPosition(const FloatPoint3D &position)
    {
        m_positionX->setValue(position.x);
        m_positionY->setValue(position.y);
        m_positionY->setValue(position.z);
    }

    void AudioListener::setVelocity(const FloatPoint3D &velocity)
    {
        m_velocityX->setValue(velocity.x);
        m_velocityY->setValue(velocity.y);
        m_velocityZ->setValue(velocity.z);
    }

}
