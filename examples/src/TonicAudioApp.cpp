
#include "LabSound/LabSound.h"

#include "LabSound/AudioBasicProcessorNode.h"
#include "LabSound/AudioNodeInput.h"
#include "LabSound/AudioNodeOutput.h"
#include "LabSound/AudioProcessor.h"
#include "LabSound/GainNode.h"

#include <iostream>

using namespace LabSound;

class TonicAudioNode : public AudioBasicProcessorNode
{
public:
    static WTF::PassRefPtr<TonicAudioNode> create(WebCore::AudioContext* context, float sampleRate)
    {
        return adoptRef(new TonicAudioNode(context, sampleRate));
    }

    virtual bool propagatesSilence() const OVERRIDE;

private:
    TonicAudioNode(WebCore::AudioContext*, float sampleRate);
    virtual ~TonicAudioNode();

    class TonicNodeInternal;
    TonicNodeInternal* data;
};





class TonicAudioNode::TonicNodeInternal : public AudioProcessor {
public:

    TonicNodeInternal(float sampleRate)
    : AudioProcessor(sampleRate)
    , numChannels(2)
    , pdBuffSize(128)
    {
    }

    virtual ~TonicNodeInternal() {
    }

    // AudioProcessor interface
    virtual void initialize() {
    }

    virtual void uninitialize() { }

    // Processes the source to destination bus.  The number of channels must match in source and destination.
    virtual void process(const WebCore::AudioBus* source, WebCore::AudioBus* destination, size_t framesToProcess) {
        if (!numChannels)
            return;
    }

    // Resets filter state
    virtual void reset() { }

    virtual void setNumberOfChannels(unsigned i) {
        numChannels = i;
    }

    virtual double tailTime() const { return 0; }
    virtual double latencyTime() const { return 0; }

    int numChannels;
    int pdBuffSize;
    int blockSizeAsLog;
};

TonicAudioNode::TonicAudioNode(AudioContext* context, float sampleRate)
: AudioBasicProcessorNode(context, sampleRate)
, data(new TonicNodeInternal(sampleRate))
{
    // Initially setup as lowpass filter.
    m_processor = std::move(std::unique_ptr<WebCore::AudioProcessor>(data));

    setNodeType((AudioNode::NodeType) LabSound::NodeTypePd);

    addInput(adoptPtr(new AudioNodeInput(this)));
    addOutput(adoptPtr(new AudioNodeOutput(this, 2))); // 2 stereo

    initialize();
}

TonicAudioNode::~TonicAudioNode() {
    data->numChannels = 0;  // ensure there if there is a latent callback pending, pd is not invoked
    data = 0;
    uninitialize();
}

bool TonicAudioNode::propagatesSilence() const {
    return false;
}
























int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();

    ExceptionCode ec;

    RefPtr<TonicAudioNode> tonic = TonicAudioNode::create(context.get(), context->sampleRate());
    RefPtr<GainNode> wetGain = context->createGain();
    wetGain->gain()->setValue(2.f);
    RefPtr<GainNode> dryGain = context->createGain();
    dryGain->gain()->setValue(1.f);

    tonic->connect(wetGain.get(), 0, 0, ec);
    wetGain->connect(context->destination(), 0, 0, ec);
    dryGain->connect(context->destination(), 0, 0, ec);
    dryGain->connect(tonic.get(), 0, 0, ec);

    std::cout << "Starting tonic" << std::endl;

    float seconds = 10.0f;
    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
    std::cout << "Ending tonic" << std::endl;
    
    return 0;
}
