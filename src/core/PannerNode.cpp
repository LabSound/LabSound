// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/PannerNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"
#include "LabSound/core/SampledAudioNode.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"
#include "internal/Cone.h"
#include "internal/Distance.h"
#include "internal/EqualPowerPanner.h"
#include "internal/HRTFDatabaseLoader.h"
#include "internal/HRTFPanner.h"
#include "internal/Panner.h"

using namespace std;

namespace lab
{

template <typename T>
static void fixNANs(T & x)
{
    if (std::isnan(T(x)) || std::isinf(x))
        x = T(0);
}


static char const * const s_distance_models[lab::DistanceEffect::ModelType::_Count + 1] = {
    "Linear", "Inverse", "Exponential", nullptr};
static char const * const s_panning_models[lab::PanningModel::_PanningModeCount + 1] = {
    "Linear", "Inverse", "Exponential", nullptr};

static AudioParamDescriptor s_pDesc[] = {
    {"orientationX", "OR X", 0.f,    -1.f,    1.f},
    {"orientationY", "OR Y", 0.f,    -1.f,    1.f},
    {"orientationZ", "OR Z", 0.f,    -1.f,    1.f},
    {"velocityX",    "VELX", 0.f, -1000.f, 1000.f},
    {"velocityY",    "VELY", 0.f, -1000.f, 1000.f},
    {"velocityZ",    "VELZ", 0.f, -1000.f, 1000.f},
    {"positionX",    "POSX", 0.f,    -1.e6f,  1.e6f},
    {"positionY",    "POSY", 0.f,    -1.e6f,  1.e6f},
    {"positionZ",    "POSZ", 0.f,    -1.e6f,  1.e6f},
    {"distanceGain", "DGAN", 1.0,     0.0,    1.0},
    {"coneGain",     "CGAN", 1.0,     0.0,    1.0},
    nullptr};

static AudioSettingDescriptor s_sDesc[] = {
    {"distanceModel",  "DSTM", SettingType::Enum, s_distance_models},
    {"refDistance",    "REFD", SettingType::Float},
    {"maxDistance",    "MAXD", SettingType::Float},
    {"rolloffFactor",  "ROLL", SettingType::Float},
    {"coneInnerAngle", "CONI", SettingType::Float},
    {"coneOuterAngle", "CONO", SettingType::Float},
    {"panningMode",    "PANM", SettingType::Enum, s_panning_models},
    nullptr};

AudioNodeDescriptor * PannerNode::desc()
{
    static AudioNodeDescriptor d {s_pDesc, s_sDesc};
    return &d;
}

PannerNode::PannerNode(AudioContext & ac, const std::string & searchPath)
    : AudioNode(ac, *desc())
    , m_sampleRate(ac.sampleRate())
{

    /// @TODO in the future a panner could be multi-channel beyond stereo
    addInput("in");
    addOutput("out", 2, AudioNode::ProcessingSizeInFrames);

    m_orientationX = param("orientationX");
    m_orientationY = param("orientationY");
    m_orientationZ = param("orientationZ");
    m_velocityX = param("velocityX");
    m_velocityY = param("velocityY");
    m_velocityZ = param("velocityZ");
    m_positionX = param("positionX");
    m_positionY = param("positionY");
    m_positionZ = param("positionZ");
    m_distanceGain = param("distanceGain");
    m_coneGain = param("coneGain");
    m_distanceModel = setting("distanceModel");
    m_refDistance = setting("refDistance");
    m_maxDistance = setting("maxDistance");
    m_rolloffFactor = setting("rolloffFactor");
    m_coneInnerAngle = setting("coneInnerAngle");
    m_coneOuterAngle = setting("coneOuterAngle");
    m_panningModel = setting("panningMode");

    if (searchPath.length())
    {
        auto stripSlash = [&](const std::string & path) -> std::string {
            if (path[path.size() - 1] == '/' || path[path.size() - 1] == '\\')
                return path.substr(0, path.size() - 1);
            return path;
        };
        LOG_INFO("Initializing HRTF Database");
        m_hrtfDatabaseLoader = HRTFDatabaseLoader::MakeHRTFLoaderSingleton(m_sampleRate, stripSlash(searchPath));
    }

    m_distanceEffect.reset(new DistanceEffect());
    m_coneEffect.reset(new ConeEffect());

    m_distanceModel->setValueChanged(
        [this]() {
            DistanceEffect::ModelType model(static_cast<DistanceEffect::ModelType>(m_distanceModel->valueUint32()));
            switch (model)
            {
                case DistanceEffect::ModelLinear:
                case DistanceEffect::ModelInverse:
                case DistanceEffect::ModelExponential:
                    m_distanceEffect->setModel(model, true);
                    break;

                default:
                    throw std::invalid_argument("invalid distance model");
                    break;
            }
        });

    m_refDistance->setValueChanged(
        [this]() {
            m_distanceEffect->setRefDistance(m_refDistance->valueFloat());
        });

    m_maxDistance->setValueChanged(
        [this]() {
            m_distanceEffect->setMaxDistance(m_maxDistance->valueFloat());
        });

    m_rolloffFactor->setValueChanged(
        [this]() {
            m_distanceEffect->setRolloffFactor(m_maxDistance->valueFloat());
        });

    m_coneInnerAngle->setValueChanged(
        [this]() {
            m_coneEffect->setInnerAngle(m_coneInnerAngle->valueFloat());
        });

    m_coneOuterAngle->setValueChanged(
        [this]() {
            m_coneEffect->setOuterAngle(m_coneOuterAngle->valueFloat());
        });

    m_panningModel->setUint32(static_cast<uint32_t>(EQUALPOWER));
    m_panningModel->setValueChanged(
        [this]() {
            setPanningModel(static_cast<PanningModel>(m_panningModel->valueUint32()));
        });

    // Node-specific default mixing rules.
    m_channelInterpretation = ChannelInterpretation::Speakers;

    initialize();
}

PannerNode::~PannerNode()
{
    uninitialize();
}

void PannerNode::initialize()
{
    if (isInitialized()) return;

    switch (static_cast<PanningModel>(m_panningModel->valueUint32()))
    {
        case PanningModel::EQUALPOWER:
            m_panner = std::unique_ptr<Panner>(new EqualPowerPanner(m_sampleRate));
            break;
        case PanningModel::HRTF:
            m_panner = std::unique_ptr<Panner>(new HRTFPanner(m_sampleRate));
            break;
        default:
            throw std::runtime_error("invalid panning model");
    }

    AudioNode::initialize();
}

void PannerNode::uninitialize()
{
    if (!isInitialized())
        return;

    m_panner.reset();

    AudioNode::uninitialize();
}

void PannerNode::setOrientation(const FloatPoint3D & fwd)
{
    m_orientationX->setValue(fwd.x);
    m_orientationY->setValue(fwd.y);
    m_orientationZ->setValue(fwd.z);
}

void PannerNode::setPosition(const FloatPoint3D & position)
{
    m_positionX->setValue(position.x);
    m_positionY->setValue(position.y);
    m_positionZ->setValue(position.z);
}

void PannerNode::setVelocity(const FloatPoint3D & velocity)
{
    m_velocityX->setValue(velocity.x);
    m_velocityY->setValue(velocity.y);
    m_velocityZ->setValue(velocity.z);
}

void PannerNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * destination = outputBus(r, 0);
    AudioBus * source = inputBus(r, 0);

    if (!destination || !source || !m_panner.get())
    {
        if (destination)
            destination->zero();
        return;
    }

    // HRTFDatabase should be loaded before proceeding for offline audio context
    if (static_cast<PanningModel>(m_panningModel->valueUint32()) == PanningModel::HRTF && !m_hrtfDatabaseLoader->isLoaded())
    {
        if (r.context()->isOfflineContext())
        {
            m_hrtfDatabaseLoader->waitForLoaderThreadCompletion();
        }
        else
        {
            destination->zero();
            return;
        }
    }

    // Apply the panning effect.
    double azimuth;
    double elevation;
    getAzimuthElevation(r, &azimuth, &elevation);

    m_panner->pan(r, azimuth, elevation, source + _scheduler._renderOffset, destination + _scheduler._renderOffset, _scheduler._renderLength);

    // Get the distance and cone gain.
    float totalGain = distanceConeGain(r);

    // Snap to desired gain at the beginning.
    if (m_lastGain == -1.f)
        m_lastGain = totalGain;

    // Apply gain in-place with de-zippering.
    destination->copyWithGainFrom(*destination, &m_lastGain, totalGain);
}

void PannerNode::reset(ContextRenderLock &)
{
    m_lastGain = -1.0;  // force to snap to initial gain
    if (m_panner.get())
        m_panner->reset();
}

PanningModel PannerNode::panningModel() const
{
    return static_cast<PanningModel>(m_panningModel->valueUint32());
}

void PannerNode::setPanningModel(PanningModel model)
{
    if (model != PanningModel::EQUALPOWER && model != PanningModel::HRTF)
        throw std::invalid_argument("Unknown panning model specified");

    ASSERT(m_sampleRate);

    PanningModel curr = static_cast<PanningModel>(m_panningModel->valueUint32());
    if (!m_panner.get() || model != curr)
    {
        m_panningModel->setUint32(static_cast<uint32_t>(model));

        switch (model)
        {
            case PanningModel::EQUALPOWER:
                m_panner = std::unique_ptr<Panner>(new EqualPowerPanner(m_sampleRate));
                break;
            case PanningModel::HRTF:
                m_panner = std::unique_ptr<Panner>(new HRTFPanner(m_sampleRate));
                break;
            default:
                throw std::invalid_argument("invalid panning model");
        }
    }
}

void PannerNode::setDistanceModel(DistanceModel model)
{
    m_distanceModel->setUint32(static_cast<uint32_t>(model));
}
PannerNode::DistanceModel PannerNode::distanceModel()
{
    return static_cast<PannerNode::DistanceModel>(m_distanceModel->valueUint32());
}

void PannerNode::getAzimuthElevation(ContextRenderLock & r, double * outAzimuth, double * outElevation)
{
    // FIXME: we should cache azimuth and elevation (if possible), so we only re-calculate if a change has been made.

    double azimuth = 0.0;

    auto listener = r.context()->listener();

    // Calculate the source-listener vector
    /// @fixme these values should be per sample, not per quantum
    FloatPoint3D listenerPosition = {
        listener->positionX()->value(),
        listener->positionY()->value(),
        listener->positionZ()->value()};

    /// @fixme these values should be per sample, not per quantum
    FloatPoint3D sourceListener = {
        positionX()->value(),
        positionY()->value(),
        positionZ()->value()};

    sourceListener = normalize(sourceListener - listenerPosition);

    if (is_zero(sourceListener))
    {
        // degenerate case if source and listener are at the same point
        *outAzimuth = 0.0;
        *outElevation = 0.0;
        return;
    }

    // Align axes
    /// @fixme these values should be per sample, not per quantum
    FloatPoint3D listenerFront = normalize(FloatPoint3D{
        listener->forwardX()->value(),
        listener->forwardY()->value(),
        listener->forwardZ()->value()});

    /// @fixme these values should be per sample, not per quantum
    FloatPoint3D listenerUp = {
        listener->upX()->value(),
        listener->upY()->value(),
        listener->upZ()->value()};

    FloatPoint3D listenerRight = normalize(cross(listenerFront, listenerUp));
    FloatPoint3D up = cross(listenerRight, listenerFront);

    float upProjection = dot(sourceListener, up);

    FloatPoint3D projectedSource = normalize(sourceListener - upProjection * up);

    azimuth = 180.0 * acos(dot(projectedSource, listenerRight)) / static_cast<double>(LAB_PI);
    fixNANs(azimuth);  // avoid illegal values

    // Source in front or behind the listener
    double frontBack = dot(projectedSource, listenerFront);
    if (frontBack < 0.0)
        azimuth = 360.0 - azimuth;

    // Make azimuth relative to "front" and not "right" listener vector
    if ((azimuth >= 0.0) && (azimuth <= 270.0))
        azimuth = 90.0 - azimuth;
    else
        azimuth = 450.0 - azimuth;

    // Elevation
    double elevation = 90.0 - 180.0 * acos(dot(sourceListener, up)) / static_cast<double>(LAB_PI);
    fixNANs(elevation);  // avoid illegal values

    if (elevation > 90.0)
        elevation = 180.0 - elevation;
    else if (elevation < -90.0)
        elevation = -180.0 - elevation;

    if (outAzimuth)
        *outAzimuth = azimuth;
    if (outElevation)
        *outElevation = elevation;
}

float PannerNode::dopplerRate(ContextRenderLock & r)
{
    double dopplerShift = 1.0;

    auto listener = r.context()->listener();

    // FIXME: optimize for case when neither source nor listener has changed...
    /// @fixme these values should be per sample, not per quantum
    double dopplerFactor = listener->dopplerFactor()->value();

    if (dopplerFactor > 0.0)
    {
        /// @fixme should be a setting
        double speedOfSound = listener->speedOfSound()->value();

        /// @fixme these values should be per sample, not per quantum
        const FloatPoint3D sourceVelocity = {
            velocityX()->value(),
            velocityY()->value(),
            velocityZ()->value()};
        /// @fixme these values should be per sample, not per quantum
        const FloatPoint3D listenerVelocity = {
            listener->velocityX()->value(),
            listener->velocityY()->value(),
            listener->velocityZ()->value()};

        // Don't bother if both source and listener have no velocity
        bool sourceHasVelocity = !is_zero(sourceVelocity);
        bool listenerHasVelocity = !is_zero(listenerVelocity);

        if (sourceHasVelocity || listenerHasVelocity)
        {
            // Calculate the source to listener vector
            /// @fixme these values should be per sample, not per quantum
            FloatPoint3D listenerPosition = {
                listener->positionX()->value(),
                listener->positionY()->value(),
                listener->positionZ()->value()};

            /// @fixme these values should be per sample, not per quantum
            FloatPoint3D sourceToListener = {
                positionX()->value(),
                positionY()->value(),
                positionZ()->value()};

            sourceToListener = sourceToListener - listenerPosition;

            double sourceListenerMagnitude = magnitude(sourceToListener);

            double listenerProjection = dot(sourceToListener, listenerVelocity) / sourceListenerMagnitude;
            double sourceProjection = dot(sourceToListener, sourceVelocity) / sourceListenerMagnitude;

            listenerProjection = -listenerProjection;
            sourceProjection = -sourceProjection;

            double scaledSpeedOfSound = speedOfSound / dopplerFactor;
            listenerProjection = min(listenerProjection, scaledSpeedOfSound);
            sourceProjection = min(sourceProjection, scaledSpeedOfSound);

            dopplerShift = ((speedOfSound - dopplerFactor * listenerProjection) / (speedOfSound - dopplerFactor * sourceProjection));
            fixNANs(dopplerShift);  // avoid illegal values

            // Limit the pitch shifting to 4 octaves up and 3 octaves down.
            if (dopplerShift > 16.0)
                dopplerShift = 16.0;
            else if (dopplerShift < 0.125)
                dopplerShift = 0.125;
        }
    }

    return static_cast<float>(dopplerShift);
}

float PannerNode::distanceConeGain(ContextRenderLock & r)
{
    auto listener = r.context()->listener();

    /// @fixme these values should be per sample, not per quantum
    FloatPoint3D listenerPosition = {
        listener->positionX()->value(),
        listener->positionY()->value(),
        listener->positionZ()->value()};

    /// @fixme these values should be per sample, not per quantum
    FloatPoint3D position = {
        positionX()->value(),
        positionY()->value(),
        positionZ()->value()};

    double listenerDistance = magnitude(position - listenerPosition);  // "distanceTo"

    double distanceGain = m_distanceEffect->gain(listenerDistance);

    m_distanceGain->setValue(static_cast<float>(distanceGain));

    // FIXME: could optimize by caching coneGain

    /// @fixme these values should be per sample, not per quantum
    FloatPoint3D orientation = {
        orientationX()->value(),
        orientationY()->value(),
        orientationZ()->value()};

    double coneGain = m_coneEffect->gain(position, orientation, listenerPosition);

    m_coneGain->setValue(static_cast<float>(coneGain));

    return float(distanceGain * coneGain);
}

float PannerNode::refDistance()
{
    return static_cast<float>(m_distanceEffect->refDistance());
}
void PannerNode::setRefDistance(float refDistance)
{
    m_refDistance->setFloat(refDistance);
}

float PannerNode::maxDistance()
{
    return static_cast<float>(m_distanceEffect->maxDistance());
}
void PannerNode::setMaxDistance(float maxDistance)
{
    m_maxDistance->setFloat(maxDistance);
}

float PannerNode::rolloffFactor()
{
    return static_cast<float>(m_distanceEffect->rolloffFactor());
}
void PannerNode::setRolloffFactor(float rolloffFactor)
{
    m_rolloffFactor->setFloat(rolloffFactor);
}

float PannerNode::coneInnerAngle() const
{
    return static_cast<float>(m_coneEffect->innerAngle());
}
void PannerNode::setConeInnerAngle(float angle)
{
    m_coneInnerAngle->setFloat(angle);
}

float PannerNode::coneOuterAngle() const
{
    return static_cast<float>(m_coneEffect->outerAngle());
}
void PannerNode::setConeOuterAngle(float angle)
{
    m_coneOuterAngle->setFloat(angle);
}

float PannerNode::coneOuterGain() const
{
    return static_cast<float>(m_coneEffect->outerGain());
}
void PannerNode::setConeOuterGain(float angle)
{
    m_coneEffect->setOuterGain(angle);
}

double PannerNode::tailTime(ContextRenderLock & r) const
{
    return m_panner ? m_panner->tailTime(r) : 0;
}
double PannerNode::latencyTime(ContextRenderLock & r) const
{
    return m_panner ? m_panner->latencyTime(r) : 0;
}

}  // namespace lab
