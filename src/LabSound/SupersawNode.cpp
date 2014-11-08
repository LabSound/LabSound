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
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"

using namespace WebCore;

namespace LabSound {

    class SupersawNode::Data {
    public:
        RefPtr<ADSRNode> gainNode;

        Data(SupersawNode* self, AudioContext* context, float sampleRate)
        : gainNode(ADSRNode::create(context, sampleRate))
        , sawCount(AudioParam::create(context, "detune", 1.0, 100.0f, 3.0f))
        , detune(AudioParam::create(context, "detune", 1.0, 0, 120))
        , frequency(AudioParam::create(context, "frequency", 440.0, 1.0f, sampleRate * 0.5f))
        , self(self)
        , context(context)
        , sampleRate(sampleRate)
        , cachedDetune(FLT_MAX)
        , cachedFrequency(FLT_MAX)
        {
        }

        void update(bool okayToReallocate) {
            int currentN = saws.size();
            int n = int(sawCount->value() + 0.5f);
            if (okayToReallocate && (n != currentN)) {
                WebCore::ExceptionCode ec;

                for (auto i : sawStorage) {
                    LabSound::disconnect(i.get());
                }
                sawStorage.clear();
                saws.clear();
                for (int i = 0; i < n; ++i)
                    sawStorage.push_back(OscillatorNode::create(context, sampleRate));
                for (int i = 0; i < n; ++i)
                    saws.push_back(sawStorage[i].get());

                for (auto i : saws) {
                    i->setType(OscillatorNode::SAWTOOTH, ec);
                    LabSound::connect(i, gainNode.get());
                    i->start(0);
                }
                cachedFrequency = FLT_MAX;
                cachedDetune = FLT_MAX;
            }

            if (cachedFrequency != frequency->value()) {
                cachedFrequency = frequency->value();
                for (auto i : saws) {
                    i->frequency()->setValue(cachedFrequency);
                    i->frequency()->resetSmoothedValue();
                }
            }

            if (cachedDetune != detune->value()) {
                cachedDetune = detune->value();
                float n = cachedDetune / ((float) saws.size() - 1.0f);
                for (int i = 0; i < saws.size(); ++i) {
                    saws[i]->detune()->setValue(-cachedDetune + float(i) * 2 * n);
                }
            }
        }

        SupersawNode* self;
        AudioContext* context;
        RefPtr<AudioParam> detune;
        RefPtr<AudioParam> frequency;
        RefPtr<AudioParam> sawCount;
        std::vector<RefPtr<OscillatorNode>> sawStorage;
        std::vector<OscillatorNode*> saws;

        float sampleRate;
        float cachedDetune;
        float cachedFrequency;
    };

    SupersawNode::SupersawNode(AudioContext* context, float sampleRate)
    : AudioNode(context, sampleRate)
    , _data(new Data(this, context, sampleRate))
    {
        addInput(adoptPtr(new AudioNodeInput(this)));
        addOutput(adoptPtr(new AudioNodeOutput(this, 1)));

        setNodeType((AudioNode::NodeType) LabSound::NodeTypeSupersaw);

        initialize();
        LabSound::connect(_data->gainNode.get(), this);
    }

    void SupersawNode::process(size_t framesToProcess) {
        _data->update(false);
        AudioBus* outputBus = output(0)->bus();

        if (!isInitialized() || !outputBus->numberOfChannels()) {
            outputBus->zero();
            return;
        }

        AudioBus* inputBus = input(0)->bus();
        outputBus->copyFrom(*inputBus);
        outputBus->clearSilentFlag();
    }

    void SupersawNode::update() {
        _data->update(true);
    }

    AudioParam* SupersawNode::attack()    const { return _data->gainNode->attackTime(); }
    AudioParam* SupersawNode::decay()     const { return _data->gainNode->decayTime(); }
    AudioParam* SupersawNode::sustain()   const { return _data->gainNode->sustainLevel(); }
    AudioParam* SupersawNode::release()   const { return _data->gainNode->releaseTime(); }
    AudioParam* SupersawNode::detune()    const { return _data->detune.get(); }
    AudioParam* SupersawNode::frequency() const { return _data->frequency.get(); }
    AudioParam* SupersawNode::sawCount()  const { return _data->sawCount.get(); }

    void SupersawNode::noteOn()  {
        _data->gainNode->noteOn();
    }
    void SupersawNode::noteOff() { _data->gainNode->noteOff(); }

    bool SupersawNode::propagatesSilence() const
    {
        return _data->gainNode->propagatesSilence();
    }

} // namespace LabSound
