// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "AudioDestinationWindows.h"
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
    return new AudioDestinationWin(callback, numberOfOutputChannels, sampleRate);
}

unsigned long AudioDestination::maxChannelCount()
{
    return NumDefaultOutputChannels();
}

AudioDestinationWin::AudioDestinationWin(AudioIOCallback & callback, size_t numChannels, float sampleRate)
: m_callback(callback)
, m_renderBus(numChannels, AudioNode::ProcessingSizeInFrames, false)
, m_inputBus(1, AudioNode::ProcessingSizeInFrames, false)
{
    m_numChannels = numChannels;
    m_sampleRate = sampleRate;
    m_renderBus.setSampleRate(m_sampleRate);
    configure();
}

AudioDestinationWin::~AudioDestinationWin()
{
    //dac.release(); // XXX
     if (dac.isStreamOpen())
        dac.closeStream();
}

void AudioDestinationWin::configure()
{
    if (dac.getDeviceCount() < 1)
    {
        LOG_ERROR("No audio devices available");
    }

    dac.showWarnings(true);

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = dac.getDefaultOutputDevice();
    outputParams.nChannels = static_cast<unsigned int>(m_numChannels);
    outputParams.firstChannel = 0;

    auto outDeviceInfo = dac.getDeviceInfo(outputParams.deviceId);
    LOG("Using Default Audio Device: %s", outDeviceInfo.name.c_str());

    RtAudio::StreamParameters inputParams;
    inputParams.deviceId = dac.getDefaultInputDevice();
    inputParams.nChannels = 1;
    inputParams.firstChannel = 0;

    auto inDeviceInfo = dac.getDeviceInfo(outputParams.deviceId);

    unsigned int bufferFrames = AudioNode::ProcessingSizeInFrames;

    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;

    try
    {
        dac.openStream(outDeviceInfo.probed ? &outputParams : nullptr, 
                       inDeviceInfo.probed ? &inputParams : nullptr, 
            RTAUDIO_FLOAT32, 
            (unsigned int) m_sampleRate, &bufferFrames, &outputCallback, this, &options);
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
    }
}

void AudioDestinationWin::start()
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

void AudioDestinationWin::stop()
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
void AudioDestinationWin::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
{
    float *myOutputBufferOfFloats = (float*) outputBuffer;
    float *myInputBufferOfFloats = (float*) inputBuffer;

    // Inform bus to use an externally allocated buffer from rtaudio
    if (m_renderBus.isFirstTime())
    {
        for (uint32_t i = 0; i < m_numChannels; ++i)
        {
            m_renderBus.setChannelMemory(i, myOutputBufferOfFloats + i * numberOfFrames, numberOfFrames);
        }
    }

    if (m_inputBus.isFirstTime())
    {
        m_inputBus.setChannelMemory(0, myInputBufferOfFloats, numberOfFrames);
    }

    // Source Bus :: Destination Bus
    m_callback.render(&m_inputBus, &m_renderBus, numberOfFrames);

    // Clamp values at 0db (i.e., [-1.0, 1.0])
    for (unsigned i = 0; i < m_renderBus.numberOfChannels(); ++i)
    {
        AudioChannel * channel = m_renderBus.channel(i);
        VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
    }
}

int outputCallback(void * outputBuffer, void * inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void * userData)
{
    float *fBufOut = (float*) outputBuffer;

    // Buffer is nBufferFrames * channels
    // NB: channel count should be set in a principled way
    memset(fBufOut, 0, sizeof(float) * nBufferFrames * 2);

    AudioDestinationWin * audioDestination = static_cast<AudioDestinationWin*>(userData);
    audioDestination->render(nBufferFrames, fBufOut, inputBuffer);

    return 0;
}

} // namespace lab
