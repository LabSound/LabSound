// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/ADSRNode.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"
#include "LabSound/core/AudioBus.h"

#include "internal/VectorMath.h"

#include <limits>

using namespace lab;

namespace lab
{
    ///////////////////////////////////////////
    // ADSRNode::ADSRNodeImpl Implementation //
    ///////////////////////////////////////////

    class ADSRNode::ADSRNodeImpl : public lab::AudioProcessor
    {
        float cached_sample_rate;

    public:

        ADSRNodeImpl() : AudioProcessor(2)
        {
            envelope.reserve(AudioNode::ProcessingSizeInFrames * 4);

            m_attackTime = std::make_shared<AudioSetting>("attackTime", "ATKT", AudioSetting::Type::Float);
            m_attackTime->setFloat(0.125f); // 125ms

            m_attackLevel = std::make_shared<AudioSetting>("attackLevel", "ATKL", AudioSetting::Type::Float);
            m_attackLevel->setFloat(1.0f); // 1.0f

            m_decayTime = std::make_shared<AudioSetting>("decayTime", "DCYT", AudioSetting::Type::Float);
            m_decayTime->setFloat(0.125f); // 125ms

            m_sustainLevel = std::make_shared<AudioSetting>("sustainLevel", "SUSL", AudioSetting::Type::Float);
            m_sustainLevel->setFloat(0.5f); // 0.5f

            m_releaseTime = std::make_shared<AudioSetting>("ReleaseTime", "RELT", AudioSetting::Type::Float);
            m_releaseTime->setFloat(0.125f); // 125ms
        }

        virtual ~ADSRNodeImpl() {}

        virtual void initialize() override { }
        virtual void uninitialize() override { }

        // Processes the source to destination bus. The number of channels must match in source and destination.
        virtual void process(ContextRenderLock & r, const lab::AudioBus * sourceBus, lab::AudioBus* destinationBus, int framesToProcess) override
        {
            if (!numberOfChannels())
                return;

            if (m_noteOnTime >= 0)
            {
                cached_sample_rate = r.context()->sampleRate();

                // If there's an active envelope value and we begin this node, we first
                // rapidly push the value to zero before beginning the new attack
                if (currentEnvelope > 0)
                {
                    m_zeroSteps = 16;
                    m_zeroStepSize = -currentEnvelope / 16.0f;
                }
                else m_zeroSteps = 0;

                m_attackTimeTarget = m_noteOnTime + m_attackTime->valueFloat();

                m_attackSteps = static_cast<int>(m_attackTime->valueFloat() * cached_sample_rate);
                m_attackStepSize = m_attackLevel->valueFloat() / m_attackSteps;

                m_decayTimeTarget = m_attackTimeTarget + m_decayTime->valueFloat();

                m_decaySteps = static_cast<int>(m_decayTime->valueFloat() * cached_sample_rate);
                m_decayStepSize = (m_sustainLevel->valueFloat() - m_attackLevel->valueFloat()) / m_decaySteps;

                m_releaseSteps = 0;

                m_noteOffTime = std::numeric_limits<double>::max();
                m_noteOnTime = -std::numeric_limits<double>::max();
            }

            // We handle both the 1 -> N and N -> N case here.
            const float * source = sourceBus->channelByType(Channel::First)->data();

            // this will only ever happen once, so if heap contention is an issue it should only ever cause one glitch
            // what would be better, alloca? What does webaudio do elsewhere for this sort of thing?
            // -> See the .reserve() call in the constructor; impact should be minimized.
            if (envelope.size() < framesToProcess)
            {
                envelope.resize(framesToProcess);
            }

            const float sustain_lvl = m_sustainLevel->valueFloat();

            for (int i = 0; i < framesToProcess; ++i)
            {
                if (m_zeroSteps > 0)
                {
                    // Pull envelope to zero
                    --m_zeroSteps;
                    currentEnvelope += m_zeroStepSize;
                    envelope[i] = currentEnvelope;
                }
                else if (m_attackSteps > 0)
                {
                    // Do the attack
                    --m_attackSteps;
                    currentEnvelope += m_attackStepSize;
                    envelope[i] = currentEnvelope;
                }
                else if (m_decaySteps > 0)
                {
                    // Do the decay
                    --m_decaySteps;
                    currentEnvelope += m_decayStepSize;
                    envelope[i] = currentEnvelope;
                }
                else if (m_releaseSteps > 0)
                {
                    // Do the release
                    --m_releaseSteps;
                    currentEnvelope += m_releaseStepSize;
                    envelope[i] = currentEnvelope;
                }
                else
                {
                    // Otherwise do the sustain
                    currentEnvelope = (m_noteOffTime == std::numeric_limits<double>::max()) ? sustain_lvl : 0;
                    envelope[i] = currentEnvelope;
                }
            }

            const int numChannels = numberOfChannels();
            for (int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
            {
                if (sourceBus->numberOfChannels() == numChannels)
                {
                    source = sourceBus->channel(channelIndex)->data();
                }

                float * destination = destinationBus->channel(channelIndex)->mutableData();

                VectorMath::vmul(source, 1, &envelope[0], 1, destination, 1, framesToProcess);
            }
        }

        virtual void reset() override { }

        virtual double tailTime(ContextRenderLock & r) const override { return 0; }
        virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

        void noteOn(const double time)
        {
            m_noteOnTime = time;
        }

        void noteOff(const double time)
        {
            // note off at any time except while a note is on, has no effect
            m_noteOnTime = -std::numeric_limits<double>::max();

            if (m_noteOffTime == std::numeric_limits<double>::max())
            {
                m_noteOffTime = time + m_releaseTime->valueFloat();
                m_releaseSteps = static_cast<int>(m_releaseTime->valueFloat() * cached_sample_rate);
                m_releaseStepSize = -m_sustainLevel->valueFloat() / m_releaseSteps;
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

        double m_attackTimeTarget;
        double m_decayTimeTarget;

        double m_noteOnTime { -std::numeric_limits<double>::max() };
        double m_noteOffTime {0.0};

        float currentEnvelope {0.f};
        std::vector<float> envelope;

        std::shared_ptr<AudioSetting> m_attackTime;
        std::shared_ptr<AudioSetting> m_attackLevel;
        std::shared_ptr<AudioSetting> m_decayTime;
        std::shared_ptr<AudioSetting> m_sustainLevel;
        std::shared_ptr<AudioSetting> m_releaseTime;
    };

    /////////////////////
    // Public ADSRNode //
    /////////////////////

    ADSRNode::ADSRNode(AudioContext& ac) 
        : AudioBasicProcessorNode(ac)
    {
        m_processor.reset(new ADSRNodeImpl());
        adsr_impl = static_cast<ADSRNodeImpl*>(m_processor.get());

        m_settings.push_back(adsr_impl->m_attackTime);
        m_settings.push_back(adsr_impl->m_attackLevel);
        m_settings.push_back(adsr_impl->m_decayTime);
        m_settings.push_back(adsr_impl->m_sustainLevel);
        m_settings.push_back(adsr_impl->m_releaseTime);

        initialize();
    }

    ADSRNode::~ADSRNode()
    {
        uninitialize();
    }

    void ADSRNode::noteOn(const double when)
    {
        adsr_impl->noteOn(when);
    }

    void ADSRNode::noteOff(double when)
    {
        adsr_impl->noteOff(when);
    }

    void ADSRNode::bang(const double time)
    {
        adsr_impl->noteOn(0);

        if (time > 0.f)
        {
            adsr_impl->noteOff(time);
        }
        else
        {
            // @todo - impulse
        }
    }

    void ADSRNode::set(float attack_time, float attack_level, float decay_time, float sustain_level, float release_time)
    {
        adsr_impl->m_attackTime->setFloat(attack_time);
        adsr_impl->m_attackLevel->setFloat(attack_level);
        adsr_impl->m_decayTime->setFloat(decay_time);
        adsr_impl->m_sustainLevel->setFloat(sustain_level);
        adsr_impl->m_releaseTime->setFloat(release_time);
    }

    // clang-format off
    std::shared_ptr<AudioSetting> ADSRNode::attackTime() const   { return adsr_impl->m_attackTime;   }
    std::shared_ptr<AudioSetting> ADSRNode::attackLevel() const  { return adsr_impl->m_attackLevel;  }
    std::shared_ptr<AudioSetting> ADSRNode::decayTime() const    { return adsr_impl->m_decayTime;    } 
    std::shared_ptr<AudioSetting> ADSRNode::sustainLevel() const { return adsr_impl->m_sustainLevel; }
    std::shared_ptr<AudioSetting> ADSRNode::releaseTime() const  { return adsr_impl->m_releaseTime;  }
    //clang-format on

    bool ADSRNode::finished(ContextRenderLock& r)
    {
        if (!r.context())
            return true;

        double now = r.context()->currentTime();

        if (now > adsr_impl->m_noteOffTime)
        {
            adsr_impl->m_noteOffTime = 0;
        }

        return now > adsr_impl->m_noteOffTime;
    }

} // End namespace lab
