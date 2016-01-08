// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include <windows.h>
#include <mmsystem.h>

#include "internal/win/AudioDestinationWin.h"
#include "internal/FloatConversion.h"
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
    return new AudioDestinationWin(callback, sampleRate);
}

unsigned long AudioDestination::maxChannelCount()
{
    return 2;
}

float AudioDestination::hardwareSampleRate()
{
    // Danger: default to 44100
    return 44100;
}

AudioDestinationWin::AudioDestinationWin(AudioIOCallback & callback, float sampleRate) : m_callback(callback)
{
    m_sampleRate = sampleRate;
    m_renderBus.setSampleRate(hardwareSampleRate());
    configure();
}

AudioDestinationWin::~AudioDestinationWin()
{
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

    RtAudio::StreamParameters parameters;
    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = 2;
    parameters.firstChannel = 0;
    unsigned int sampleRate = unsigned int (hardwareSampleRate());

	auto deviceInfo = dac.getDeviceInfo(parameters.deviceId);
	LOG("Using Default Audio Device: %s", deviceInfo.name.c_str());

    unsigned int bufferFrames = AudioNode::ProcessingSizeInFrames;

    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;

    try
    {
        dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &outputCallback, this, &options);
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
        m_isPlaying = true;
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
        m_isPlaying = false;
    }
    catch (RtAudioError & e)
    {
        e.printMessage();
    }
}

// Pulls on our provider to get rendered audio stream.
void AudioDestinationWin::render(int numberOfFrames, void *outputBuffer, void *inputBuffer)
{
    float *myOutputBufferOfFloats = (float*) outputBuffer;

    // Inform bus to use an externally allocated buffer from rtaudio
    if (m_renderBus.isFirstTime())
    {
        m_renderBus.setChannelMemory(0, myOutputBufferOfFloats, numberOfFrames);
        m_renderBus.setChannelMemory(1, myOutputBufferOfFloats + (numberOfFrames), numberOfFrames);
    }

    // Source Bus :: Destination Bus (no source/input)
    m_callback.render(0, &m_renderBus, numberOfFrames);

    // Clamp values at 0db (i.e., [-1.0, 1.0])
    for (unsigned i = 0; i < m_renderBus.numberOfChannels(); ++i)
    {
        AudioChannel* channel = m_renderBus.channel(i);
        VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
    }
}

int outputCallback(void * outputBuffer, void * inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void * userData)
{
    float *fBufOut = (float*) outputBuffer;

    // Buffer is nBufferFrames * channels
    memset(fBufOut, 0, sizeof(float) * nBufferFrames * 2);

    AudioDestinationWin* audioOutput = static_cast<AudioDestinationWin*>(userData);

    audioOutput->render(nBufferFrames, fBufOut, inputBuffer);

    return 0;
}

} // namespace lab
