// License: BSD 3 Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#include "AudioDestinationMiniaudio.h"
#include "internal/VectorMath.h"

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioIOCallback.h"
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
    // default to no input for now
    return new AudioDestinationMiniaudio(callback, 0, static_cast<unsigned int>(numberOfOutputChannels), sampleRate);
}

unsigned long AudioDestination::maxChannelCount()
{
    return NumDefaultOutputChannels();
}

AudioDestinationMiniaudio::AudioDestinationMiniaudio(AudioIOCallback & callback,
                                                     uint32_t numInputChannels,
                                                     uint32_t numChannels,
                                                     float sampleRate)
: m_callback(callback)
, m_numChannels(numChannels)
, m_numInputChannels(numInputChannels)
, m_sampleRate(sampleRate)
{
    configure();
}

AudioDestinationMiniaudio::~AudioDestinationMiniaudio()
{
    ma_device_uninit(&device);
    delete m_renderBus;
    delete m_inputBus;
}

namespace
{

    void outputCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
    {
        // Buffer is nBufferFrames * channels, interleaved
        float * fBufOut = (float *) pOutput;
        AudioDestinationMiniaudio * audioDestination = reinterpret_cast<AudioDestinationMiniaudio *>(pDevice->pUserData);
        memset(fBufOut, 0, sizeof(float) * frameCount * audioDestination->channelCount());
        audioDestination->render(frameCount, pOutput, const_cast<void*>(pInput));
    }

}

void AudioDestinationMiniaudio::configure()
{
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = m_numChannels;
    deviceConfig.sampleRate = static_cast<uint32_t>(m_sampleRate);
    deviceConfig.dataCallback = outputCallback;
    deviceConfig.performanceProfile = ma_performance_profile_low_latency;
    deviceConfig.pUserData = this;
    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to open audio playback device");
        return;
    }
}

void AudioDestinationMiniaudio::start()
{
    if (ma_device_start(&device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to start audio device");
        return;
    }
}

void AudioDestinationMiniaudio::stop()
{
    if (ma_device_stop(&device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to stop audio device");
        return;
    }
}


const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;

// Pulls on our provider to get rendered audio stream.
void AudioDestinationMiniaudio::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
{
    if (!m_renderBus || m_renderBus->length() < numberOfFrames)
    {
        if (m_renderBus)
            delete m_renderBus;

        m_renderBus = new AudioBus(m_numChannels, numberOfFrames);
        m_renderBus->setSampleRate(m_sampleRate);
    }

    float *myInputBufferOfFloats = (float*) inputBuffer;

#if 0
    if (m_inputBus)
    {
        /// @TODO all the code for input!!!
    }
#endif

    // Source Bus :: Destination Bus
    m_callback.render(m_inputBus, m_renderBus, numberOfFrames);

    // miniaudio expects interleaved data
    float* pOut = static_cast<float*>(outputBuffer);
    size_t c = m_renderBus->numberOfChannels();
    for (size_t i = 0; i < c; ++i)
    {
        const float* data = m_renderBus->channel(i)->data();
        for (int j = 0; j < numberOfFrames; ++j)
            pOut[i * c + j] = data[j];
    }
#if 0
    // miniaudio does its own clipping

    // Clamp values at 0db (i.e., [-1.0, 1.0])
    for (unsigned i = 0; i < m_renderBus->numberOfChannels(); ++i)
    {
        AudioChannel * channel = m_renderBus->channel(i);
        VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
    }
#endif
}


} // namespace lab
