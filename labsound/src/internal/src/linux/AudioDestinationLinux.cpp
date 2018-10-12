// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/linux/AudioDestinationLinux.h"
#include "internal/VectorMath.h"

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioIOCallback.h"
#include "LabSound/extended/Logging.h"

namespace lab
{

const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;

AudioDestination * AudioDestination::MakePlatformAudioDestination(AudioIOCallback & callback, unsigned numberOfOutputChannels, float sampleRate)
{
    return new AudioDestinationLinux(callback, sampleRate);
}

unsigned long AudioDestination::maxChannelCount()
{
    return 2;
}

AudioDestinationLinux::AudioDestinationLinux(AudioIOCallback & callback, float sampleRate) : m_callback(callback)
{
    m_sampleRate = sampleRate;
    m_renderBus.setSampleRate(m_sampleRate);
    dac.reset(new RtAudio()); // XXX
    configure();
}

AudioDestinationLinux::~AudioDestinationLinux()
{
    dac.release(); // XXX
    /* if (dac.isStreamOpen())
        dac.closeStream(); */
}

void AudioDestinationLinux::configure()
{
    if (dac->getDeviceCount() < 1)
    {
        LOG_ERROR("No audio devices available");
    }

    dac->showWarnings(true);

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = dac->getDefaultOutputDevice();
    outputParams.nChannels = 2;
    outputParams.firstChannel = 0;

	auto deviceInfo = dac->getDeviceInfo(outputParams.deviceId);
	LOG("Using Default Audio Device: %s", deviceInfo.name.c_str());

    RtAudio::StreamParameters inputParams;
    inputParams.deviceId = dac->getDefaultInputDevice();
    inputParams.nChannels = 1;
    inputParams.firstChannel = 0;

    unsigned int bufferFrames = AudioNode::ProcessingSizeInFrames;

    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;

    try
    {
        dac->openStream(&outputParams, &inputParams, RTAUDIO_FLOAT32, (unsigned int) m_sampleRate, &bufferFrames, &outputCallback, this, &options);
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
    }
}

void AudioDestinationLinux::start()
{
    try
    {
        dac->startStream();
        m_isPlaying = true;
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
    }
}

void AudioDestinationLinux::stop()
{
    try
    {
        // dac->stopStream(); // XXX
        m_isPlaying = false;
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
    }
}

void AudioDestinationLinux::startRecording()
{
  m_isRecording = true;
}

void AudioDestinationLinux::stopRecording()
{
  m_isRecording = false;
}

// Pulls on our provider to get rendered audio stream.
void AudioDestinationLinux::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
{
    float *myOutputBufferOfFloats = (float*) outputBuffer;
    float *myInputBufferOfFloats = (float*) inputBuffer;

    // Inform bus to use an externally allocated buffer from rtaudio
    if (m_renderBus.isFirstTime())
    {
        m_renderBus.setChannelMemory(0, myOutputBufferOfFloats, numberOfFrames);
        m_renderBus.setChannelMemory(1, myOutputBufferOfFloats + (numberOfFrames), numberOfFrames);
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
    memset(fBufOut, 0, sizeof(float) * nBufferFrames * 2);

    AudioDestinationLinux * audioDestination = static_cast<AudioDestinationLinux*>(userData);

    audioDestination->render(nBufferFrames, fBufOut, inputBuffer);

    return 0;
}

} // namespace lab
