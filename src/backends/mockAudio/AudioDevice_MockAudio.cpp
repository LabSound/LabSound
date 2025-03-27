// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#define LABSOUND_ENABLE_LOGGING

#include "LabSound/backends/AudioDevice_MockAudio.h"

#include "internal/Assertions.h"

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNode.h"

#include "LabSound/extended/Logging.h"
#include "LabSound/extended/VectorMath.h"

#include <assert.h>
#include <atomic>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

namespace lab
{

////////////////////////////////////////////////////
//   Platform/backend specific static functions   //
////////////////////////////////////////////////////

const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;
const int kRenderQuantum = AudioNode::ProcessingSizeInFrames;

std::vector<AudioDeviceInfo> AudioDevice_Mockaudio::MakeAudioDeviceList()
{
    // Mock device pretends there are no input devices
    // It has one output device (a memory buffer)
    std::vector<AudioDeviceInfo> devices;
    
    AudioDeviceInfo mockDevice;
    mockDevice.index = 0;
    mockDevice.identifier = "Mock Audio Output";
    mockDevice.num_input_channels = 0;
    mockDevice.num_output_channels = 2; // Stereo output
    mockDevice.supported_samplerates.push_back(44100.0f);
    mockDevice.supported_samplerates.push_back(48000.0f);
    mockDevice.nominal_samplerate = 44100.0f;
    mockDevice.is_default_output = true;
    mockDevice.is_default_input = false;
    
    devices.push_back(mockDevice);
    
    return devices;
}

AudioDevice_Mockaudio::AudioDevice_Mockaudio(
    const AudioStreamConfig & outputConfig,
    const AudioStreamConfig & inputConfig,
    size_t bufferSize,
    const std::string& wavFilePath)
    : AudioDevice(inputConfig, outputConfig)
    , _bufferSize(bufferSize)
    , _wavFilePath(wavFilePath)
{
    // Initialize the memory buffer
    _memoryBuffer.resize(bufferSize);
    _currentBufferPosition = 0;
    _bufferFull = false;
    
    // Set the sample rate
    authoritativeDeviceSampleRateAtRuntime = _outConfig.desired_samplerate;
    
    // Initialize sampling info
    samplingInfo.epoch[0] = samplingInfo.epoch[1] = std::chrono::high_resolution_clock::now();
    
    LOG_INFO("MockAudio device initialized with buffer size: %zu, WAV file: %s", 
             bufferSize, wavFilePath.c_str());
}

AudioDevice_Mockaudio::~AudioDevice_Mockaudio()
{
    stop();
    
    // If buffer has data but isn't full, write what we have
    if (_currentBufferPosition > 0 && !_bufferFull) {
        writeToWavFile();
    }
    
    delete _renderBus;
    delete _inputBus;
}

void AudioDevice_Mockaudio::start()
{
    LOG_INFO("MockAudio device started");
}

void AudioDevice_Mockaudio::stop()
{
    LOG_INFO("MockAudio device stopped");
}

bool AudioDevice_Mockaudio::isRunning() const
{
    // Always return true unless buffer is full
    return !_bufferFull;
}

void AudioDevice_Mockaudio::backendReinitialize()
{
    // Nothing to do for mock device
}

// Helper function to write WAV header
void writeWavHeader(std::ofstream& file, int channels, int sampleRate, int bitsPerSample, int dataSize)
{
    // RIFF header
    file.write("RIFF", 4);
    int fileSize = 36 + dataSize;
    file.write(reinterpret_cast<const char*>(&fileSize), 4);
    file.write("WAVE", 4);
    
    // fmt chunk
    file.write("fmt ", 4);
    int fmtChunkSize = 16;
    file.write(reinterpret_cast<const char*>(&fmtChunkSize), 4);
    short audioFormat = 1; // PCM
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&channels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    int byteRate = sampleRate * channels * (bitsPerSample / 8);
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    short blockAlign = channels * (bitsPerSample / 8);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    
    // data chunk
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);
}

void AudioDevice_Mockaudio::writeToWavFile()
{
    LOG_INFO("Writing %zu samples to WAV file: %s", _currentBufferPosition, _wavFilePath.c_str());
    
    std::ofstream file(_wavFilePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open WAV file for writing: %s", _wavFilePath.c_str());
        return;
    }
    
    // Write WAV header
    int channels = 1; // Only first channel
    int sampleRate = static_cast<int>(authoritativeDeviceSampleRateAtRuntime);
    int bitsPerSample = 32; // 32-bit float
    int dataSize = static_cast<int>(_currentBufferPosition * sizeof(float));
    
    writeWavHeader(file, channels, sampleRate, bitsPerSample, dataSize);
    
    // Write audio data
    file.write(reinterpret_cast<const char*>(_memoryBuffer.data()), dataSize);
    
    file.close();
    LOG_INFO("WAV file written successfully");
}

// Pulls on our provider to get rendered audio stream.
int AudioDevice_Mockaudio::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
{
    // If buffer is full, don't process anything
    if (_bufferFull) {
        return 0;
    }
    
    if (!_renderBus) {
        _renderBus = new AudioBus(_outConfig.desired_channels, kRenderQuantum, true);
        _renderBus->setSampleRate(authoritativeDeviceSampleRateAtRuntime);
    }
    
    // We don't use input in mock device
    if (!_inputBus && _inConfig.desired_channels) {
        _inputBus = new AudioBus(_inConfig.desired_channels, kRenderQuantum, true);
        _inputBus->setSampleRate(authoritativeDeviceSampleRateAtRuntime);
    }
    
    float * pOut = static_cast<float *>(outputBuffer);
    int out_channels = _outConfig.desired_channels;
    
    int rendered = 0;
    while (numberOfFrames > 0) {
        if (_remainder > 0) {
            // Copy samples to output buffer
            int samples = _remainder < numberOfFrames ? _remainder : numberOfFrames;
            rendered += samples;
            
            // Copy to output buffer (similar to miniaudio implementation)
            for (int i = 0; i < out_channels; ++i) {
                int src_stride = 1;  // de-interleaved
                int dst_stride = out_channels;  // interleaved
                AudioChannel * channel = _renderBus->channel(i);
                VectorMath::vclip(channel->data() + kRenderQuantum - _remainder, src_stride,
                                  &kLowThreshold, &kHighThreshold,
                                  pOut + i, dst_stride, samples);
            }
            
            // Append only the first channel to our memory buffer
            AudioChannel * firstChannel = _renderBus->channel(0);
            for (int i = 0; i < samples; ++i) {
                if (_currentBufferPosition < _bufferSize) {
                    _memoryBuffer[_currentBufferPosition++] = firstChannel->data()[kRenderQuantum - _remainder + i];
                }
                else {
                    // Buffer is full, write to WAV file and stop processing
                    _bufferFull = true;
                    writeToWavFile();
                    return rendered;
                }
            }
            
            pOut += out_channels * samples;
            numberOfFrames -= samples;
            _remainder -= samples;
        }
        else {
            // Update sampling info for use by the render graph
            const int32_t index = 1 - (samplingInfo.current_sample_frame & 1);
            const uint64_t t = samplingInfo.current_sample_frame & ~1;
            samplingInfo.sampling_rate = authoritativeDeviceSampleRateAtRuntime;
            samplingInfo.current_sample_frame = t + kRenderQuantum + index;
            samplingInfo.current_time = samplingInfo.current_sample_frame / static_cast<double>(samplingInfo.sampling_rate);
            samplingInfo.epoch[index] = std::chrono::high_resolution_clock::now();
            
            // Generate new data
            _destinationNode->render(sourceProvider(), _inputBus, _renderBus, kRenderQuantum, samplingInfo);
            _remainder = kRenderQuantum;
            rendered += kRenderQuantum;
        }
    }
    return rendered;
}

}  // namespace lab
