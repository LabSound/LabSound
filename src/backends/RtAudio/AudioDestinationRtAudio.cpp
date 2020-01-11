// License: BSD 3 Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#include "AudioDestinationRtAudio.h"

#include "internal/Assertions.h"
#include "internal/VectorMath.h"

#include "LabSound/core/AudioIOCallback.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/extended/Logging.h"

#include "RtAudio.h"

namespace lab
{

static int NumDefaultOutputChannels()
{
    RtAudio audio;
    uint32_t n = audio.getDeviceCount();

    uint32_t i = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        RtAudio::DeviceInfo info(audio.getDeviceInfo(i));
        if (info.isDefaultOutput)
        {
            //printf("%d channels\n", info.outputChannels);
            return info.outputChannels;
        }
    }
    return 2;
}

const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;
const bool kInterleaved = false;

AudioDestination * AudioDestination::MakePlatformAudioDestination(AudioIOCallback & callback,
                                                                  uint32_t numberOfInputChannels,
                                                                  uint32_t numberOfOutputChannels,
                                                                  float sampleRate)
{
    return new AudioDestinationRtAudio(callback, numberOfInputChannels, numberOfOutputChannels, sampleRate);
}

unsigned long AudioDestination::maxChannelCount()
{
    return NumDefaultOutputChannels();
}

AudioDestinationRtAudio::AudioDestinationRtAudio(AudioIOCallback & callback,
                                                 uint32_t numInputChannels, uint32_t numOutputChannels, float sampleRate)
    : _callback(callback)
    , _numOutputChannels(numOutputChannels)
    , _numInputChannels(numInputChannels)
    , _sampleRate(sampleRate)
{
    configure();
}

AudioDestinationRtAudio::~AudioDestinationRtAudio()
{
    //dac.release(); // XXX
    if (_dac.isStreamOpen())
        _dac.closeStream();

    delete _renderBus;
    delete _inputBus;
}

void AudioDestinationRtAudio::configure()
{
    if (_dac.getDeviceCount() < 1)
    {
        LOG_ERROR("No audio devices available");
    }

    _dac.showWarnings(true);

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = _dac.getDefaultOutputDevice();
    outputParams.nChannels = _numOutputChannels;
    outputParams.firstChannel = 0;
    auto outDeviceInfo = _dac.getDeviceInfo(outputParams.deviceId);

    LOG("Using Default Audio Device: %s", outDeviceInfo.name.c_str());

    // Note! RtAudio has a hard limit on a power of two buffer size, non-power of two sizes will result in
    // heap corruption, for example, when dac.stopStream() is invoked.
    unsigned int bufferFrames = 128;

    RtAudio::StreamParameters inputParams;
    inputParams.deviceId = _dac.getDefaultInputDevice();
    inputParams.nChannels = 1;
    inputParams.firstChannel = 0;
    if (_numInputChannels > 0)
    {
        auto inDeviceInfo = _dac.getDeviceInfo(inputParams.deviceId);
        if (inDeviceInfo.probed && inDeviceInfo.inputChannels > 0)
        {
            // ensure that the number of input channels buffered does not exceed the number available.
            _numInputChannels = inDeviceInfo.inputChannels < _numInputChannels ? inDeviceInfo.inputChannels : _numInputChannels;
        }
    }

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_MINIMIZE_LATENCY;
    if (!kInterleaved)
        options.flags |= RTAUDIO_NONINTERLEAVED;

    try
    {
        _dac.openStream(
            outDeviceInfo.probed && outDeviceInfo.isDefaultOutput ? &outputParams : nullptr,
            _numInputChannels > 0 ? &inputParams : nullptr,
            RTAUDIO_FLOAT32,
            static_cast<unsigned int>(_sampleRate), &bufferFrames, &outputCallback, this, &options);
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
    }
}

void AudioDestinationRtAudio::start()
{
    try
    {
        _dac.startStream();
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
    }
}

void AudioDestinationRtAudio::stop()
{
    try
    {
        _dac.stopStream();
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
    }
}

// Pulls on our provider to get rendered audio stream.
void AudioDestinationRtAudio::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
{
    float * myOutputBufferOfFloats = (float *) outputBuffer;
    float * myInputBufferOfFloats = (float *) inputBuffer;

    if (_numOutputChannels)
    {
        if (!_renderBus || _renderBus->length() < numberOfFrames)
        {
            delete _renderBus;
            _renderBus = new AudioBus(_numOutputChannels, numberOfFrames, true);
            ASSERT(_renderBus);
            _renderBus->setSampleRate(_sampleRate);
        }
    }

    if (_numInputChannels)
    {
        if (!_inputBus || _inputBus->length() < numberOfFrames)
        {
            delete _inputBus;
            _inputBus = new AudioBus(_numInputChannels, numberOfFrames, true);
            ASSERT(_inputBus);
            _inputBus->setSampleRate(_sampleRate);
        }
    }

    if (_numInputChannels)
    {
        // copy the input buffer into channels
        if (kInterleaved)
            for (uint32_t i = 0; i < _numInputChannels; ++i)
            {
                AudioChannel * channel = _inputBus->channel(i);
                float * src = &myInputBufferOfFloats[i];
                VectorMath::vclip(src, 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), _numInputChannels, numberOfFrames);
            }
        else
            for (uint32_t i = 0; i < _numInputChannels; ++i)
            {
                AudioChannel * channel = _inputBus->channel(i);
                float * src = &myInputBufferOfFloats[i * numberOfFrames];
                VectorMath::vclip(src, 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
            }
    }

    // render the output
    _callback.render(_inputBus, _renderBus, numberOfFrames);

    if (_numOutputChannels)
    {
        // copy the rendered audio to the destination
        if (kInterleaved)
            for (uint32_t i = 0; i < _numOutputChannels; ++i)
            {
                AudioChannel * channel = _renderBus->channel(i);
                float * dst = &myOutputBufferOfFloats[i];
                // Clamp values at 0db (i.e., [-1.0, 1.0]) and also copy result to the DAC output buffer
                VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, dst, _numOutputChannels, numberOfFrames);
            }
        else
            for (uint32_t i = 0; i < _numOutputChannels; ++i)
            {
                AudioChannel * channel = _renderBus->channel(i);
                float * dst = &myOutputBufferOfFloats[i * numberOfFrames];
                // Clamp values at 0db (i.e., [-1.0, 1.0]) and also copy result to the DAC output buffer
                VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, dst, 1, numberOfFrames);
            }
    }
}

int outputCallback(void * outputBuffer, void * inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void * userData)
{
    AudioDestinationRtAudio * self = reinterpret_cast<AudioDestinationRtAudio *>(userData);
    float * fBufOut = (float *) outputBuffer;

    // Buffer is nBufferFrames * channels
    // NB: channel count should be set in a principled way
    memset(fBufOut, 0, sizeof(float) * nBufferFrames * self->outputChannelCount());

    AudioDestinationRtAudio * audioDestination = static_cast<AudioDestinationRtAudio *>(userData);
    audioDestination->render(nBufferFrames, fBufOut, inputBuffer);
    return 0;
}

}  // namespace lab
