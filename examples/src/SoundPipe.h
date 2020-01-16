
// Prototype replacement for ReverbConvolverNode.

// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"
#include "internal/VectorMath.h"
#include <LabSound/core/AudioSetting.h>
#include <cmath>

struct sp_conv;
struct sp_data;
struct sp_ftbl;
typedef float SPFLOAT;
extern "C" int sp_create(sp_data ** spp);
extern "C" int sp_destroy(sp_data ** spp);
extern "C" int sp_conv_compute(sp_data * sp, sp_conv * p, SPFLOAT * in, SPFLOAT * out);
extern "C" int sp_conv_create(sp_conv ** p);
extern "C" int sp_conv_destroy(sp_conv ** p);
extern "C" int sp_conv_init(sp_data * sp, sp_conv * p, sp_ftbl * ft, SPFLOAT iPartLen);
extern "C" int sp_ftbl_destroy(sp_ftbl ** ft);
extern "C" int sp_ftbl_bind(sp_data * sp, sp_ftbl ** ft, SPFLOAT * tbl, size_t size);

// Empirical gain calibration tested across many impulse responses to ensure perceived volume is same as dry (unprocessed) signal
const float GainCalibration = -58;
const float GainCalibrationSampleRate = 44100;

// A minimum power value to when normalizing a silent (or very quiet) impulse response
const float MinPower = 0.000125f;

// This function is adapted from webkit's Reverb.cpp, and carried the license:
// License: BSD 3 Clause, Copyright (C) 2010, Google Inc. All rights reserved.
static float calculateNormalizationScale(AudioBus * response)
{
    // Normalize by RMS power
    size_t numberOfChannels = response->numberOfChannels();
    size_t length = response->length();

    float power = 0;

    for (size_t i = 0; i < numberOfChannels; ++i)
    {
        float channelPower = 0;
        VectorMath::vsvesq(response->channel(i)->data(), 1, &channelPower, length);
        power += channelPower;
    }

    power = sqrt(power / (numberOfChannels * length));

    // Protect against accidental overload
    if (std::isinf(power) || std::isnan(power) || power < MinPower)
        power = MinPower;

    float scale = 1 / power;

    scale *= powf(10, GainCalibration * 0.05f);  // calibrate to make perceived volume same as unprocessed

    // Scale depends on sample-rate.
    if (response->sampleRate())
        scale *= GainCalibrationSampleRate / response->sampleRate();

    // True-stereo compensation
    if (response->numberOfChannels() == Channels::Quad)
        scale *= 0.5f;

    return scale;
}

struct ReverbKernel
{
    ReverbKernel() = default;
    ReverbKernel(ReverbKernel && rh) noexcept
        : conv(rh.conv)
        , ft(rh.ft)
    {
        rh.conv = nullptr;
        rh.ft = nullptr;
    }

    ~ReverbKernel()
    {
        if (ft)
            sp_ftbl_destroy(&ft);

        if (conv)
            sp_conv_destroy(&conv);
    }

    sp_conv * conv = nullptr;
    sp_ftbl * ft = nullptr;
};

class SoundPipeNode : public AudioScheduledSourceNode
{
public:
    SoundPipeNode(int channels)
        : AudioScheduledSourceNode()
        , m_normalize(std::make_shared<AudioSetting>("normalize"))
    {
        m_settings.push_back(m_normalize);
        m_normalize->setUint32(1);

        addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 0)));
        initialize();

        sp_create(&_sp);
    }

    virtual ~SoundPipeNode()
    {
        _kernels.clear();
        sp_destroy(&_sp);
        uninitialize();
    }

    bool normalize() const { return m_normalize->valueUint32() != 0; }
    void setNormalize(bool new_n)
    {
        bool n = normalize();
        if (new_n == n)
            return;

        int len = _impulseResponseClip->length();
        if (n && _scale != 1.f)
        {
            // undo previous normalization
            float s = 1.f / _scale;
            for (int i = 0; i < _impulseResponseClip->numberOfChannels(); ++i)
            {
                float * data = _impulseResponseClip->channel(i)->mutableData();
                for (int j = 0; j < len; ++j)
                    data[j] *= s;
            }
        }
        if (!n)
        {
            // wasn't previously normalized, so normalize
            _scale = calculateNormalizationScale(_impulseResponseClip.get());
            for (int i = 0; i < _impulseResponseClip->numberOfChannels(); ++i)
            {
                float * data = _impulseResponseClip->channel(i)->mutableData();
                for (int j = 0; j < len; ++j)
                    data[j] *= _scale;
            }
        }

        m_normalize->setUint32(new_n ? 1 : 0);
    }

    void setImpulse(std::shared_ptr<AudioBus> bus)
    {
        _kernels.clear();

        int len = bus->length();
        _impulseResponseClip = std::make_shared<AudioBus>(bus->numberOfChannels(), len);
        _impulseResponseClip->copyFrom(*bus.get());

        if (normalize())
        {
            float scale = calculateNormalizationScale(bus.get());
            for (int i = 0; i < _impulseResponseClip->numberOfChannels(); ++i)
            {
                float * data = _impulseResponseClip->channel(i)->mutableData();
                for (int j = 0; j < len; ++j)
                    data[j] *= scale;
            }
        }

        int c = _impulseResponseClip->numberOfChannels();
        for (int i = 0; i < c; ++i)
        {
            // create one kernel per IR channel
            ReverbKernel kernel;

            // ft doesn't own the data; it does retain a pointer to it.
            sp_ftbl_bind(_sp, &kernel.ft,
                         _impulseResponseClip->channel(0)->mutableData(), _impulseResponseClip->channel(0)->length());

            sp_conv_create(&kernel.conv);
            sp_conv_init(_sp, kernel.conv, kernel.ft, 8192);

            _kernels.emplace_back(std::move(kernel));
        }
    }

    std::shared_ptr<AudioBus> getImpulse() const { return _impulseResponseClip; }

    virtual void process(ContextRenderLock & r, size_t framesToProcess) override
    {
        AudioBus * outputBus = output(0)->bus(r);
        AudioBus * inputBus = input(0)->bus(r);

        if (!isInitialized() || !outputBus || !inputBus || !inputBus->numberOfChannels() || !_kernels.size())
        {
            outputBus->zero();
            return;
        }

        // if the user never specified the number of output channels, make it match the input
        if (!outputBus->numberOfChannels())
            output(0)->setNumberOfChannels(r, inputBus->numberOfChannels());

        size_t quantumFrameOffset;
        size_t nonSilentFramesToProcess;

        updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);

        int numInputChannels = inputBus->numberOfChannels();
        int numOutputChannels = outputBus->numberOfChannels();
        int numReverbChannels = _kernels.size();
        bool valid = numInputChannels == numOutputChannels && ((numInputChannels == numReverbChannels) || (numReverbChannels == 1));

        if (!nonSilentFramesToProcess || !valid)
        {
            outputBus->zero();
            return;
        }

        for (int i = 0; i < numOutputChannels; ++i)
        {
            sp_conv * conv = numReverbChannels == 1 ? _kernels[0].conv : _kernels[i].conv;
            float * destP = outputBus->channel(i)->mutableData();

            // Start rendering at the correct offset.
            destP += quantumFrameOffset;
            {
                size_t clipFrame = 0;
                AudioBus * input_bus = input(0)->bus(r);
                float const * data = input_bus->channel(i)->data();
                int c = input_bus->channel(i)->length();
                for (int j = 0; j < framesToProcess; ++j)
                {
                    SPFLOAT in = j < c ? data[j] : 0.f;  // don't read off the end of the input buffer
                    SPFLOAT out = 0.f;
                    sp_conv_compute(_sp, conv, &in, &out);
                    *destP++ = out;
                }
            }
        }

        _now += double(framesToProcess) / r.context()->sampleRate();
        outputBus->clearSilentFlag();
    }

    virtual void reset(ContextRenderLock &) override {}

    double now() const { return _now; }

private:
    virtual bool propagatesSilence(ContextRenderLock & r) const override
    {
        return !isPlayingOrScheduled() || hasFinished();
    }

    double _now = 0.0;

    // Normalize the impulse response or not. Must default to true.
    std::shared_ptr<AudioSetting> m_normalize;

    std::shared_ptr<AudioBus> _impulseResponseClip;

    sp_data * _sp = nullptr;
    std::vector<ReverbKernel> _kernels;  // one per impulse response channel
    float _scale = 1.f;
};

struct SoundPipeApp : public LabSoundExampleApp
{
    virtual void PlayExample(int argc, char ** argv) override
    {
        auto context = lab::Sound::MakeRealtimeAudioContext(lab::Channels::Stereo);

        std::shared_ptr<SoundPipeNode> soundpipeNode;
        std::shared_ptr<SampledAudioNode> voiceNode;
        std::shared_ptr<GainNode> dryGainNode;
        std::shared_ptr<GainNode> wetGainNode;
        std::shared_ptr<AudioBus> voiceClip;
        {
            ContextRenderLock r(context.get(), "SoundPipe");

            soundpipeNode = std::make_shared<SoundPipeNode>(1);
            soundpipeNode->start(0);

            /// @TODO MakeBusFromFile should also do resampling because we need r.sampleRate() for sure
            const bool mixToMono = true;
            soundpipeNode->setImpulse(MakeBusFromFile("impulse/cardiod-rear-levelled.wav", mixToMono));

            wetGainNode = std::make_shared<GainNode>();
            wetGainNode->gain()->setValue(0.5f);

            voiceClip = MakeBusFromFile("samples/voice.ogg", mixToMono);
            voiceNode = std::make_shared<SampledAudioNode>();
            voiceNode->setBus(r, voiceClip);
            voiceNode->start(0.0f);

            dryGainNode = std::make_shared<GainNode>();
            dryGainNode->gain()->setValue(0.75f);

            context->connect(dryGainNode, voiceNode);
            context->connect(soundpipeNode, dryGainNode);
            context->connect(wetGainNode, soundpipeNode);
            context->connect(context->destination(), wetGainNode, 0, 0);
        }

        Wait(std::chrono::seconds(20));
    }
};
