// License: The MIT License
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.
// Copyright (C) 2013, Two Big Ears Ltd (http://twobigears.com)

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/PeakCompNode.h"
#include "LabSound/extended/VectorMath.h"

#include "LabSound/core/Macros.h"

#include <algorithm>
#include <vector>

using namespace lab;

namespace lab
{

/////////////////////////////////////////
// Prviate PeakCompNode Implementation //
/////////////////////////////////////////

static AudioParamDescriptor s_pcParams[] = {
    {"threshold", "THRS", 0.0,   0, -1e6},
    {"ratio",     "RATI", 1.0,   0,   10},
    {"attack",    "ATCK", 0.001, 0, 1000},
    {"release",   "RELS", 0.001, 0, 1000},
    {"makeup",    "MAKE", 0.0,   0,   60},
    {"knee",      "KNEE", 0.0,   0,    1}, nullptr };

    
AudioNodeDescriptor * PeakCompNode::desc()
{
    static AudioNodeDescriptor d {s_pcParams, nullptr, 1};
    return &d;
}


class PeakCompNode::PeakCompNodeInternal : public AudioProcessor
{
public:
    PeakCompNodeInternal() : AudioProcessor()
    {
        for (int i = 0; i < 2; i++)
        {
            kneeRecursive[i] = 0.f;
            attackRecursive[i] = 0.f;
            releaseRecursive[i] = 0.f;
        }
    }

    virtual ~PeakCompNodeInternal() {}

    virtual void initialize() override {}

    virtual void uninitialize() override {}

    // Processes the source to destination bus.  The number of channels must match in source and destination.
    virtual void process(ContextRenderLock & r, const lab::AudioBus * sourceBus, lab::AudioBus * destinationBus, int framesToProcess) override
    {
        int sourceNumChannels = sourceBus->numberOfChannels();
        int destNumChannels = destinationBus->numberOfChannels();

        if (!destNumChannels)
            return;

        // Get sample rate
        internalSampleRate = r.context()->sampleRate();
        oneOverSampleRate = 1.0 / internalSampleRate;

        // copy attributes to run time variables
        /// @fixme these values should be per sample, not per quantum
        /// -or- they should be settings if they don't vary per sample
        float v = m_threshold->value();
        if (v <= 0)
        {
            // dB to linear (could use the function from m_pd.h)
            threshold = powf(10.f, (v * 0.05f));
        }
        else
            threshold = 0;

        /// @fixme these values should be per sample, not per quantum
        /// -or- they should be settings if they don't vary per sample
        v = m_ratio->value();
        if (v >= 1)
        {
            ratio = 1.f / v;
        }
        else
            ratio = 1;

        /// @fixme these values should be per sample, not per quantum
        /// -or- they should be settings if they don't vary per sample
        v = m_attack->value();
        if (v >= 0.001f)
        {
            attack = v * 0.001;
        }
        else
            attack = 0.000001;

        /// @fixme these values should be per sample, not per quantum
        /// -or- they should be settings if they don't vary per sample
        v = m_release->value();
        if (v >= 0.001f)
        {
            release = v * 0.001;
        }
        else
            release = 0.000001;

        /// @fixme these values should be per sample, not per quantum
        /// -or- they should be settings if they don't vary per sample
        v = m_makeup->value();
        // dB to linear (could use the function from m_pd.h)
        makeupGain = powf(10.f, (v * 0.05f));

        /// @fixme these values should be per sample, not per quantum
        /// -or- they should be settings if they don't vary per sample
        v = m_knee->value();
        if (v >= 0 && v <= 1)
        {
            // knee value (0 to 1) is scaled from 0 (hard) to 0.02 (smooth). Could be scaled to a larger number.
            knee = v * 0.02;
        }

        // calc coefficients from run time vars
        kneeCoeffs = exp(0. - (oneOverSampleRate / knee));
        kneeCoeffsMinus = 1. - kneeCoeffs;

        attackCoeffs = exp(0. - (oneOverSampleRate / attack));
        attackCoeffsMinus = 1. - attackCoeffs;

        releaseCoeff = exp(0. - (oneOverSampleRate / release));
        releaseCoeffMinus = 1. - releaseCoeff;

        // Handle both the 1 -> N and N -> N case here.
        const float * source[16];
        for (int i = 0; i < sourceNumChannels; ++i)
        {
            if (sourceBus->numberOfChannels() == sourceNumChannels)
                source[i] = sourceBus->channel(i)->data();
            else
                source[i] = sourceBus->channel(0)->data();
        }

        float * dest[16];
        for (int i = 0; i < destNumChannels; ++i)
            dest[i] = destinationBus->channel(i)->mutableData();

        for (int i = 0; i < framesToProcess; ++i)
        {
            float peakEnv = 0;
            for (int j = 0; j < sourceNumChannels; ++j)
            {
                peakEnv += source[j][i];
            }
            // Release recursive
            releaseRecursive[0] = (releaseCoeffMinus * peakEnv) + (releaseCoeff * std::max(peakEnv, float(releaseRecursive[1])));

            // Attack recursive
            attackRecursive[0] = ((attackCoeffsMinus * releaseRecursive[0]) + (attackCoeffs * attackRecursive[1])) + 0.000001f;  // avoid div by 0

            // Knee smoothening and gain reduction
            kneeRecursive[0] = (kneeCoeffsMinus * std::max(std::min(((threshold + (ratio * (attackRecursive[0] - threshold))) / attackRecursive[0]), 1.), 0.)) + (kneeCoeffs * kneeRecursive[1]);

            int k = std::min(sourceNumChannels, destNumChannels);
            for (int j = 0; j < k; ++j)
            {
                dest[j][i] = static_cast<float>(source[j][i] * kneeRecursive[0] * makeupGain);
            }
        }

        releaseRecursive[1] = releaseRecursive[0];
        attackRecursive[1] = attackRecursive[0];
        kneeRecursive[1] = kneeRecursive[0];
    }

    float internalSampleRate = 44000.f;
    double oneOverSampleRate = 1.0 / 44000.f;

    // Arrays for delay lines
    double kneeRecursive[2];
    double attackRecursive[2];
    double releaseRecursive[2];

    double attack = 0.;
    double release = 0.;
    double ratio = 1.;
    double threshold = 0.;

    double knee = 0;
    double kneeCoeffs = 0;
    double kneeCoeffsMinus = 0;

    double attackCoeffs = 0;
    double attackCoeffsMinus = 0;

    double releaseCoeff = 0;
    double releaseCoeffMinus = 0;

    double makeupGain = 0;

    // Resets filter state
    virtual void reset() override
    { 
        // @tofix
    }

    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    std::shared_ptr<AudioParam> m_threshold;
    std::shared_ptr<AudioParam> m_ratio;
    std::shared_ptr<AudioParam> m_attack;
    std::shared_ptr<AudioParam> m_release;
    std::shared_ptr<AudioParam> m_makeup;
    std::shared_ptr<AudioParam> m_knee;
};

std::shared_ptr<AudioParam> PeakCompNode::threshold() const
{
    return internalNode->m_threshold;
}

std::shared_ptr<AudioParam> PeakCompNode::ratio() const
{
    return internalNode->m_ratio;
}

std::shared_ptr<AudioParam> PeakCompNode::attack() const
{
    return internalNode->m_attack;
}

std::shared_ptr<AudioParam> PeakCompNode::release() const
{
    return internalNode->m_release;
}

std::shared_ptr<AudioParam> PeakCompNode::makeup() const
{
    return internalNode->m_makeup;
}

std::shared_ptr<AudioParam> PeakCompNode::knee() const
{
    return internalNode->m_knee;
}

/////////////////////////
// Public PeakCompNode //
/////////////////////////

PeakCompNode::PeakCompNode(AudioContext & ac)
: lab::AudioBasicProcessorNode(ac, *desc())
{
    m_processor.reset(new PeakCompNodeInternal());

    auto n = static_cast<PeakCompNodeInternal *>(m_processor.get());
    n->m_threshold = param("threshold");
    n->m_ratio = param("ratio");
    n->m_attack = param("attack");
    n->m_release = param("release");
    n->m_makeup = param("makeup");
    n->m_knee = param("knee");
    initialize();
}

PeakCompNode::~PeakCompNode()
{
    uninitialize();
}

}  // End namespace lab
