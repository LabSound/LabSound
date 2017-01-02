// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/PannerNode.h"
#include "LabSound/core/AudioBufferSourceNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/HRTFDatabaseLoader.h"
#include "internal/HRTFPanner.h"
#include "internal/AudioBus.h"
#include "internal/Panner.h"
#include "internal/Cone.h"
#include "internal/Distance.h"
#include "internal/EqualPowerPanner.h"

#include <WTF/MathExtras.h>

using namespace std;

namespace lab {

static void fixNANs(double & x)
{
    if (std::isnan(double(x)) || std::isinf(x)) x = 0.0;
}

PannerNode::PannerNode(float sampleRate, const std::string & searchPath) : AudioNode(sampleRate), m_panningModel(PanningMode::EQUALPOWER)
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

    m_position = FloatPoint3D(0, 0, 0);
    m_orientation = FloatPoint3D(0, 0, 0);
    m_velocity = FloatPoint3D(0, 0, 0);

    // Node-specific default mixing rules.
    m_channelCount = 2;
    m_channelCountMode = ChannelCountMode::ClampedMax;
    m_channelInterpretation = ChannelInterpretation::Speakers;

    setNodeType(NodeTypePanner);

    initialize();
}

PannerNode::~PannerNode()
{
    uninitialize();
}

void PannerNode::initialize()
{
    if (isInitialized()) return;

    switch (m_panningModel)
    {
        case PanningMode::EQUALPOWER:
            m_panner = std::unique_ptr<Panner>(new EqualPowerPanner(sampleRate()));
            break;
        case PanningMode::HRTF:
            m_panner = std::unique_ptr<Panner>(new HRTFPanner(sampleRate()));
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

void PannerNode::pullInputs(ContextRenderLock& r, size_t framesToProcess)
{
    // We override pullInputs(), so we can detect new AudioSourceNodes which have connected to us when new connections are made.
    // These AudioSourceNodes need to be made aware of our existence in order to handle doppler shift pitch changes.
    auto ac = r.context();

    if (!ac)
        return;

    if (m_connectionCount != ac->connectionCount())
    {
        m_connectionCount = ac->connectionCount();
        // Recursively go through all nodes connected to us.
        // notifyAudioSourcesConnectedToNode(r, this); // @tofix - dimitri commented out
    }

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
    if (m_panningModel == PanningMode::HRTF && !m_hrtfDatabaseLoader->isLoaded())
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
    double totalGain = distanceConeGain(r);

    // Snap to desired gain at the beginning.
    if (m_lastGain == -1.0)
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

std::shared_ptr<AudioListener> PannerNode::listener(ContextRenderLock& r)
{
    if (!r.context())
        return nullptr;

    return r.context()->listener();
}

void PannerNode::setPanningModel(PanningMode model)
{
    if (model != PanningMode::EQUALPOWER && model != PanningMode::HRTF)
        throw std::invalid_argument("Unknown panning model specified");

    if (!m_panner.get() || model != m_panningModel)
    {
        m_panningModel = model;

        switch (m_panningModel)
        {
            case PanningMode::EQUALPOWER:
                m_panner = std::unique_ptr<Panner>(new EqualPowerPanner(sampleRate()));
                break;
            case PanningMode::HRTF:
                m_panner = std::unique_ptr<Panner>(new HRTFPanner(sampleRate()));
                break;
            default:
                throw std::invalid_argument("invalid panning model");
        }

    }

}

void PannerNode::setDistanceModel(unsigned short model)
{
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

void PannerNode::getAzimuthElevation(ContextRenderLock& r, double* outAzimuth, double* outElevation)
{
    // FIXME: we should cache azimuth and elevation (if possible), so we only re-calculate if a change has been made.

    double azimuth = 0.0;

    // Calculate the source-listener vector
    FloatPoint3D listenerPosition = listener(r)->position();
    FloatPoint3D sourceListener = m_position - listenerPosition;

    if (is_zero(sourceListener))
    {
        // degenerate case if source and listener are at the same point
        *outAzimuth = 0.0;
        *outElevation = 0.0;
        return;
    }

    sourceListener = normalize(sourceListener);

    // Align axes
    FloatPoint3D listenerFront = normalize(listener(r)->orientation());
    FloatPoint3D listenerUp = listener(r)->upVector();
    FloatPoint3D listenerRight = normalize(cross(listenerFront, listenerUp));

    FloatPoint3D up = cross(listenerRight, listenerFront);

    float upProjection = dot(sourceListener, up);

    FloatPoint3D projectedSource = sourceListener - upProjection * up;
    projectedSource = normalize(projectedSource);

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

float PannerNode::dopplerRate(ContextRenderLock& r)
{
    double dopplerShift = 1.0;

    // FIXME: optimize for case when neither source nor listener has changed...
    double dopplerFactor = listener(r)->dopplerFactor();

    if (dopplerFactor > 0.0)
    {
        double speedOfSound = listener(r)->speedOfSound();

        const FloatPoint3D &sourceVelocity = m_velocity;
        const FloatPoint3D &listenerVelocity = listener(r)->velocity();

        // Don't bother if both source and listener have no velocity
        bool sourceHasVelocity = !is_zero(sourceVelocity);
        bool listenerHasVelocity = !is_zero(listenerVelocity);

        if (sourceHasVelocity || listenerHasVelocity)
        {
            // Calculate the source to listener vector
            FloatPoint3D listenerPosition = listener(r)->position();
            FloatPoint3D sourceToListener = m_position - listenerPosition;

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
    FloatPoint3D listenerPosition = listener(r)->position();

    double listenerDistance = magnitude(m_position - listenerPosition); // "distanceTo"

    double distanceGain = m_distanceEffect->gain(listenerDistance);

    m_distanceGain->setValue(static_cast<float>(distanceGain));

    // FIXME: could optimize by caching coneGain
    double coneGain = m_coneEffect->gain(m_position, m_orientation, listenerPosition);

    m_coneGain->setValue(static_cast<float>(coneGain));

    return float(distanceGain * coneGain);
}

void PannerNode::notifyAudioSourcesConnectedToNode(ContextRenderLock& r, AudioNode* node)
{
    ASSERT(node);
    if (!node)
        return;

    // First check if this node is an AudioBufferSourceNode. If so, let it know about us so that doppler shift pitch can be taken into account.
    if (node->nodeType() == NodeTypeAudioBufferSource)
    {
        AudioBufferSourceNode* bufferSourceNode = reinterpret_cast<AudioBufferSourceNode*>(node);
        bufferSourceNode->setPannerNode(this);
    }
    else
    {
        // Go through all inputs to this node.
        for (unsigned i = 0; i < node->numberOfInputs(); ++i)
        {
            auto input = node->input(i);

            // For each input, go through all of its connections, looking for AudioBufferSourceNodes.
            for (unsigned j = 0; j < input->numberOfRenderingConnections(r); ++j)
            {
                auto connectedOutput = input->renderingOutput(r, j);
                AudioNode* connectedNode = connectedOutput->node();
                notifyAudioSourcesConnectedToNode(r, connectedNode); // recurse
            }
        }
    }
}

unsigned short PannerNode::distanceModel() { return m_distanceEffect->model(); }
float PannerNode::refDistance() { return static_cast<float>(m_distanceEffect->refDistance()); }

void PannerNode::setRefDistance(float refDistance) { m_distanceEffect->setRefDistance(refDistance); }

float PannerNode::maxDistance() { return static_cast<float>(m_distanceEffect->maxDistance()); }
void PannerNode::setMaxDistance(float maxDistance) { m_distanceEffect->setMaxDistance(maxDistance); }

float PannerNode::rolloffFactor() { return static_cast<float>(m_distanceEffect->rolloffFactor()); }
void PannerNode::setRolloffFactor(float rolloffFactor) { m_distanceEffect->setRolloffFactor(rolloffFactor); }

float PannerNode::coneInnerAngle() const { return static_cast<float>(m_coneEffect->innerAngle()); }
void PannerNode::setConeInnerAngle(float angle) { m_coneEffect->setInnerAngle(angle); }

float PannerNode::coneOuterAngle() const { return static_cast<float>(m_coneEffect->outerAngle()); }
void PannerNode::setConeOuterAngle(float angle) { m_coneEffect->setOuterAngle(angle); }

float PannerNode::coneOuterGain() const { return static_cast<float>(m_coneEffect->outerGain()); }
void PannerNode::setConeOuterGain(float angle) { m_coneEffect->setOuterGain(angle); }

double PannerNode::tailTime() const { return m_panner ? m_panner->tailTime() : 0; }
double PannerNode::latencyTime() const { return m_panner ? m_panner->latencyTime() : 0; }

} // namespace lab
