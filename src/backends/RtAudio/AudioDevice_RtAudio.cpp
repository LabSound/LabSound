// SPDX-License-Identifier: BSD-3-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#include "LabSound/backends/AudioDevice_RtAudio.h"

#include "internal/Assertions.h"

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNode.h"

#include "LabSound/extended/Logging.h"
#include "LabSound/extended/VectorMath.h"

#include "RtAudio.h"

namespace lab
{

// there can only be one audio context. More than one introduces data races
// in internal RtAudio mutexes.

static RtAudio* g_rtaudio_ctx = nullptr;

int rt_audio_callback(
    void * outputBuffer, void * inputBuffer,
    unsigned int nBufferFrames, double streamTime,
    RtAudioStreamStatus status, void * userData)
{
    AudioDevice_RtAudio * device = reinterpret_cast<AudioDevice_RtAudio *>(userData);
    float * fltOutputBuffer = reinterpret_cast<float *>(outputBuffer);
    memset(fltOutputBuffer, 0, nBufferFrames * device->getOutputConfig().desired_channels * sizeof(float));
    device->render(device->sourceProvider(), nBufferFrames, fltOutputBuffer, inputBuffer);
    return 0;
}

// static
std::vector<AudioDeviceInfo>
AudioDevice_RtAudio::MakeAudioDeviceList() 
{
    std::vector<std::string> rt_audio_apis {
        "unspecified",
        "linux_alsa",
        "linux_pulse",
        "linux_oss",
        "unix_jack",
        "macos_coreaudio",
        "windows_wasapi",
        "windows_asio",
        "windows_directsound",
        "rtaudio_dummy" };

    if (!g_rtaudio_ctx)
        g_rtaudio_ctx = new RtAudio();

    if (g_rtaudio_ctx->getDeviceCount() <= 0)
        throw std::runtime_error("no rtaudio devices available!");

    const auto api_enum = g_rtaudio_ctx->getCurrentApi();
    LOG_INFO("using rtaudio api %s", rt_audio_apis[static_cast<int>(api_enum)].c_str());

    auto to_flt_vec = [](const std::vector<unsigned int> & vec) {
        std::vector<float> result;
        for (auto & i : vec) result.push_back(static_cast<float>(i));
        return result;
    };

    std::vector<AudioDeviceInfo> devices;
    for (uint32_t i = 0; i < g_rtaudio_ctx->getDeviceCount(); ++i)
    {
        RtAudio::DeviceInfo info = g_rtaudio_ctx->getDeviceInfo(i);

        if (info.probed)
        {
            AudioDeviceInfo lab_device_info;
            lab_device_info.index = i;
            lab_device_info.identifier = info.name;
            lab_device_info.num_output_channels = info.outputChannels;
            lab_device_info.num_input_channels = info.inputChannels;
            lab_device_info.supported_samplerates = to_flt_vec(info.sampleRates);
            lab_device_info.nominal_samplerate = static_cast<float>(info.preferredSampleRate);
            lab_device_info.is_default_output = info.isDefaultOutput;
            lab_device_info.is_default_input = info.isDefaultInput;

            devices.push_back(lab_device_info);
        }
        else
        {
            LOG_ERROR("probing failed %s", info.name.c_str());
        }
    }

    return devices;
}


/////////////////////////////
//   AudioDevice_RtAudio   //
/////////////////////////////

const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;
const bool kInterleaved = false;

void AudioDevice_RtAudio::createContext()
{
    if (!g_rtaudio_ctx)
        g_rtaudio_ctx = new RtAudio();
    
    if (g_rtaudio_ctx->getDeviceCount() < 1)
    {
        LOG_ERROR("no audio devices available");
    }
    
    // Ensure that input and output sample rates match
    if (_inConfig.device_index != -1 && _outConfig.device_index != -1)
    {
        ASSERT(_inConfig.desired_samplerate == _outConfig.desired_samplerate);
    }


    g_rtaudio_ctx->showWarnings(true);

    // Translate AudioStreamConfig into RTAudio-native data structures

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = _outConfig.device_index;
    outputParams.nChannels = _outConfig.desired_channels;
    LOG_INFO("using output device idx: %i", _outConfig.device_index);
    if (_outConfig.device_index >= 0)
        LOG_INFO("using output device name: %s", g_rtaudio_ctx->getDeviceInfo(outputParams.deviceId).name.c_str());

    RtAudio::StreamParameters inputParams;
    inputParams.deviceId = _inConfig.device_index;
    inputParams.nChannels = _inConfig.desired_channels;
    LOG_INFO("using input device idx: %i", _inConfig.device_index);
    if (_inConfig.device_index >= 0)
        LOG_INFO("using input device name: %s", g_rtaudio_ctx->getDeviceInfo(inputParams.deviceId).name.c_str());

    authoritativeDeviceSampleRateAtRuntime = _outConfig.desired_samplerate;

    if (_outConfig.desired_channels > 0 && _outConfig.desired_channels > 0) {
        if (_outConfig.desired_samplerate != _outConfig.desired_samplerate) {
            throw std::runtime_error("RtAudio can't handle differing input and output rates");
        }
    }

    if (_inConfig.desired_channels > 0)
    {
        auto inDeviceInfo = g_rtaudio_ctx->getDeviceInfo(inputParams.deviceId);
        if (inDeviceInfo.probed && inDeviceInfo.inputChannels > 0)
        {
            // ensure that the number of input channels buffered does not exceed the number available.
            inputParams.nChannels = (inDeviceInfo.inputChannels < _inConfig.desired_channels) ?
                inDeviceInfo.inputChannels : _inConfig.desired_channels;
            _inConfig.desired_channels = inputParams.nChannels;
            LOG_INFO("[AudioDevice_RtAudio] adjusting number of input channels: %i ", inputParams.nChannels);
        }
        authoritativeDeviceSampleRateAtRuntime = _inConfig.desired_samplerate;
    }

    RtAudio::StreamOptions options;
    // RTAUDIO_MINIMIZE_LATENCY tells RtAudio to use the hardware's minimum buffer size
    // which is not desirable as the minimum way be too small, and a non-power of 2.
    //options.flags = RTAUDIO_MINIMIZE_LATENCY;
    if (!kInterleaved) options.flags |= RTAUDIO_NONINTERLEAVED;

    // Note! RtAudio has a hard limit on a power of two buffer size, non-power of two sizes will result in
    // heap corruption, for example, when dac.stopStream() is invoked.
    uint32_t bufferFrames = AudioNode::ProcessingSizeInFrames;

    samplingInfo.epoch[0] = samplingInfo.epoch[1] = std::chrono::high_resolution_clock::now();

    try
    {
        g_rtaudio_ctx->openStream(
                (outputParams.nChannels > 0) ? &outputParams : nullptr,
                (inputParams.nChannels > 0) ? &inputParams : nullptr,
                RTAUDIO_FLOAT32,
                static_cast<unsigned int>(authoritativeDeviceSampleRateAtRuntime),
                &bufferFrames,
                &rt_audio_callback,
                this,
                &options);
    }
    catch (const RtAudioError & e)
    {
        LOG_ERROR(e.getMessage().c_str());
    }
}

AudioDevice_RtAudio::AudioDevice_RtAudio(
    const AudioStreamConfig & _inputConfig,
    const AudioStreamConfig & _outputConfig)
: AudioDevice(_inputConfig, _outputConfig)
{
    createContext();
}

AudioDevice_RtAudio::~AudioDevice_RtAudio()
{
    if (g_rtaudio_ctx) {
        if (g_rtaudio_ctx->isStreamOpen())
            g_rtaudio_ctx->closeStream();

        delete g_rtaudio_ctx;
        g_rtaudio_ctx = nullptr;
    }
}

void AudioDevice_RtAudio::backendReinitialize()
{
    if (g_rtaudio_ctx) {
        if (g_rtaudio_ctx->isStreamOpen())
            g_rtaudio_ctx->closeStream();

        delete g_rtaudio_ctx;
        g_rtaudio_ctx = nullptr;
    }
    createContext();
}


void AudioDevice_RtAudio::start()
{
    ASSERT(g_rtaudio_ctx);
    ASSERT(authoritativeDeviceSampleRateAtRuntime != 0.f);  // something went very wrong
    try
    {
        if (!g_rtaudio_ctx->isStreamRunning())
            g_rtaudio_ctx->startStream();
    }
    catch (const RtAudioError & e)
    {
        LOG_ERROR(e.getMessage().c_str());
    }
}

void AudioDevice_RtAudio::stop()
{
    ASSERT(g_rtaudio_ctx);
    try
    {
        if (g_rtaudio_ctx->isStreamRunning())
            g_rtaudio_ctx->stopStream();
    }
    catch (const RtAudioError & e)
    {
        LOG_ERROR(e.getMessage().c_str());
    }
}

bool AudioDevice_RtAudio::isRunning() const
{
    ASSERT(g_rtaudio_ctx);
    try
    {
        return g_rtaudio_ctx->isStreamRunning();
    }
    catch (const RtAudioError & e)
    {
        LOG_ERROR(e.getMessage().c_str());
        return false;
    }
}


// called by RtAudio periodically to get audio data.
// Pulls on our provider to get rendered audio stream.
//
void AudioDevice_RtAudio::render(
    AudioSourceProvider* provider,
    int numberOfFrames, void * outputBuffer, void * inputBuffer)
{
    float * fltOutputBuffer = reinterpret_cast<float *>(outputBuffer);
    float * fltInputBuffer = reinterpret_cast<float *>(inputBuffer);

    if (_outConfig.desired_channels)
    {
        if (!_renderBus || _renderBus->length() < numberOfFrames)
        {
            _renderBus.reset(new AudioBus(_outConfig.desired_channels, numberOfFrames, true));
            _renderBus->setSampleRate(authoritativeDeviceSampleRateAtRuntime);
        }
    }

    if (_inConfig.desired_channels)
    {
        if (!_inputBus || _inputBus->length() < numberOfFrames)
        {
            _inputBus.reset(new AudioBus(_inConfig.desired_channels, numberOfFrames, true));
            _inputBus->setSampleRate(authoritativeDeviceSampleRateAtRuntime);
        }
    }

    // copy the input buffer
    if (_inConfig.desired_channels)
    {
        if (kInterleaved)
        {
            for (uint32_t i = 0; i < _inConfig.desired_channels; ++i)
            {
                AudioChannel * channel = _inputBus->channel(i);
                float * src = &fltInputBuffer[i];
                VectorMath::vclip(src, 1, &kLowThreshold, &kHighThreshold,
                                  channel->mutableData(), _inConfig.desired_channels,
                                  numberOfFrames);
            }
        }
        else
        {
            for (uint32_t i = 0; i < _inConfig.desired_channels; ++i)
            {
                AudioChannel * channel = _inputBus->channel(i);
                float * src = &fltInputBuffer[i * numberOfFrames];
                VectorMath::vclip(src, 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
            }
        }
    }

    // Update sampling info
    const int32_t index = 1 - (samplingInfo.current_sample_frame & 1);
    const uint64_t t = samplingInfo.current_sample_frame & ~1;
    samplingInfo.sampling_rate = authoritativeDeviceSampleRateAtRuntime;
    samplingInfo.current_sample_frame = t + numberOfFrames + index;
    samplingInfo.current_time = samplingInfo.current_sample_frame / static_cast<double>(samplingInfo.sampling_rate);
    samplingInfo.epoch[index] = std::chrono::high_resolution_clock::now();

    // Pull on the graph
    auto dn = _destinationNode; // up the ref count
    if (dn)
        dn->render(provider, _inputBus.get(), _renderBus.get(), numberOfFrames, samplingInfo);

    // Then deliver the rendered audio back to rtaudio, ready for the next callback
    if (_outConfig.desired_channels)
    {
        // Clamp values at 0db (i.e., [-1.0, 1.0]) and also copy result to the DAC output buffer
        if (kInterleaved)
        {
            for (uint32_t i = 0; i < _outConfig.desired_channels; ++i)
            {
                AudioChannel * channel = _renderBus->channel(i);
                float * dst = &fltOutputBuffer[i];
                VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, dst,
                                  _outConfig.desired_channels, numberOfFrames);
            }
        }
        else
        {
            for (uint32_t i = 0; i < _outConfig.desired_channels; ++i)
            {
                AudioChannel * channel = _renderBus->channel(i);
                float * dst = &fltOutputBuffer[i * numberOfFrames];
                VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, dst, 1, numberOfFrames);
            }
        }
    }
}

}  // namespace lab
