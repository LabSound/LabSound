// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/ml/AudioDestinationMl.h"
#include "internal/VectorMath.h"

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioIOCallback.h"
#include "LabSound/extended/Logging.h"

namespace lab
{

const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;

void outputBufferCallback(MLHandle handle, void *callback_context) {
    AudioDestinationMl *audioDestination = (AudioDestinationMl *)callback_context;

    MLAudioBuffer outputMlBuffer;
    MLResult result = MLAudioGetOutputStreamBuffer(
      audioDestination->outputHandle,
      &outputMlBuffer 
    );
    if (result != MLResult_Ok) {
      std::cerr << "failed to get ml output buffer" << std::endl;
    }

    MLAudioBuffer inputMlBuffer;
    if (audioDestination->isRecording()) {
      result = MLAudioGetInputStreamBuffer(
        audioDestination->inputHandle,
        &inputMlBuffer
      );
      if (result != MLResult_Ok) {
        std::cerr << "failed to get ml output buffer" << std::endl;
      }

      for (size_t i = 0; i < audioDestination->inputBuffer.size(); i++) {
        audioDestination->inputBuffer[i] = (float)(((uint16_t *)inputMlBuffer.ptr)[i]) / 32768.0f;
      }
    } else {
      memset(audioDestination->inputBuffer.data(), 0, sizeof(float) * audioDestination->inputBuffer.size());
    }

    audioDestination->render(audioDestination->outputBuffer.size(), audioDestination->outputBuffer.data(), audioDestination->inputBuffer.data());

    for (size_t i = 0; i < audioDestination->outputBuffer.size(); i++) {
      ((uint16_t *)outputMlBuffer.ptr)[i] = (uint16_t)(audioDestination->outputBuffer[i] * 32768.0f);
    }

    result = MLAudioReleaseOutputStreamBuffer(audioDestination->outputHandle);
    if (result != MLResult_Ok) {
      std::cerr << "failed to release ml output buffer" << std::endl;
    }

    if (audioDestination->isRecording()) {
      result = MLAudioReleaseInputStreamBuffer(audioDestination->inputHandle);
      if (result != MLResult_Ok) {
        std::cerr << "failed to release ml input buffer" << std::endl;
      }
    }
}

void inputBufferCallback(MLHandle handle, void *callback_context) {
  // nothing
}

AudioDestination * AudioDestination::MakePlatformAudioDestination(AudioIOCallback & callback, unsigned numberOfOutputChannels, float sampleRate)
{
    return new AudioDestinationMl(callback, sampleRate);
}

unsigned long AudioDestination::maxChannelCount()
{
    return 2;
}

AudioDestinationMl::AudioDestinationMl(AudioIOCallback & callback, float sampleRate) : m_callback(callback)
{
    outputAudioBufferFormat.bits_per_sample = 16;
    outputAudioBufferFormat.channel_count = 2;
    outputAudioBufferFormat.sample_format = MLAudioSampleFormat_Int;
    outputAudioBufferFormat.samples_per_second = (uint32_t)sampleRate;
    outputAudioBufferFormat.valid_bits_per_sample = 16;

    nBufferFrames = (uint32_t)sampleRate / 10;

    outputBuffer = std::vector<float>(nBufferFrames);
    inputBuffer = std::vector<float>(nBufferFrames);

    MLResult result = MLAudioCreateSoundWithOutputStream(
      &outputAudioBufferFormat,
      nBufferFrames * sizeof(uint16_t),
      outputBufferCallback,
      this,
      &outputHandle 
    );
    if (result != MLResult_Ok) {
      std::cerr << "failed to create ml output sound" << std::endl;
    }

    inputAudioBufferFormat.bits_per_sample = 16;
    inputAudioBufferFormat.channel_count = 1;
    inputAudioBufferFormat.sample_format = MLAudioSampleFormat_Int;
    inputAudioBufferFormat.samples_per_second = (uint32_t)sampleRate;
    inputAudioBufferFormat.valid_bits_per_sample = 16;

    result = MLAudioCreateInputFromVoiceComm(
      &inputAudioBufferFormat,
      nBufferFrames * sizeof(uint16_t),
      inputBufferCallback,
      nullptr,
      &inputHandle
    );
    if (result != MLResult_Ok) {
      std::cerr << "failed to create ml microphone input" << std::endl;
    }

    m_sampleRate = sampleRate;
    m_renderBus.setSampleRate(m_sampleRate);
    // configure();
}

AudioDestinationMl::~AudioDestinationMl()
{
    // dac.release(); // XXX
    /* if (dac.isStreamOpen())
        dac.closeStream(); */
}

/* void AudioDestinationMl::configure()
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
} */

void AudioDestinationMl::start()
{
    MLResult result = MLAudioStartSound(outputHandle);

    if (result == MLResult_Ok) {
      m_isPlaying = true;
    } else {
      std::cerr << "failed to start ml output sound" << std::endl;
    }
}

void AudioDestinationMl::stop()
{
    MLResult result = MLAudioStopSound(outputHandle);

    if (result == MLResult_Ok) {
      m_isPlaying = false;
    } else {
      std::cerr << "failed to stop ml output sound" << std::endl;
    }
}

void AudioDestinationMl::startRecording()
{
    MLResult result = MLAudioStartSound(inputHandle);

    if (result == MLResult_Ok) {
      m_isRecording = true;
    } else {
      std::cerr << "failed to start ml input" << std::endl;
    }
}

void AudioDestinationMl::stopRecording()
{
    MLResult result = MLAudioStopSound(inputHandle);

    if (result == MLResult_Ok) {
      m_isRecording = false;
    } else {
      std::cerr << "failed to stop ml input" << std::endl;
    }
}

// Pulls on our provider to get rendered audio stream.
void AudioDestinationMl::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
{
    float *myOutputBufferOfFloats = (float*) outputBuffer;
    float *myInputBufferOfFloats = (float*) inputBuffer;

    // Inform bus to use an externally allocated buffer from rtaudio
    m_renderBus.setChannelMemory(0, myOutputBufferOfFloats, numberOfFrames);
    m_renderBus.setChannelMemory(1, myOutputBufferOfFloats + (numberOfFrames), numberOfFrames);

    m_inputBus.setChannelMemory(0, myInputBufferOfFloats, numberOfFrames);

    // Source Bus :: Destination Bus 
    m_callback.render(&m_inputBus, &m_renderBus, numberOfFrames);

    // Clamp values at 0db (i.e., [-1.0, 1.0])
    for (unsigned i = 0; i < m_renderBus.numberOfChannels(); ++i)
    {
        AudioChannel * channel = m_renderBus.channel(i);
        VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
    }
}

} // namespace lab
