// SPDX-License-Identifier: BSD-3-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#include "AudioDevice_Miniaudio.h"

#include "internal/Assertions.h"
#include "internal/VectorMath.h"

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"

#include "LabSound/extended/Logging.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace lab
{

////////////////////////////////////////////////////
//   Platform/backend specific static functions   //
////////////////////////////////////////////////////

const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;
const int kRenderQuantum = 128;

/// @TODO - the AudioDeviceInfo wants support sample rates, but miniaudio only tells min and max
///         miniaudio also has a concept of minChannels, which LabSound ignores

std::vector<AudioDeviceInfo> AudioDevice::MakeAudioDeviceList()
{
    static std::vector<AudioDeviceInfo> s_devices;
    static bool probed = false;
    if (probed)
        return s_devices;

    probed = true;

    ma_result result;
    ma_context context;
    ma_device_info* pPlaybackDeviceInfos;
    ma_uint32 playbackDeviceCount;
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;
    ma_uint32 iDevice;

    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        LOG_ERROR("Failed to initialize miniaudio context");
        return {};
    }

    result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
    if (result != MA_SUCCESS) {
        LOG_ERROR("Failed to retrieve audio device information.\n");
        return {};
    }

    for (iDevice = 0; iDevice < playbackDeviceCount; ++iDevice) 
    {
        AudioDeviceInfo lab_device_info;
        lab_device_info.index = iDevice;
        lab_device_info.identifier = pPlaybackDeviceInfos[iDevice].name;
        lab_device_info.num_output_channels = pPlaybackDeviceInfos[iDevice].maxChannels;
        lab_device_info.num_input_channels = 0;
        lab_device_info.supported_samplerates.push_back(static_cast<float>(pPlaybackDeviceInfos[iDevice].minSampleRate));
        lab_device_info.supported_samplerates.push_back(static_cast<float>(pPlaybackDeviceInfos[iDevice].maxSampleRate));
        lab_device_info.nominal_samplerate = 48000;
        lab_device_info.is_default_output = iDevice == 0;
        lab_device_info.is_default_input = false;

        s_devices.push_back(lab_device_info);
    }

    for (iDevice = 0; iDevice < captureDeviceCount; ++iDevice) {
        AudioDeviceInfo lab_device_info;
        lab_device_info.index = iDevice;
        lab_device_info.identifier = pCaptureDeviceInfos[iDevice].name;
        lab_device_info.num_output_channels = pCaptureDeviceInfos[iDevice].maxChannels;
        lab_device_info.num_input_channels = 0;
        lab_device_info.supported_samplerates.push_back(static_cast<float>(pCaptureDeviceInfos[iDevice].minSampleRate));
        lab_device_info.supported_samplerates.push_back(static_cast<float>(pCaptureDeviceInfos[iDevice].maxSampleRate));
        lab_device_info.nominal_samplerate = 48000;
        lab_device_info.is_default_output = false;
        lab_device_info.is_default_input = iDevice == 0;

        s_devices.push_back(lab_device_info);
    }

    ma_context_uninit(&context);
    return s_devices;
}


uint32_t AudioDevice::GetDefaultOutputAudioDeviceIndex()
{
    auto devices = MakeAudioDeviceList();
    size_t c = devices.size();
    for (uint32_t i = 0; i < c; ++i)
        if (devices[i].is_default_output)
            return i;

    throw std::runtime_error("no rtaudio devices available!");
}

uint32_t AudioDevice::GetDefaultInputAudioDeviceIndex()
{
    auto devices = MakeAudioDeviceList();
    size_t c = devices.size();
    for (uint32_t i = 0; i < c; ++i)
        if (devices[i].is_default_input)
            return i;

    throw std::runtime_error("no rtaudio devices available!");
}



namespace
{
    void outputCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
    {
        // Buffer is nBufferFrames * channels, interleaved
        float* fBufOut = (float*)pOutput;
        AudioDevice_Miniaudio* ad = reinterpret_cast<AudioDevice_Miniaudio*>(pDevice->pUserData);
        memset(fBufOut, 0, sizeof(float) * frameCount * ad->outputConfig.desired_channels);
        ad->render(frameCount, pOutput, const_cast<void*>(pInput));
    }
}

AudioDevice * AudioDevice::MakePlatformSpecificDevice(AudioDeviceRenderCallback & callback, 
    const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig)
{
    return new AudioDevice_Miniaudio(callback, outputConfig, inputConfig);
}

AudioDevice_Miniaudio::AudioDevice_Miniaudio(AudioDeviceRenderCallback & callback, 
    const AudioStreamConfig _outputConfig, const AudioStreamConfig _inputConfig) 
        : _callback(callback), outputConfig(_outputConfig), inputConfig(_inputConfig)
{
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_duplex);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = outputConfig.desired_channels;
    deviceConfig.sampleRate = static_cast<int>(outputConfig.desired_samplerate);
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = inputConfig.desired_channels;
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

    _ring = new RingBufferT<float>();
    _ring->resize(static_cast<int>(_sampleRate));  // ad hoc. hold one second
    _scratch = reinterpret_cast<float *>(malloc(sizeof(float) * kRenderQuantum * _numInputChannels));
}

AudioDevice_Miniaudio::~AudioDevice_Miniaudio()
{
    stop();
    ma_device_uninit(&_device);
    delete _renderBus;
    delete _inputBus;
    delete _ring;
    if (_scratch)
        free(_scratch);
}


void AudioDevice_Miniaudio::start()
{
    ASSERT(authoritativeDeviceSampleRateAtRuntime != 0.f); // something went very wrong
    samplingInfo.epoch[0] = samplingInfo.epoch[1] = std::chrono::high_resolution_clock::now();
    if (ma_device_start(&_device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to start audio device");
    }
}

void AudioDevice_Miniaudio::stop()
{
    if (ma_device_stop(&_device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to stop audio device");
    }
    samplingInfo = {};
}

// Pulls on our provider to get rendered audio stream.
void AudioDevice_Miniaudio::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
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

    if (numberOfFrames * _numInputChannels)
        _ring->write(pIn, numberOfFrames * _numInputChannels);

    while (numberOfFrames > 0)
    {
        if (_remainder > 0)
        {
            // copy samples to output buffer. There might have been some rendered frames
            // left over from the previous numberOfFrames, so start by moving those into
            // the output buffer.

            int samples = _remainder < numberOfFrames ? _remainder : numberOfFrames;
            for (unsigned int i = 0; i < _numChannels; ++i)
            {
                AudioChannel * channel = _renderBus->channel(i);
                VectorMath::vclip(channel->data() + static_cast<ptrdiff_t>(kRenderQuantum - _remainder), 1,
                                  &kLowThreshold, &kHighThreshold,
                                  pOut + i, _numChannels, samples);
            }
            pOut += static_cast<ptrdiff_t>(_numChannels * samples);

            // deduct the output samples from the desired copy amount, and from the remaining
            // samples from the last invocation of _callback.render().
            numberOfFrames -= samples;
            _remainder -= samples;
        }
        else
        {
            if (_numInputChannels)
            {
                _ring->read(_scratch, _numInputChannels * kRenderQuantum);
                for (unsigned int i = 0; i < _numInputChannels; ++i)
                {
                    AudioChannel * channel = _inputBus->channel(i);
                    VectorMath::vclip(_scratch + i, _numInputChannels,
                                      &kLowThreshold, &kHighThreshold,
                                      channel->mutableData(), 1, kRenderQuantum);
                }
            }

            // generate new data
            _callback.render(_inputBus, _renderBus, kRenderQuantum, samplingInfo);
            _remainder = kRenderQuantum;
        }
    }

    // Update sampling info
    const int32_t index = 1 - (samplingInfo.current_sample_frame & 1);
    const uint64_t t = samplingInfo.current_sample_frame & ~1;
    samplingInfo.sampling_rate = authoritativeDeviceSampleRateAtRuntime;
    samplingInfo.current_sample_frame = t + numberOfFrames + index;
    samplingInfo.current_time = samplingInfo.current_sample_frame / static_cast<double>(samplingInfo.sampling_rate);
    samplingInfo.epoch[index] = std::chrono::high_resolution_clock::now();
}

}  // namespace lab
