// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#include "LabSound/extended/ADSRNode.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"
#include "LabSound/core/AudioBus.h"

#include "internal/VectorMath.h"

#include <math.h>
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
        enum class Phase { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };

        ADSRNodeInternal()
        : AudioProcessor(1), m_noteOnTime(-1.), m_noteOffTime(0), m_currentGain(0)
        {
            m_attackTime = std::make_shared<AudioSetting>("attackTime", "ATCK", AudioSetting::Type::Float);
            m_attackTime->setFloat(0.1f);
            m_attackLevel = std::make_shared<AudioSetting>("attackLevel", "ALVL", AudioSetting::Type::Float);
            m_attackLevel->setFloat(1.0f);
            m_holdTime = std::make_shared<AudioSetting>("holdTime", "HOLD", AudioSetting::Type::Float);
            m_holdTime->setFloat(0.1f);
            m_decayTime = std::make_shared<AudioSetting>("decayTime", "DECY", AudioSetting::Type::Float);
            m_decayTime->setFloat(0.05f);
            m_sustainLevel = std::make_shared<AudioSetting>("sustain", "SUS ", AudioSetting::Type::Float);
            m_sustainLevel->setFloat(0.75f);
            m_releaseTime = std::make_shared<AudioSetting>("release", "REL ", AudioSetting::Type::Float);
            m_releaseTime->setFloat(0.0625f);
        }

        virtual ~ADSRNodeInternal() { }

        virtual void initialize() override { m_initialized = true; }

        virtual void uninitialize() override { }

        // Processes the source to destination bus. The number of channels must match in source and destination.
        virtual void process(ContextRenderLock& r,
            const lab::AudioBus* sourceBus, lab::AudioBus* destinationBus,
            size_t framesToProcess) override
        {
            size_t source_channels = sourceBus->numberOfChannels();
            size_t dest_channels = destinationBus->numberOfChannels();

            float oosr = 1.f / r.context()->sampleRate();

            if (!isInitialized() || !source_channels || !dest_channels)
            {
                destinationBus->zero();
                return;
            }

            float quantum_duration = float(framesToProcess) * oosr;
            if (m_on_scheduled)
                m_noteOnTime -= quantum_duration;
            if (m_off_scheduled)
                m_noteOffTime -= quantum_duration;

            if (m_noteOnTime > quantum_duration || (!m_on_scheduled && m_phase == Phase::OFF))
            {
                // quick bail out if the scheduled start hasn't arrived, after decrementing scheduling
                m_noteOnTime -= quantum_duration;
                m_noteOffTime -= quantum_duration;
                destinationBus->zero();
                return;
            }

            // prepare a frame accurate start
            size_t silence = 0;
            if (m_noteOnTime > 0)
                silence = static_cast<size_t>(m_noteOnTime * oosr);

            if (gainValues.size() < framesToProcess)
                gainValues.resize(framesToProcess);

            float s = m_sustainLevel->valueFloat();
            bool done = false;
            for (size_t i = 0; i < framesToProcess; ++i)
            {
                if (i < silence)
                {
                    gainValues[i] = 0;
                    continue;
                }
                switch (m_phase)
                {
                case Phase::OFF:
                    m_currentGain = 0.f;
                    m_gainSteps = static_cast<int>(m_attackTime->valueFloat() * r.context()->sampleRate());
                    m_gainStepSize = m_attackLevel->valueFloat() / m_gainSteps;
                    m_phase = Phase::ATTACK;
                    m_on_scheduled = false;
                    break;

                case Phase::ATTACK:
                    --m_gainSteps;
                    m_currentGain += m_gainStepSize;
                    gainValues[i] = m_currentGain;
                    if (m_gainSteps <= 0)
                    {
                        m_gainSteps = static_cast<int>(m_decayTime->valueFloat() * r.context()->sampleRate());
                        m_gainStepSize = (m_sustainLevel->valueFloat() - m_attackLevel->valueFloat()) / m_gainSteps;
                        m_phase = Phase::DECAY;
                    }
                    break;

                case Phase::DECAY:
                    --m_gainSteps;
                    m_currentGain += m_gainStepSize;
                    gainValues[i] = m_currentGain;
                    if (m_gainSteps <= 0)
                    {
                        m_phase = Phase::SUSTAIN;
                    }
                    break;

                case Phase::SUSTAIN:
                    if (m_noteOffTime > 0)
                    {
                        gainValues[i] = m_currentGain;
                        m_noteOffTime -= oosr;
                    }
                    else
                    {
                        m_gainSteps = static_cast<int>(m_releaseTime->valueFloat() * r.context()->sampleRate());
                        m_gainStepSize = -m_sustainLevel->valueFloat() / m_gainSteps;
                        m_phase = Phase::RELEASE;
                    }
                    break;
                
                case Phase::RELEASE:
                    --m_gainSteps;
                    m_currentGain += m_gainStepSize;
                    gainValues[i] = m_currentGain;
                    if (m_gainSteps <= 0 || fabs(m_currentGain) < 0.0001f)
                    {
                        done = true;
                    }
                    break;
               }
            }

            if (done)
                m_phase = Phase::OFF;

            for (size_t channelIndex = 0; channelIndex < dest_channels; ++channelIndex)
            {
                const float* source;
                if (channelIndex < source_channels)
                    source = sourceBus->channel(channelIndex)->data();
                else
                    source = sourceBus->channel(source_channels - 1)->data();

                float * destination = destinationBus->channel(channelIndex)->mutableData();
                VectorMath::vmul(source, 1, &gainValues[0], 1, destination, 1, framesToProcess);
            }
        }

        virtual void reset() override { }

        virtual double tailTime(ContextRenderLock & r) const override { return 0; }
        virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

        void noteOn(double now)
        {
            m_noteOnTime = now;
            m_noteOffTime = FLT_MAX;
            m_on_scheduled = true;
            m_phase = Phase::OFF;
        }

        void noteOff(ContextRenderLock& r, double now)
        {
            m_off_scheduled = true;
            m_noteOffTime = now;
        }

        bool m_on_scheduled = false;
        bool m_off_scheduled = false;
        Phase m_phase = Phase::OFF;

        int m_gainSteps;
        float m_gainStepSize;
        float m_currentGain;

        double m_noteOnTime, m_noteOffTime;

        std::vector<float> gainValues;

        std::shared_ptr<AudioSetting> m_attackTime;
        std::shared_ptr<AudioSetting> m_attackLevel;
        std::shared_ptr<AudioSetting> m_holdTime;
        std::shared_ptr<AudioSetting> m_decayTime;
        std::shared_ptr<AudioSetting> m_sustainLevel;
        std::shared_ptr<AudioSetting> m_releaseTime;
    };

    /////////////////////
    // Public ADSRNode //
    /////////////////////

    ADSRNode::ADSRNode() : lab::AudioBasicProcessorNode()
    {
        m_processor.reset(new ADSRNodeInternal());

        internalNode = static_cast<ADSRNodeInternal*>(m_processor.get());

        m_settings.push_back(internalNode->m_attackTime);
        m_settings.push_back(internalNode->m_attackLevel);
        m_settings.push_back(internalNode->m_holdTime);
        m_settings.push_back(internalNode->m_decayTime);
        m_settings.push_back(internalNode->m_sustainLevel);
        m_settings.push_back(internalNode->m_releaseTime);

        initialize();
        m_processor->initialize();
    }

    ADSRNode::~ADSRNode()
    {
        uninitialize();
    }

    void ADSRNode::bang(ContextRenderLock& r)
    {
        internalNode->noteOn(0);
        internalNode->noteOff(r, internalNode->m_holdTime->valueFloat());
    }

    void ADSRNode::noteOn(double when)
    {
        internalNode->noteOn(when);
    }

    void ADSRNode::noteOff(ContextRenderLock& r, double when)
    {
        internalNode->noteOff(r, when);
    }

    std::shared_ptr<AudioSetting> ADSRNode::attackTime() const
    {
        return internalNode->m_attackTime;
    }

    void ADSRNode::set(float aT, float aL, float d, float s, float r)
    {
        internalNode->m_attackTime->setFloat(aT);
        internalNode->m_attackLevel->setFloat(aL);
        internalNode->m_decayTime->setFloat(d);
        internalNode->m_sustainLevel->setFloat(s);
        internalNode->m_releaseTime->setFloat(r);
    }

    std::shared_ptr<AudioSetting> ADSRNode::attackLevel() const
    {
        return internalNode->m_attackLevel;
    }

    std::shared_ptr<AudioSetting> ADSRNode::decayTime() const
    {
        return internalNode->m_decayTime;
    }

    std::shared_ptr<AudioSetting> ADSRNode::holdTime() const
    {
        return internalNode->m_holdTime;
    }

    std::shared_ptr<AudioSetting> ADSRNode::sustainLevel() const
    {
        return internalNode->m_sustainLevel;
    }

    std::shared_ptr<AudioSetting> ADSRNode::releaseTime() const
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
