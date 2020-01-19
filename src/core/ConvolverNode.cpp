
// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/ConvolverNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/extended/AudioContextLock.h"
#include "internal/VectorMath.h"

namespace lab {
    namespace Sound {

// from _SoundPipe_FFT.cpp
struct sp_conv;
struct sp_data;
struct sp_ftbl;
typedef float SPFLOAT;
int sp_create(sp_data ** spp);
int sp_destroy(sp_data ** spp);
int sp_conv_compute(sp_data * sp, sp_conv * p, SPFLOAT * in, SPFLOAT * out);
int sp_conv_create(sp_conv ** p);
int sp_conv_destroy(sp_conv ** p);
int sp_conv_init(sp_data * sp, sp_conv * p, sp_ftbl * ft, SPFLOAT iPartLen);
int sp_ftbl_destroy(sp_ftbl ** ft);
int sp_ftbl_bind(sp_data * sp, sp_ftbl ** ft, SPFLOAT * tbl, size_t size);

//------------------------------------------------------------------------------
// calculateNormalizationScale is adapted from webkit's Reverb.cpp, and carried the license:
// Empirical gain calibration tested across many impulse responses to ensure 
// perceived volume is same as dry (unprocessed) signal
const float GainCalibration = -58;
const float GainCalibrationSampleRate = 44100;

// A minimum power value to when normalizing a silent (or very quiet) impulse response
const float MinPower = 0.000125f;

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
//------------------------------------------------------------------------------


ConvolverNode::ReverbKernel::ReverbKernel(ReverbKernel && rh) noexcept
    : conv(rh.conv)
    , ft(rh.ft)
{
    rh.conv = nullptr;
    rh.ft = nullptr;
}

ConvolverNode::ReverbKernel::~ReverbKernel()
{
    if (ft)
        lab::Sound::sp_ftbl_destroy(&ft);

    if (conv)
        lab::Sound::sp_conv_destroy(&conv);
}


ConvolverNode::ConvolverNode()
: AudioScheduledSourceNode()
, m_normalize(std::make_shared<AudioSetting>("normalize"))
{
    m_settings.push_back(m_normalize);
    m_normalize->setUint32(1);

    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 0)));
    initialize();

    lab::Sound::sp_create(&_sp);
}

ConvolverNode::~ConvolverNode()
{
    _kernels.clear();
    lab::Sound::sp_destroy(&_sp);
    uninitialize();
}

bool ConvolverNode::normalize() const { return m_normalize->valueUint32() != 0; }
void ConvolverNode::setNormalize(bool new_n)
{
    bool n = normalize();
    if (new_n == n)
        return;

    size_t len = _impulseResponseClip->length();
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

void ConvolverNode::setImpulse(std::shared_ptr<AudioBus> bus)
{
    _kernels.clear();
    if (!bus)
        return;

    size_t len = bus->length();
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

    int c = static_cast<int>(_impulseResponseClip->numberOfChannels());
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

    start(0);
}

std::shared_ptr<AudioBus> ConvolverNode::getImpulse() const { return _impulseResponseClip; }

void ConvolverNode::process(ContextRenderLock & r, size_t framesToProcess)
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

    int numInputChannels = static_cast<int>(inputBus->numberOfChannels());
    int numOutputChannels = static_cast<int>(outputBus->numberOfChannels());
    int numReverbChannels = static_cast<int>(_kernels.size());
    bool valid = numInputChannels == numOutputChannels && ((numInputChannels == numReverbChannels) || (numReverbChannels == 1));

    if (!nonSilentFramesToProcess || !valid)
    {
        outputBus->zero();
        return;
    }

    for (int i = 0; i < numOutputChannels; ++i)
    {
        lab::Sound::sp_conv * conv = numReverbChannels == 1 ? _kernels[0].conv : _kernels[i].conv;
        float * destP = outputBus->channel(i)->mutableData();

        // Start rendering at the correct offset.
        destP += quantumFrameOffset;
        {
            size_t clipFrame = 0;
            AudioBus * input_bus = input(0)->bus(r);
            float const * data = input_bus->channel(i)->data();
            size_t c = input_bus->channel(i)->length();
            for (int j = 0; j < framesToProcess; ++j)
            {
                lab::Sound::SPFLOAT in = j < c ? data[j] : 0.f;  // don't read off the end of the input buffer
                lab::Sound::SPFLOAT out = 0.f;
                lab::Sound::sp_conv_compute(_sp, conv, &in, &out);
                *destP++ = out;
            }
        }
    }

    _now += double(framesToProcess) / r.context()->sampleRate();
    outputBus->clearSilentFlag();
}

void ConvolverNode::reset(ContextRenderLock &)
{
    _kernels.clear();
}

bool ConvolverNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}

}} // lab::Sound
