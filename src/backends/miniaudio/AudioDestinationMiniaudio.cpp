// License: BSD 3 Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#include "AudioDestinationMiniaudio.h"
#include "internal/VectorMath.h"

#include "LabSound/core/AudioIOCallback.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/extended/Logging.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace lab
{

static int NumDefaultOutputChannels()
{
    /// @TODO how does miniaudio expose this?
    return 2;
}

AudioDestination * AudioDestination::MakePlatformAudioDestination(AudioIOCallback & callback,
                                                                  uint32_t numberOfInputChannels,
                                                                  uint32_t numberOfOutputChannels,
                                                                  float sampleRate)
{
    return new AudioDestinationMiniaudio(callback, 
                numberOfInputChannels, numberOfOutputChannels, sampleRate);
}

unsigned long AudioDestination::maxChannelCount()
{
    return NumDefaultOutputChannels();
}

AudioDestinationMiniaudio::AudioDestinationMiniaudio(AudioIOCallback & callback,
                                                     uint32_t numInputChannels,
                                                     uint32_t numChannels,
                                                     float sampleRate)
: _callback(callback)
, _numChannels(numChannels)
, _numInputChannels(numInputChannels)
, _sampleRate(sampleRate)
{
    configure();
}

AudioDestinationMiniaudio::~AudioDestinationMiniaudio()
{
    ma_device_uninit(&_device);
    delete _renderBus;
    delete _inputBus;
}

namespace
{
    void outputCallback(ma_device * pDevice, void * pOutput, const void * pInput, ma_uint32 frameCount)
    {
        // Buffer is nBufferFrames * channels, interleaved
        float * fBufOut = (float *) pOutput;
        AudioDestinationMiniaudio * audioDestination = reinterpret_cast<AudioDestinationMiniaudio *>(pDevice->pUserData);
        memset(fBufOut, 0, sizeof(float) * frameCount * audioDestination->channelCount());
        audioDestination->render(frameCount, pOutput, const_cast<void *>(pInput));
    }
}

void AudioDestinationMiniaudio::configure()
{
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_duplex);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = _numChannels;
    deviceConfig.sampleRate = static_cast<uint32_t>(_sampleRate);
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = _numInputChannels;
    deviceConfig.dataCallback = outputCallback;
    deviceConfig.performanceProfile = ma_performance_profile_low_latency;
    deviceConfig.pUserData = this;

    #ifdef __WINDOWS_WASAPI__
    deviceConfig.wasapi.noAutoConvertSRC = true;
    #endif

    if (ma_device_init(NULL, &deviceConfig, &_device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to open audio playback device");
        return;
    }
}

void AudioDestinationMiniaudio::start()
{
    if (ma_device_start(&_device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to start audio device");
        return;
    }
}

void AudioDestinationMiniaudio::stop()
{
    if (ma_device_stop(&_device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to stop audio device");
        return;
    }
}

const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;
const int kRenderQuantum = 128;

// Pulls on our provider to get rendered audio stream.
void AudioDestinationMiniaudio::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
{
    if (!_renderBus)
    {
        _renderBus = new AudioBus(_numChannels, kRenderQuantum, true);
        _renderBus->setSampleRate(_sampleRate);
    }

    if (!_inputBus && _numInputChannels)
    {
        _inputBus = new AudioBus(_numInputChannels, kRenderQuantum, true);
        _inputBus->setSampleRate(_sampleRate);
    }

    float * pIn = static_cast<float *>(inputBuffer);
    float * pOut = static_cast<float *>(outputBuffer);

    while (numberOfFrames > 0)
    {
        if (_remainder > 0)
        {
            // use up samples from previous callback
            int samples = _remainder < numberOfFrames ? _remainder : numberOfFrames;

            for (int i = 0; i < _numInputChannels; ++i)
            {
                AudioChannel * channel = _inputBus->channel(i);
                VectorMath::vclip(pIn + i, 1,
                                  &kLowThreshold, &kHighThreshold,
                                  channel->mutableData() + static_cast<ptrdiff_t>(kRenderQuantum - _remainder),
                                  _numInputChannels, samples);
            }
            pIn += static_cast<ptrdiff_t>(_numInputChannels * samples);

            for (int i = 0; i < _numInputChannels; ++i)
            {
                AudioChannel * channel = _renderBus->channel(i);
                VectorMath::vclip(channel->data() + static_cast<ptrdiff_t>(kRenderQuantum - _remainder), 1,
                                  &kLowThreshold, &kHighThreshold,
                                  pOut + i, _numChannels, samples);
            }
            pOut += static_cast<ptrdiff_t>(_numChannels * samples);

            numberOfFrames -= samples;
            _remainder -= samples;
        }
        else
        {
            // generate new data
            _callback.render(_inputBus, _renderBus, kRenderQuantum);
            _remainder = kRenderQuantum;
        }
    }
}

}  // namespace lab
