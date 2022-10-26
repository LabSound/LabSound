// License: BSD 2 Clause
// Copyright (C) 2018, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioListener.h"

namespace lab
{

static AudioParamDescriptor s_dopplerFactorParam {"dopplerFactor", "DPLR",   1.f,     0.01f, 100.f};
static AudioParamDescriptor s_speedOfSoundParam  {"speedOfSound",  "SPED", 343.f,     1.f, 10000.f};
static AudioParamDescriptor s_forwardXParam      {"forwardX",      "FWDX",   0.f,    -1.f,     1.f};
static AudioParamDescriptor s_forwardYParam      {"forwardY",      "FWDY",   0.f,    -1.f,     1.f};
static AudioParamDescriptor s_forwardZParam      {"forwardZ",      "FWDZ",  -1.f,    -1.f,     1.f};
static AudioParamDescriptor s_upXParam           {"upX",           "UP X",   0.f,    -1.f,     1.f};
static AudioParamDescriptor s_upYParam           {"upY",           "UP Y",   1.f,    -1.f,     1.f};
static AudioParamDescriptor s_upZParam           {"upZ",           "UP Z",   0.f,    -1.f,     1.f};
static AudioParamDescriptor s_velocityXParam     {"velocityX",     "VELX",   0.f, -1000.f,  1000.f};
static AudioParamDescriptor s_velocityYParam     {"velocityY",     "VELY",   0.f, -1000.f,  1000.f};
static AudioParamDescriptor s_velocityZParam     {"velocityZ",     "VELZ",   0.f, -1000.f,  1000.f};
static AudioParamDescriptor s_positionXParam     {"positionX",     "POSX",   0.f,    -1.e6f,   1.e6f};
static AudioParamDescriptor s_positionYParam     {"positionY",     "POSY",   0.f,    -1.e6f,   1.e6f};
static AudioParamDescriptor s_positionZParam     {"positionZ",     "POSZ",   0.f,    -1.e6f,   1.e6f};


AudioListener::AudioListener()
    : m_dopplerFactor(std::make_shared<AudioParam>(&s_dopplerFactorParam))
    , m_speedOfSound(std::make_shared<AudioParam>(&s_speedOfSoundParam))
    , m_forwardX(std::make_shared<AudioParam>(&s_forwardXParam))
    , m_forwardY(std::make_shared<AudioParam>(&s_forwardYParam))
    , m_forwardZ(std::make_shared<AudioParam>(&s_forwardZParam))
    , m_upX(std::make_shared<AudioParam>(&s_upXParam))
    , m_upY(std::make_shared<AudioParam>(&s_upYParam))
    , m_upZ(std::make_shared<AudioParam>(&s_upZParam))
    , m_velocityX(std::make_shared<AudioParam>(&s_velocityXParam))
    , m_velocityY(std::make_shared<AudioParam>(&s_velocityYParam))
    , m_velocityZ(std::make_shared<AudioParam>(&s_velocityZParam))
    , m_positionX(std::make_shared<AudioParam>(&s_positionXParam))
    , m_positionY(std::make_shared<AudioParam>(&s_positionYParam))
    , m_positionZ(std::make_shared<AudioParam>(&s_positionZParam))
{
}

void AudioListener::setForward(const FloatPoint3D & fwd)
{
    m_forwardX->setValue(fwd.x);
    m_forwardY->setValue(fwd.y);
    m_forwardZ->setValue(fwd.z);
}

void AudioListener::setUpVector(const FloatPoint3D & upVector)
{
    m_upX->setValue(upVector.x);
    m_upY->setValue(upVector.y);
    m_upZ->setValue(upVector.z);
}

void AudioListener::setPosition(const FloatPoint3D & position)
{
    m_positionX->setValue(position.x);
    m_positionY->setValue(position.y);
    m_positionZ->setValue(position.z);
}

void AudioListener::setVelocity(const FloatPoint3D & velocity)
{
    m_velocityX->setValue(velocity.x);
    m_velocityY->setValue(velocity.y);
    m_velocityZ->setValue(velocity.z);
}
}
