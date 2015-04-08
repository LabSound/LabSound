
#include "LabSound/LabSound.h"

#include "LabSound/AudioBasicProcessorNode.h"
#include "LabSound/AudioNodeInput.h"
#include "LabSound/AudioNodeOutput.h"
#include "LabSound/AudioProcessor.h"
#include "LabSound/GainNode.h"

#include "Tonic.h"

#include <chrono>
#include <iostream>
#include <thread>

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








class TonicSample {
public:

    TonicSample() {
        ControlMetro metro = ControlMetro().bpm(100);
        ControlGenerator freq = ControlRandom().trigger(metro).min(0).max(1);

        Generator tone = SquareWaveBL().freq(
                                             freq * 0.25 + 100
                                             + 400
                                             ) * SineWave().freq(50);

        ADSR env = ADSR()
        .attack(0.01)
        .decay( 0.4 )
        .sustain(0)
        .release(0)
        .doesSustain(false)
        .trigger(metro);

        StereoDelay delay = StereoDelay(3.0f,3.0f)
        .delayTimeLeft( 0.5 + SineWave().freq(0.2) * 0.01)
        .delayTimeRight(0.55 + SineWave().freq(0.23) * 0.01)
        .feedback(0.3)
        .dryLevel(0.8)
        .wetLevel(0.2);

        Generator filterFreq = (SineWave().freq(0.01) + 1) * 200 + 225;

        LPF24 filter = LPF24().Q(2).cutoff( filterFreq );
        
        Generator output = (( tone * env ) >> filter >> delay) * 0.3;
        
        synth.setOutputGen(output);
    }

};














int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();

    Tonic::setSampleRate(context->sampleRate());

    // --------- MAKE A SYNTH HERE -----------



    // ---------------------------------------


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

    // update gain at 10ms intervals
    const float seconds = 10.0f;
    for (float i = 0; i < seconds; i += 0.01f) {
		std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }

    std::cout << "Ending tonic" << std::endl;
    
    return 0;
}
