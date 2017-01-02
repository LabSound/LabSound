// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/ADSRNode.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"

#include "internal/AudioBus.h"
#include "internal/VectorMath.h"

#include <limits>

using namespace lab;

namespace lab
{
    /////////////////////////////////////
    // Private ADSRNode Implementation //
    /////////////////////////////////////

    class ADSRNode::ADSRNodeInternal : public lab::AudioProcessor
    {

    public:

        ADSRNodeInternal(float sampleRate) : AudioProcessor(sampleRate, 2), m_noteOffTime(0), m_currentGain(0), m_noteOnTime(-1.)
        {
            m_attackTime = std::make_shared<AudioParam>("attackTime",  0.05, 0, 120);
            m_attackLevel = std::make_shared<AudioParam>("attackLevel",  1.0, 0, 10);
            m_decayTime = std::make_shared<AudioParam>("decayTime",   0.05,  0, 120);
            m_sustainLevel = std::make_shared<AudioParam>("sustain", 0.75, 0, 10);
            m_releaseTime = std::make_shared<AudioParam>("release", 0.0625, 0, 120);
        }

        virtual ~ADSRNodeInternal() { }

        virtual void initialize() override { }

        virtual void uninitialize() override { }

        // Processes the source to destination bus. The number of channels must match in source and destination.
        virtual void process(ContextRenderLock& r,
                             const lab::AudioBus * sourceBus, lab::AudioBus* destinationBus,
                             size_t framesToProcess) override
        {
            if (!numberOfChannels())
                return;

            std::shared_ptr<lab::AudioContext> c = r.contextPtr();

            if (m_noteOnTime >= 0)
            {
                if (m_currentGain > 0)
                {
                    m_zeroSteps = 16;
                    m_zeroStepSize = -m_currentGain / 16.0f;
                }
                else
                    m_zeroSteps = 0;

                m_attackTimeTarget = m_noteOnTime + m_attackTime->value(r);

                m_attackSteps = m_attackTime->value(r) * sampleRate();
                m_attackStepSize = m_attackLevel->value(r) / m_attackSteps;

                m_decayTimeTarget = m_attackTimeTarget + m_decayTime->value(r);

                m_decaySteps = m_decayTime->value(r) * sampleRate();
                m_decayStepSize = (m_sustainLevel->value(r) - m_attackLevel->value(r)) / m_decaySteps;

                m_releaseSteps = 0;

                m_noteOffTime = std::numeric_limits<double>::max();
                m_noteOnTime = -1.;
            }

            // We handle both the 1 -> N and N -> N case here.
            const float* source = sourceBus->channelByType(Channel::First)->data();

            // this will only ever happen once, so if heap contention is an issue it should only ever cause one glitch
            // what would be better, alloca? What does webaudio do elsewhere for this sort of thing?
            if (gainValues.size() < framesToProcess)
                gainValues.resize(framesToProcess);

            float s = m_sustainLevel->value(r);

            for (size_t i = 0; i < framesToProcess; ++i)
            {
                if (m_zeroSteps > 0)
                {
                    --m_zeroSteps;
                    m_currentGain += m_zeroStepSize;
                    gainValues[i] = m_currentGain;
                }

                else if (m_attackSteps > 0)
                {
                    --m_attackSteps;
                    m_currentGain += m_attackStepSize;
                    gainValues[i] = m_currentGain;
                }

                else if (m_decaySteps > 0)
                {
                    --m_decaySteps;
                    m_currentGain += m_decayStepSize;
                    gainValues[i] = m_currentGain;
                }

                else if (m_releaseSteps > 0)
                {
                    --m_releaseSteps;
                    m_currentGain += m_releaseStepSize;
                    gainValues[i] = m_currentGain;
                }
                else
                {
                    m_currentGain = (m_noteOffTime == std::numeric_limits<double>::max()) ? s : 0;
                    gainValues[i] = m_currentGain;
                }
            }

            unsigned numChannels = numberOfChannels();
            for (unsigned int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
            {
                if (sourceBus->numberOfChannels() == numChannels)
                    source = sourceBus->channel(channelIndex)->data();

                float * destination = destinationBus->channel(channelIndex)->mutableData();

                VectorMath::vmul(source, 1, &gainValues[0], 1, destination, 1, framesToProcess);
            }
        }

        virtual void reset() override { }

        virtual double tailTime() const override { return 0; }
        virtual double latencyTime() const override { return 0; }

        void noteOn(double now)
        {
            m_noteOnTime = now;
        }

        void noteOff(ContextRenderLock& r, double now)
        {
            // note off at any time except while a note is on, has no effect
            m_noteOnTime = -1.;

            if (m_noteOffTime == std::numeric_limits<double>::max())
            {
                m_noteOffTime = now + m_releaseTime->value(r);
                m_releaseSteps = m_releaseTime->value(r) * sampleRate();
                m_releaseStepSize = -m_sustainLevel->value(r) / m_releaseSteps;
            }
        }

        int m_zeroSteps;
        float m_zeroStepSize;

        int m_attackSteps;
        float m_attackStepSize;

        int m_decaySteps;
        float m_decayStepSize;

        int m_releaseSteps;
        float m_releaseStepSize;

        double m_noteOnTime;

        double m_attackTimeTarget, m_decayTimeTarget, m_noteOffTime;

        float m_currentGain;

        std::vector<float> gainValues;

        std::shared_ptr<AudioParam> m_attackTime;
        std::shared_ptr<AudioParam> m_attackLevel;
        std::shared_ptr<AudioParam> m_decayTime;
        std::shared_ptr<AudioParam> m_sustainLevel;
        std::shared_ptr<AudioParam> m_releaseTime;
    };

    /////////////////////
    // Public ADSRNode //
    /////////////////////

    ADSRNode::ADSRNode(float sampleRate) : lab::AudioBasicProcessorNode(sampleRate)
    {
        m_processor.reset(new ADSRNodeInternal(sampleRate));

        internalNode = static_cast<ADSRNodeInternal*>(m_processor.get());

        setNodeType(lab::NodeTypeADSR);

        addInput(std::unique_ptr<AudioNodeInput>(new lab::AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new lab::AudioNodeOutput(this, 2)));

        m_params.push_back(internalNode->m_attackTime);
        m_params.push_back(internalNode->m_attackLevel);
        m_params.push_back(internalNode->m_decayTime);
        m_params.push_back(internalNode->m_sustainLevel);
        m_params.push_back(internalNode->m_releaseTime);

        initialize();
    }

    ADSRNode::~ADSRNode()
    {
        uninitialize();
    }


    void ADSRNode::noteOn(double when)
    {
        internalNode->noteOn(when);
    }

    void ADSRNode::noteOff(ContextRenderLock& r, double when)
    {
        internalNode->noteOff(r, when);
    }

    std::shared_ptr<AudioParam> ADSRNode::attackTime() const
    {
        return internalNode->m_attackTime;
    }

    void ADSRNode::set(float aT, float aL, float d, float s, float r)
    {
        internalNode->m_attackTime->setValue(aT);
        internalNode->m_attackLevel->setValue(aL);
        internalNode->m_decayTime->setValue(d);
        internalNode->m_sustainLevel->setValue(s);
        internalNode->m_releaseTime->setValue(r);
    }

    std::shared_ptr<AudioParam> ADSRNode::attackLevel() const
    {
        return internalNode->m_attackLevel;
    }

    std::shared_ptr<AudioParam> ADSRNode::decayTime() const
    {
        return internalNode->m_decayTime;
    }

    std::shared_ptr<AudioParam> ADSRNode::sustainLevel() const
    {
        return internalNode->m_sustainLevel;
    }

    std::shared_ptr<AudioParam> ADSRNode::releaseTime() const
    {
        return internalNode->m_releaseTime;
    }

    bool ADSRNode::finished(ContextRenderLock& r)
    {
        if (!r.context())
            return true;

        double now = r.context()->currentTime();

        if (now > internalNode->m_noteOffTime)
        {
            internalNode->m_noteOffTime = 0;
        }

        return now > internalNode->m_noteOffTime;
    }

} // End namespace lab
