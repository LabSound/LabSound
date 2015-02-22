//
//  SupersawNode.cpp
//  LabSound
//
//  Created by Nick Porcino on 2013 11/17.
//
//  reference http://noisehack.com/how-to-build-supersaw-synth-web-audio-api/

#include "SupersawNode.h"

#include "OscillatorNode.h"

#include "LabSound.h"
#include "ADSRNode.h"
#include "AudioContextLock.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"

using namespace WebCore;

namespace LabSound {

    class SupersawNode::Data {
    public:
        std::shared_ptr<ADSRNode> gainNode;

        Data(SupersawNode* self, float sampleRate)
        : gainNode(std::make_shared<ADSRNode>(sampleRate))
        , sawCount(std::make_shared<AudioParam>("sawCount", 1.0, 100.0f, 3.0f))
        , detune(std::make_shared<AudioParam>("detune", 1.0, 0, 120))
        , frequency(std::make_shared<AudioParam>("frequency", 440.0, 1.0f, sampleRate * 0.5f))
        , self(self)
        , sampleRate(sampleRate)
        , cachedDetune(FLT_MAX)
        , cachedFrequency(FLT_MAX)
        {
        }

        void update(ContextGraphLock& g, ContextRenderLock& r, bool okayToReallocate) {
            int currentN = saws.size();
            std::shared_ptr<AudioContext> c = r.contextPtr();
            int n = int(sawCount->value(c) + 0.5f);
            if (okayToReallocate && (n != currentN)) {
                ExceptionCode ec;

                for (auto i : sawStorage) {
                    LabSound::disconnect(g, r, i.get());
                }
                sawStorage.clear();
                saws.clear();
                for (int i = 0; i < n; ++i)
                    sawStorage.emplace_back(std::make_shared<OscillatorNode>(r, sampleRate));
                for (int i = 0; i < n; ++i)
                    saws.push_back(sawStorage[i].get());

                for (auto i : saws) {
                    i->setType(r, OscillatorNode::SAWTOOTH, ec);
                    LabSound::connect(g, r, i, gainNode.get());
                    i->start(0);
                }
                cachedFrequency = FLT_MAX;
                cachedDetune = FLT_MAX;
            }

            if (cachedFrequency != frequency->value(c)) {
                cachedFrequency = frequency->value(c);
                for (auto i : saws) {
                    i->frequency()->setValue(cachedFrequency);
                    i->frequency()->resetSmoothedValue();
                }
            }

            if (cachedDetune != detune->value(c)) {
                cachedDetune = detune->value(c);
                float n = cachedDetune / ((float) saws.size() - 1.0f);
                for (size_t i = 0; i < saws.size(); ++i) {
                    saws[i]->detune()->setValue(-cachedDetune + float(i) * 2 * n);
                }
            }
        }

        SupersawNode* self;
        std::shared_ptr<AudioParam> detune;
        std::shared_ptr<AudioParam> frequency;
        std::shared_ptr<AudioParam> sawCount;
        std::vector<std::shared_ptr<OscillatorNode>> sawStorage;
        std::vector<OscillatorNode*> saws;

        float sampleRate;
        float cachedDetune;
        float cachedFrequency;
    };

    SupersawNode::SupersawNode(ContextGraphLock& g, ContextRenderLock& r,float sampleRate)
    : AudioNode(sampleRate)
    , _data(new Data(this, sampleRate))
    {
        addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

        setNodeType((AudioNode::NodeType) LabSound::NodeTypeSupersaw);

        initialize();
        LabSound::connect(g, r, _data->gainNode.get(), this);
    }

    void SupersawNode::process(ContextGraphLock& g, ContextRenderLock& r, size_t framesToProcess) {
        _data->update(g, r, false);
        AudioBus* outputBus = output(0)->bus();

        if (!isInitialized() || !outputBus->numberOfChannels()) {
            outputBus->zero();
            return;
        }

        AudioBus* inputBus = input(0)->bus();
        outputBus->copyFrom(*inputBus);
        outputBus->clearSilentFlag();
    }

    void SupersawNode::update(ContextGraphLock& g, ContextRenderLock& r) {
        _data->update(g, r, true);
    }

    std::shared_ptr<AudioParam> SupersawNode::attack()    const { return _data->gainNode->attackTime(); }
    std::shared_ptr<AudioParam> SupersawNode::decay()     const { return _data->gainNode->decayTime(); }
    std::shared_ptr<AudioParam> SupersawNode::sustain()   const { return _data->gainNode->sustainLevel(); }
    std::shared_ptr<AudioParam> SupersawNode::release()   const { return _data->gainNode->releaseTime(); }
    std::shared_ptr<AudioParam> SupersawNode::detune()    const { return _data->detune; }
    std::shared_ptr<AudioParam> SupersawNode::frequency() const { return _data->frequency; }
    std::shared_ptr<AudioParam> SupersawNode::sawCount()  const { return _data->sawCount; }

    void SupersawNode::noteOn(double when)  {
        _data->gainNode->noteOn(when);
    }
    void SupersawNode::noteOff(ContextRenderLock& r, double when) {
        _data->gainNode->noteOff(r, when);
    }

    bool SupersawNode::propagatesSilence(double now) const
    {
        return _data->gainNode->propagatesSilence(now);
    }

} // namespace LabSound
