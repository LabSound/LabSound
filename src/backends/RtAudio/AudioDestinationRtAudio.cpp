// License: BSD 3 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "AudioDestinationRtAudio.h"
#include "internal/VectorMath.h"

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioIOCallback.h"
#include "LabSound/extended/Logging.h"

#include <rtaudio/RtAudio.h>

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

AudioDestination * AudioDestination::MakePlatformAudioDestination(AudioIOCallback & callback, size_t numberOfOutputChannels, float sampleRate)
{
    return new AudioDestinationRtAudio(callback, numberOfOutputChannels,
                                       1, // default to mono for now
                                       sampleRate);
}

unsigned long AudioDestination::maxChannelCount()
{
    return NumDefaultOutputChannels();
}

AudioDestinationRtAudio::AudioDestinationRtAudio(AudioIOCallback & callback,
                                                 uint32_t numChannels, uint32_t numInputChannels, float sampleRate)
: m_callback(callback)
, m_numChannels(numChannels)
, m_numInputChannels(numInputChannels)
, m_sampleRate(sampleRate)
{
    configure();
}

AudioDestinationRtAudio::~AudioDestinationRtAudio()
{
    //dac.release(); // XXX
     if (dac.isStreamOpen())
        dac.closeStream();
}

void AudioDestinationRtAudio::configure()
{
    if (dac.getDeviceCount() < 1)
    {
        LOG_ERROR("No audio devices available");
    }

    dac.showWarnings(true);

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = dac.getDefaultOutputDevice();
    outputParams.nChannels = m_numChannels;
    outputParams.firstChannel = 0;

    auto outDeviceInfo = dac.getDeviceInfo(outputParams.deviceId);
    LOG("Using Default Audio Device: %s", outDeviceInfo.name.c_str());

    RtAudio::StreamParameters inputParams;
    inputParams.deviceId = dac.getDefaultInputDevice();
    inputParams.nChannels = 1;
    inputParams.firstChannel = 0;

    // Note! RtAudio has a hard limit on a power of two buffer size, non-power of two sizes will result in
    // heap corruption, for example, when dac.stopStream() is invoked.
    unsigned int bufferFrames = 128;
    auto inDeviceInfo = dac.getDeviceInfo(inputParams.deviceId);
    if (inDeviceInfo.probed && inDeviceInfo.inputChannels > 0 && m_numInputChannels > 0)
    {
        // ensure that the number of input channels buffered does not exceed the number available.
        m_numInputChannels = inDeviceInfo.inputChannels < m_numInputChannels ? inDeviceInfo.inputChannels : m_numInputChannels;
        m_inputBus = std::make_unique<AudioBus>(m_numInputChannels, bufferFrames, false);
    }

    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;

    try
    {
        dac.openStream(
            outDeviceInfo.probed && outDeviceInfo.isDefaultOutput ? &outputParams : nullptr,
            inDeviceInfo.probed && inDeviceInfo.isDefaultInput ? &inputParams : nullptr,
            RTAUDIO_FLOAT32,
            (unsigned int) m_sampleRate, &bufferFrames, &outputCallback, this, &options
        );
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
        m_inputBus.reset();
    }
}

void AudioDestinationRtAudio::start()
{
    try
    {
        dac.startStream();
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
        dac.stopStream();
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
    }
}

// Pulls on our provider to get rendered audio stream.
void AudioDestinationRtAudio::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
{
    float *myOutputBufferOfFloats = (float*) outputBuffer;
    float *myInputBufferOfFloats = (float*) inputBuffer;

    if (!m_renderBus || m_renderBus->length() < numberOfFrames)
    {
        m_renderBus = std::make_unique<AudioBus>(m_numChannels, numberOfFrames, false);
        m_renderBus->setSampleRate(m_sampleRate);
    }

    if (m_inputBus && m_inputBus->length() < numberOfFrames)
    {
        m_inputBus = std::make_unique<AudioBus>(m_numInputChannels, numberOfFrames, false);
        m_inputBus->setSampleRate(m_sampleRate);
    }

    // Inform bus to use an externally allocated buffer from rtaudio
    if (m_renderBus->isFirstTime())
    {
        for (uint32_t i = 0; i < m_numChannels; ++i)
            m_renderBus->setChannelMemory(i, myOutputBufferOfFloats + i * static_cast<uint32_t>(numberOfFrames), numberOfFrames);
    }

    if (m_inputBus && m_inputBus->isFirstTime())
    {
        for (uint32_t i = 0; i < m_numInputChannels; ++i)
            m_inputBus->setChannelMemory(i, myInputBufferOfFloats + i * static_cast<uint32_t>(numberOfFrames), numberOfFrames);
    }

    // Source Bus :: Destination Bus
    m_callback.render(m_inputBus.get(), m_renderBus.get(), numberOfFrames);

    // Clamp values at 0db (i.e., [-1.0, 1.0])
    for (unsigned i = 0; i < m_renderBus->numberOfChannels(); ++i)
    {
        AudioChannel * channel = m_renderBus->channel(i);
        VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
    }
}

int outputCallback(void * outputBuffer, void * inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void * userData)
{
    float *fBufOut = (float*) outputBuffer;

    // Buffer is nBufferFrames * channels
    // NB: channel count should be set in a principled way
    memset(fBufOut, 0, sizeof(float) * nBufferFrames * 2);

    AudioDestinationRtAudio * audioDestination = static_cast<AudioDestinationRtAudio*>(userData);
    audioDestination->render(nBufferFrames, fBufOut, inputBuffer);

    return 0;
}

} // namespace lab
