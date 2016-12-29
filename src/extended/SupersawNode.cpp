// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/OscillatorNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/Synthesis.h"

#include "LabSound/extended/SupersawNode.h"
#include "LabSound/extended/ADSRNode.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioBus.h"

#include <cfloat>

using namespace lab;

namespace lab
{

    //////////////////////////////////////////
    // Private Supersaw Node Implementation //
    //////////////////////////////////////////

    class SupersawNode::SupersawNodeInternal
    {
    public:

        SupersawNodeInternal(float sampleRate) : sampleRate(sampleRate), cachedDetune(FLT_MAX), cachedFrequency(FLT_MAX)
        {
            gainNode = std::make_shared<ADSRNode>(sampleRate);
            sawCount = std::make_shared<AudioParam>("sawCount", 1.0, 100.0f, 3.0f);
            detune = std::make_shared<AudioParam>("detune", 1.0, 0, 120);
            frequency= std::make_shared<AudioParam>("frequency", 440.0, 1.0f, sampleRate * 0.5f);
        }

        ~SupersawNodeInternal()
        {

        }

        void update(ContextRenderLock& r)
        {
            if (cachedFrequency != frequency->value(r))
            {
                cachedFrequency = frequency->value(r);
                for (auto i : saws)
                {
                    i->frequency()->setValue(cachedFrequency);
                    i->frequency()->resetSmoothedValue();
                }
            }

            if (cachedDetune != detune->value(r))
            {
                cachedDetune = detune->value(r);
                float n = cachedDetune / ((float) saws.size() - 1.0f);
                for (size_t i = 0; i < saws.size(); ++i)
                {
                    saws[i]->detune()->setValue(-cachedDetune + float(i) * 2 * n);
                }
            }
        }

        void update(ContextRenderLock& r, bool okayToReallocate)
        {
            int currentN = saws.size();
            int n = int(sawCount->value(r) + 0.5f);

            if (okayToReallocate && (n != currentN))
            {
                for (auto i : sawStorage)
                {
                    r.context()->disconnect(i);
                }

                sawStorage.clear();
                saws.clear();

                for (int i = 0; i < n; ++i)
                    sawStorage.emplace_back(std::make_shared<OscillatorNode>(r, sampleRate));

                for (int i = 0; i < n; ++i)
                    saws.push_back(sawStorage[i].get());


                auto c = r.context();
                for (auto i : sawStorage)
                {
                    i->setType(r, OscillatorType::SAWTOOTH);
                    c->connect(i, gainNode);
                    i->start(0);
                }

                cachedFrequency = FLT_MAX;
                cachedDetune = FLT_MAX;
            }

            update(r);
        }

        std::shared_ptr<ADSRNode> gainNode;
        std::shared_ptr<AudioParam> detune;
        std::shared_ptr<AudioParam> frequency;
        std::shared_ptr<AudioParam> sawCount;

    private:

        float sampleRate;
        float cachedDetune;
        float cachedFrequency;

        std::vector<std::shared_ptr<OscillatorNode>> sawStorage;
        std::vector<OscillatorNode * > saws;
    };

    //////////////////////////
    // Public Supersaw Node //
    //////////////////////////

    SupersawNode::SupersawNode(ContextRenderLock & r, float sampleRate) : AudioNode(sampleRate)
    {
        internalNode.reset(new SupersawNodeInternal(sampleRate));

        m_params.push_back(internalNode->detune);
        m_params.push_back(internalNode->frequency);
        m_params.push_back(internalNode->sawCount);

        addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

        setNodeType(lab::NodeType::NodeTypeSupersaw);

        initialize();
    }

    SupersawNode::~SupersawNode()
    {
        uninitialize();
    }

    void SupersawNode::process(ContextRenderLock & r, size_t framesToProcess)
    {
        internalNode->update(r);

        AudioBus * outputBus = output(0)->bus(r);

        if (!isInitialized() || !outputBus->numberOfChannels())
        {
            outputBus->zero();
            return;
        }

        AudioBus * inputBus = input(0)->bus(r);

        outputBus->copyFrom(*inputBus);
        outputBus->clearSilentFlag();
    }

    void SupersawNode::update(ContextRenderLock & r)
    {
        internalNode->update(r, true);
    }

    std::shared_ptr<AudioParam> SupersawNode::attack() const { return internalNode->gainNode->attackTime(); }
    std::shared_ptr<AudioParam> SupersawNode::decay() const { return internalNode->gainNode->decayTime(); }
    std::shared_ptr<AudioParam> SupersawNode::sustain() const { return internalNode->gainNode->sustainLevel(); }
    std::shared_ptr<AudioParam> SupersawNode::release() const { return internalNode->gainNode->releaseTime(); }
    std::shared_ptr<AudioParam> SupersawNode::detune() const { return internalNode->detune; }
    std::shared_ptr<AudioParam> SupersawNode::frequency() const { return internalNode->frequency; }
    std::shared_ptr<AudioParam> SupersawNode::sawCount() const { return internalNode->sawCount; }

    void SupersawNode::noteOn(double when)
    {
        internalNode->gainNode->noteOn(when);
    }

    void SupersawNode::noteOff(ContextRenderLock& r, double when)
    {
        internalNode->gainNode->noteOff(r, when);
    }

    bool SupersawNode::propagatesSilence(double now) const
    {
        return internalNode->gainNode->propagatesSilence(now);
    }

} // End namespace lab
