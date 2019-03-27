// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/PannerNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"
#include "LabSound/core/SampledAudioNode.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/HRTFDatabaseLoader.h"
#include "internal/HRTFPanner.h"
#include "internal/Panner.h"
#include "internal/Cone.h"
#include "internal/Distance.h"
#include "internal/EqualPowerPanner.h"
#include "internal/Assertions.h"

using namespace std;

namespace lab {

template <typename T>
static void fixNANs(T& x)
{
    if (std::isnan(T(x)) || std::isinf(x)) x = T(0);
}

PannerNode::PannerNode(const float sampleRate, const std::string & searchPath)
: AudioNode()
, m_sampleRate(sampleRate)
, m_orientationX(std::make_shared<AudioParam>("orientationX", 0.f, -1.f, 1.f))
, m_orientationY(std::make_shared<AudioParam>("orientationY", 0.f, -1.f, 1.f))
, m_orientationZ(std::make_shared<AudioParam>("orientationZ", 0.f, -1.f, 1.f))
, m_velocityX(std::make_shared<AudioParam>("velocityX", 0.f, -1000.f, 1000.f))
, m_velocityY(std::make_shared<AudioParam>("velocityY", 0.f, -1000.f, 1000.f))
, m_velocityZ(std::make_shared<AudioParam>("velocityZ", 0.f, -1000.f, 1000.f))
, m_positionX(std::make_shared<AudioParam>("positionX", 0.f, -1.e6f, 1.e6f))
, m_positionY(std::make_shared<AudioParam>("positionY", 0.f, -1.e6f, 1.e6f))
, m_positionZ(std::make_shared<AudioParam>("positionZ", 0.f, -1.e6f, 1.e6f))
, m_distanceModel(std::make_shared<AudioSetting>("distanceModel"))
, m_refDistance(std::make_shared<AudioSetting>("refDistance"))
, m_maxDistance(std::make_shared<AudioSetting>("maxDistance"))
, m_rolloffFactor(std::make_shared<AudioSetting>("rolloffFactor"))
, m_coneInnerAngle(std::make_shared<AudioSetting>("coneInnerAngle"))
, m_coneOuterAngle(std::make_shared<AudioSetting>("coneOuterAngle"))
, m_panningModel(std::make_shared<AudioSetting>("panningMode"))
{
    if (searchPath.length())
    {
        auto stripSlash = [&](const std::string & path) -> std::string
        {
            if (path[path.size()-1] == '/' || path[path.size()-1] == '\\')
                return path.substr(0, path.size()-1);
            return path;
        };
        LOG("Initializing HRTF Database");
        m_hrtfDatabaseLoader = HRTFDatabaseLoader::MakeHRTFLoaderSingleton(sampleRate, stripSlash(searchPath));
    }

    m_distanceEffect.reset(new DistanceEffect());
    m_coneEffect.reset(new ConeEffect());

    addInput(unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 2)));

    m_distanceGain = std::make_shared<AudioParam>("distanceGain", 1.0, 0.0, 1.0);
    m_coneGain = std::make_shared<AudioParam>("coneGain", 1.0, 0.0, 1.0);

    m_params.push_back(m_distanceGain);
    m_params.push_back(m_coneGain);

    m_distanceModel->setValueChanged(
        [this]() {
            DistanceModel model(static_cast<DistanceModel>(m_distanceModel->valueUint32()));
            switch (model)
            {
                case DistanceEffect::ModelLinear:
                case DistanceEffect::ModelInverse:
                case DistanceEffect::ModelExponential:
                    m_distanceEffect->setModel(static_cast<DistanceEffect::ModelType>(model), true);
                    break;

                default:
                    throw std::invalid_argument("invalid distance model");
                    break;
            }
        }
    );
    m_settings.push_back(m_distanceModel);

    m_refDistance->setValueChanged(
        [this]() {
            m_distanceEffect->setRefDistance(m_refDistance->valueFloat());
        }
    );
    m_settings.push_back(m_refDistance);

    m_maxDistance->setValueChanged(
        [this]() {
            m_distanceEffect->setMaxDistance(m_maxDistance->valueFloat());
        }
    );
    m_settings.push_back(m_maxDistance);

    m_rolloffFactor->setValueChanged(
        [this]() {
            m_distanceEffect->setRolloffFactor(m_maxDistance->valueFloat());
        }
    );
    m_settings.push_back(m_rolloffFactor);

    m_coneInnerAngle->setValueChanged(
        [this]() {
            m_coneEffect->setInnerAngle(m_coneInnerAngle->valueFloat());
        }
    );
    m_settings.push_back(m_coneInnerAngle);

    m_coneOuterAngle->setValueChanged(
        [this]() {
            m_coneEffect->setOuterAngle(m_coneOuterAngle->valueFloat());
        }
    );
    m_settings.push_back(m_coneOuterAngle);

    m_panningModel->setUint32(static_cast<uint32_t>(EQUALPOWER));
    m_panningModel->setValueChanged(
        [this]() {
            setPanningModel(static_cast<PanningMode>(m_panningModel->valueUint32()));
        }
    );

    // Node-specific default mixing rules.
    m_channelCount = 2;
    m_channelCountMode = ChannelCountMode::ClampedMax;
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

    switch (static_cast<PanningMode>(m_panningModel->valueUint32()))
    {
        case PanningMode::EQUALPOWER:
            m_panner = std::unique_ptr<Panner>(new EqualPowerPanner(m_sampleRate));
            break;
        case PanningMode::HRTF:
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

void PannerNode::pullInputs(ContextRenderLock& r, size_t framesToProcess)
{
    // We override pullInputs(), so we can detect new AudioSourceNodes which have connected to us when new connections are made.
    // These AudioSourceNodes need to be made aware of our existence in order to handle doppler shift pitch changes.
    auto ac = r.context();

    if (!ac)
        return;

    AudioNode::pullInputs(r, framesToProcess);
}

void PannerNode::process(ContextRenderLock & r, size_t framesToProcess)
{
    AudioBus* destination = output(0)->bus(r);

    if (!isInitialized() || !input(0)->isConnected() || !m_panner.get())
    {
        destination->zero();
        return;
    }

    AudioBus* source = input(0)->bus(r);

    if (!source)
    {
        destination->zero();
        return;
    }

    // HRTFDatabase should be loaded before proceeding for offline audio context
    if (static_cast<PanningMode>(m_panningModel->valueUint32()) == PanningMode::HRTF && !m_hrtfDatabaseLoader->isLoaded())
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

    m_panner->pan(r, azimuth, elevation, source, destination, framesToProcess);

    // Get the distance and cone gain.
    float totalGain = distanceConeGain(r);

    // Snap to desired gain at the beginning.
    if (m_lastGain == -1.f)
        m_lastGain = totalGain;

    // Apply gain in-place with de-zippering.
    destination->copyWithGainFrom(*destination, &m_lastGain, totalGain);
}

void PannerNode::reset(ContextRenderLock&)
{
    m_lastGain = -1.0; // force to snap to initial gain
    if (m_panner.get())
        m_panner->reset();
}

void PannerNode::setPanningModel(PanningMode model)
{
    if (model != PanningMode::EQUALPOWER && model != PanningMode::HRTF)
        throw std::invalid_argument("Unknown panning model specified");

    PanningMode curr = static_cast<PanningMode>(m_panningModel->valueUint32());
    if (!m_panner.get() || model != curr)
    {
        m_panningModel->setUint32(static_cast<uint32_t>(model));

        switch (model)
        {
            case PanningMode::EQUALPOWER:
                m_panner = std::unique_ptr<Panner>(new EqualPowerPanner(m_sampleRate));
                break;
            case PanningMode::HRTF:
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

void PannerNode::getAzimuthElevation(ContextRenderLock& r, double* outAzimuth, double* outElevation)
{
    // FIXME: we should cache azimuth and elevation (if possible), so we only re-calculate if a change has been made.

    double azimuth = 0.0;

    AudioListener & listener = r.context()->listener();

    // Calculate the source-listener vector
    FloatPoint3D listenerPosition = {
                                        listener.positionX()->value(r),
                                        listener.positionY()->value(r),
                                        listener.positionZ()->value(r) };

    FloatPoint3D sourceListener = {
                                        positionX()->value(r),
                                        positionY()->value(r),
                                        positionZ()->value(r) };

    sourceListener = normalize(sourceListener - listenerPosition);

    if (is_zero(sourceListener))
    {
        // degenerate case if source and listener are at the same point
        *outAzimuth = 0.0;
        *outElevation = 0.0;
        return;
    }

    // Align axes
    FloatPoint3D listenerFront = normalize(FloatPoint3D{
                                        listener.forwardX()->value(r),
                                        listener.forwardY()->value(r),
                                        listener.forwardZ()->value(r) }
                                    );

    FloatPoint3D listenerUp = {
                                        listener.upX()->value(r),
                                        listener.upY()->value(r),
                                        listener.upZ()->value(r) };

    FloatPoint3D listenerRight = normalize(cross(listenerFront, listenerUp));
    FloatPoint3D up = cross(listenerRight, listenerFront);

    float upProjection = dot(sourceListener, up);

    FloatPoint3D projectedSource = normalize(sourceListener - upProjection * up);

    azimuth = 180.0 * acos(dot(projectedSource, listenerRight)) / piDouble;
    fixNANs(azimuth); // avoid illegal values

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
    double elevation = 90.0 - 180.0 * acos(dot(sourceListener, up)) / piDouble;
    fixNANs(elevation); // avoid illegal values

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

    AudioListener & listener = r.context()->listener();

    // FIXME: optimize for case when neither source nor listener has changed...
    double dopplerFactor = listener.dopplerFactor()->value(r);

    if (dopplerFactor > 0.0)
    {
        double speedOfSound = listener.speedOfSound()->value(r);

        const FloatPoint3D sourceVelocity = {
                                                    velocityX()->value(r),
                                                    velocityY()->value(r),
                                                    velocityZ()->value(r) };
        const FloatPoint3D listenerVelocity = {
                                                    listener.velocityX()->value(r),
                                                    listener.velocityY()->value(r),
                                                    listener.velocityZ()->value(r) };

        // Don't bother if both source and listener have no velocity
        bool sourceHasVelocity = !is_zero(sourceVelocity);
        bool listenerHasVelocity = !is_zero(listenerVelocity);

        if (sourceHasVelocity || listenerHasVelocity)
        {
            // Calculate the source to listener vector
            FloatPoint3D listenerPosition = {
                                                    listener.positionX()->value(r),
                                                    listener.positionY()->value(r),
                                                    listener.positionZ()->value(r) };

            FloatPoint3D sourceToListener = {
                                                    positionX()->value(r),
                                                    positionY()->value(r),
                                                    positionZ()->value(r) };

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
            fixNANs(dopplerShift); // avoid illegal values

            // Limit the pitch shifting to 4 octaves up and 3 octaves down.
            if (dopplerShift > 16.0)
                dopplerShift = 16.0;
            else if (dopplerShift < 0.125)
                dopplerShift = 0.125;
        }
    }

    return static_cast<float>(dopplerShift);
}

float PannerNode::distanceConeGain(ContextRenderLock& r)
{
    AudioListener & listener = r.context()->listener();

    FloatPoint3D listenerPosition = {
                                                    listener.positionX()->value(r),
                                                    listener.positionY()->value(r),
                                                    listener.positionZ()->value(r) };

    FloatPoint3D position = {
                                                    positionX()->value(r),
                                                    positionY()->value(r),
                                                    positionZ()->value(r) };


    double listenerDistance = magnitude(position - listenerPosition); // "distanceTo"

    double distanceGain = m_distanceEffect->gain(listenerDistance);

    m_distanceGain->setValue(static_cast<float>(distanceGain));

    // FIXME: could optimize by caching coneGain

    FloatPoint3D orientation = {
                                                    orientationX()->value(r),
                                                    orientationY()->value(r),
                                                    orientationZ()->value(r) };

    double coneGain = m_coneEffect->gain(position, orientation, listenerPosition);

    m_coneGain->setValue(static_cast<float>(coneGain));

    return float(distanceGain * coneGain);
}

void PannerNode::notifyAudioSourcesConnectedToNode(ContextRenderLock& r, AudioNode* node)
{
    ASSERT(node);
    if (!node)
        return;

    // First check if this node is an SampledAudioNode.
    if (auto bufferSourceNode = dynamic_cast<SampledAudioNode*>(node))
    {
        bufferSourceNode->setPannerNode(this);
    }
    else
    {
        // Go through all inputs to this node.
        for (unsigned i = 0; i < node->numberOfInputs(); ++i)
        {
            auto input = node->input(i);

            // For each input, go through all of its connections, looking for SampledAudioNodes.
            for (unsigned j = 0; j < input->numberOfRenderingConnections(r); ++j)
            {
                auto connectedOutput = input->renderingOutput(r, j);
                AudioNode* connectedNode = connectedOutput->node();
                notifyAudioSourcesConnectedToNode(r, connectedNode); // recurse
            }
        }
    }
}

float PannerNode::refDistance() { return static_cast<float>(m_distanceEffect->refDistance()); }
void PannerNode::setRefDistance(float refDistance) { m_refDistance->setFloat(refDistance); }

float PannerNode::maxDistance() { return static_cast<float>(m_distanceEffect->maxDistance()); }
void PannerNode::setMaxDistance(float maxDistance) { m_maxDistance->setFloat(maxDistance); }

float PannerNode::rolloffFactor() { return static_cast<float>(m_distanceEffect->rolloffFactor()); }
void PannerNode::setRolloffFactor(float rolloffFactor) { m_rolloffFactor->setFloat(rolloffFactor); }

float PannerNode::coneInnerAngle() const { return static_cast<float>(m_coneEffect->innerAngle()); }
void PannerNode::setConeInnerAngle(float angle) { m_coneInnerAngle->setFloat(angle); }

float PannerNode::coneOuterAngle() const { return static_cast<float>(m_coneEffect->outerAngle()); }
void PannerNode::setConeOuterAngle(float angle) { m_coneOuterAngle->setFloat(angle); }

float PannerNode::coneOuterGain() const { return static_cast<float>(m_coneEffect->outerGain()); }
void PannerNode::setConeOuterGain(float angle) { m_coneEffect->setOuterGain(angle); }

double PannerNode::tailTime(ContextRenderLock & r) const { return m_panner ? m_panner->tailTime(r) : 0; }
double PannerNode::latencyTime(ContextRenderLock & r) const { return m_panner ? m_panner->latencyTime(r) : 0; }

} // namespace lab
