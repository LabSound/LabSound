
// LabSound
// License is MIT: http://opensource.org/licenses/MIT

#include "ADSRNode.h"
#include "AudioContextLock.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "LabSound.h"
#include "AudioProcessor.h"
#include "VectorMath.h"

using namespace WebCore;
using namespace WebCore::VectorMath;
using namespace LabSound; 
using namespace std;

namespace LabSound {


    class ADSRNode::AdsrNodeInternal : public WebCore::AudioProcessor {
    public:

        AdsrNodeInternal(float sampleRate)
        : AudioProcessor(sampleRate)
        , numChannels(1)
        , m_noteOffTime(0)
        , m_currentGain(0)
        , m_noteOnTime(-1.)
        {
            m_attackTime = std::make_shared<AudioParam>("attackTime",  0.05, 0, 120);   // duration
            m_attackLevel = std::make_shared<AudioParam>("attackLevel",  1.0, 0, 10);   // duration
            m_decayTime = std::make_shared<AudioParam>("decayTime",   0.05,  0, 120);   // duration
            m_sustainLevel = std::make_shared<AudioParam>("sustain", 0.75, 0, 10);   // level
            m_releaseTime = std::make_shared<AudioParam>("release", 0.0625, 0, 120);   // duration
        }

        virtual ~AdsrNodeInternal() {
        }

        // AudioProcessor interface
        virtual void initialize() {
        }

        virtual void uninitialize() { }

        // Processes the source to destination bus.  The number of channels must match in source and destination.
        virtual void process(ContextRenderLock& r,
                             const WebCore::AudioBus* sourceBus, WebCore::AudioBus* destinationBus,
                             size_t framesToProcess) override {
            if (!numChannels)
                return;
            
            std::shared_ptr<WebCore::AudioContext> c = r.contextPtr();

            if (m_noteOnTime >= 0) {
                if (m_currentGain > 0) {
                    m_zeroSteps = 16;
                    m_zeroStepSize = -m_currentGain / 16.0f;
                }
                else
                    m_zeroSteps = 0;
                
                m_attackTimeTarget = m_noteOnTime + m_attackTime->value(c);
                
                m_attackSteps = m_attackTime->value(c) * sampleRate();
                m_attackStepSize = m_attackLevel->value(c) / m_attackSteps;
                
                m_decayTimeTarget = m_attackTimeTarget + m_decayTime->value(c);
                
                m_decaySteps = m_decayTime->value(c) * sampleRate();
                m_decayStepSize = (m_sustainLevel->value(c) - m_attackLevel->value(c)) / m_decaySteps;
                
                m_releaseSteps = 0;
                
                m_noteOffTime = DBL_MAX;
                m_noteOnTime = -1.;
            }
            
            // We handle both the 1 -> N and N -> N case here.
            const float* source = sourceBus->channel(0)->data();

            // this will only ever happen once, so if heap contention is an issue it should only ever cause one glitch
            // what would be better, alloca? What does webaudio do elsewhere for this sort of thing?
            if (gainValues.size() < framesToProcess)
                gainValues.resize(framesToProcess);

            float s = m_sustainLevel->value(c);

            for (size_t i = 0; i < framesToProcess; ++i) {
                if (m_zeroSteps > 0) {
                    --m_zeroSteps;
                    m_currentGain += m_zeroStepSize;
                    gainValues[i] = m_currentGain;
                }
                else if (m_attackSteps > 0) {
                    --m_attackSteps;
                    m_currentGain += m_attackStepSize;
                    gainValues[i] = m_currentGain;
                }
                else if (m_decaySteps > 0) {
                    --m_decaySteps;
                    m_currentGain += m_decayStepSize;
                    gainValues[i] = m_currentGain;
                }
                else if (m_releaseSteps > 0) {
                    --m_releaseSteps;
                    m_currentGain += m_releaseStepSize;
                    gainValues[i] = m_currentGain;
                }
                else {
                    m_currentGain = (m_noteOffTime == DBL_MAX) ? s : 0;
                    gainValues[i] = m_currentGain;
                }
            }

            for (unsigned int channelIndex = 0; channelIndex < numChannels; ++channelIndex) {
                if (sourceBus->numberOfChannels() == numChannels)
                    source = sourceBus->channel(channelIndex)->data();
                float* destination = destinationBus->channel(channelIndex)->mutableData();
                vmul(source, 1, &gainValues[0], 1, destination, 1, framesToProcess);
            }
        }

        // Resets filter state
        virtual void reset() override { }

        virtual void setNumberOfChannels(unsigned i) override {
            numChannels = i;
        }

        virtual double tailTime() const override { return 0; }
        virtual double latencyTime() const override { return 0; }

        void noteOn(double now) {
            m_noteOnTime = now;
        }

        void noteOff(ContextRenderLock& r, double now) {
            // note off at any time except while a note is on, has no effect
            m_noteOnTime = -1.;
            
            std::shared_ptr<WebCore::AudioContext> c = r.contextPtr();
            
            if (m_noteOffTime == DBL_MAX) {
                m_noteOffTime = now + m_releaseTime->value(c);

                m_releaseSteps = m_releaseTime->value(c) * sampleRate();
                m_releaseStepSize = -m_sustainLevel->value(c) / m_releaseSteps;
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
        
        unsigned int numChannels;
        double m_attackTimeTarget, m_decayTimeTarget, m_noteOffTime;
        float m_currentGain;
        std::vector<float> gainValues;
		std::shared_ptr<AudioParam> m_attackTime;
		std::shared_ptr<AudioParam> m_attackLevel;
		std::shared_ptr<AudioParam> m_decayTime;
		std::shared_ptr<AudioParam> m_sustainLevel;
		std::shared_ptr<AudioParam> m_releaseTime;
    };
    
    std::shared_ptr<AudioParam> ADSRNode::attackTime() const { return data->m_attackTime; }
    std::shared_ptr<AudioParam> ADSRNode::attackLevel() const { return data->m_attackLevel; }
    std::shared_ptr<AudioParam> ADSRNode::decayTime() const { return data->m_decayTime; }
    std::shared_ptr<AudioParam> ADSRNode::sustainLevel() const { return data->m_sustainLevel; }
    std::shared_ptr<AudioParam> ADSRNode::releaseTime() const { return data->m_releaseTime; }


    void ADSRNode::set(float aT, float aL, float d, float s, float r) {
        data->m_attackTime->setValue(aT);
        data->m_attackLevel->setValue(aL);
        data->m_decayTime->setValue(d);
        data->m_sustainLevel->setValue(s);
        data->m_releaseTime->setValue(r);
    }

    void ADSRNode::noteOn(double when) {
        data->noteOn(when);
    }

    void ADSRNode::noteOff(ContextRenderLock& r, double when) {
        data->noteOff(r, when);
    }
    
    bool ADSRNode::finished(ContextRenderLock& r) {
        if (!r.context())
            return true;
        
        double now = r.context()->currentTime();
        if (now > data->m_noteOffTime) {
            data->m_noteOffTime = 0;
        }
        return now > data->m_noteOffTime;
    }

    ADSRNode::ADSRNode(float sampleRate)
    : WebCore::AudioBasicProcessorNode(sampleRate)
    , data(new AdsrNodeInternal(sampleRate))
    {
        m_processor = std::move(std::unique_ptr<WebCore::AudioProcessor>(data));

        setNodeType((AudioNode::NodeType) LabSound::NodeTypeADSR);

        addInput(std::unique_ptr<AudioNodeInput>(new WebCore::AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new WebCore::AudioNodeOutput(this, 2))); // 2 stereo

        initialize();
    }

    ADSRNode::~ADSRNode() {
        data->numChannels = 0;
        delete data;
        data = 0;
        uninitialize();
    }

} // End namespace LabSound
