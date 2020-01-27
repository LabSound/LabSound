// License: BSD 3 Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

<<<<<<< HEAD:src/backends/windows/AudioDestinationWindows.cpp
#include "AudioDestinationWindows.h"
=======
#include "AudioDevice_RtAudio.h"

#include "internal/Assertions.h"
>>>>>>> work on rtaudio backend implementation:src/backends/RtAudio/AudioDevice_RtAudio.cpp
#include "internal/VectorMath.h"

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"

#include "LabSound/extended/Logging.h"

#include "RtAudio.h"
#include "LabSound/LabSound.h"  // for AudioDeviceInfo


namespace lab
{

////////////////////////////////////////////////////
//   Platform/backend specific static functions   //
////////////////////////////////////////////////////

std::vector<AudioDeviceInfo> AudioDevice::MakeAudioDeviceList()
{
    std::vector<std::string> rt_audio_apis
    {
        "unspecified",
        "linux_alsa",
        "linux_pulse",
        "linux_oss",
        "unix_jack",
        "macos_coreaudio",
        "windows_wasapi",
        "windows_asio",
        "windows_directsound",
        "rtaudio_dummy"
    };

    RtAudio rt;
    if (rt.getDeviceCount() <= 0) throw std::runtime_error("no rtaudio devices available!");

    const auto api_enum = rt.getCurrentApi();
    LOG("using rtaudio api %s", rt_audio_apis[static_cast<int>(api_enum)]);

    auto to_flt_vec = [](const std::vector<unsigned int> & vec) {
        std::vector<float> result;
        for (auto & i : vec) result.push_back(static_cast<float>(i));
        return result;
    };

    std::vector<AudioDeviceInfo> devices;
    for (uint32_t i = 0; i < rt.getDeviceCount(); ++i)
    {
        RtAudio::DeviceInfo info = rt.getDeviceInfo(i);

        if (info.probed)
        {
            AudioDeviceInfo lab_device_info;
            lab_device_info.index = i;
            lab_device_info.identifier = info.name;
            lab_device_info.num_output_channels = info.outputChannels;
            lab_device_info.num_input_channels = info.inputChannels;
            lab_device_info.supported_samplerates = to_flt_vec(info.sampleRates);
            lab_device_info.nominal_samplerate = info.preferredSampleRate;
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

uint32_t AudioDevice::GetDefaultOutputAudioDeviceIndex()
{
    RtAudio rt;
    if (rt.getDeviceCount() <= 0) throw std::runtime_error("no rtaudio devices available!");
    return rt.getDefaultOutputDevice();
}

uint32_t AudioDevice::GetDefaultInputAudioDeviceIndex()
{
    RtAudio rt;
    if (rt.getDeviceCount() <= 0) throw std::runtime_error("no rtaudio devices available!");
    return rt.getDefaultInputDevice();
}

AudioDevice * AudioDevice::MakePlatformSpecificDevice(AudioDeviceRenderCallback & callback, 
    const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig)
{
    return new AudioDevice_RtAudio(callback, outputConfig, inputConfig);
}

/////////////////////////////
//   AudioDevice_RtAudio   //
/////////////////////////////

const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;
const bool kInterleaved = false;

<<<<<<< HEAD:src/backends/windows/AudioDestinationWindows.cpp
AudioDestination * AudioDestination::MakePlatformAudioDestination(AudioIOCallback & callback,
                                                                  uint32_t numberOfInputChannels,
                                                                  uint32_t numberOfOutputChannels,
                                                                  float sampleRate)
{
    return new AudioDestinationWin(callback, numberOfOutputChannels, sampleRate);
}

AudioDestinationRtAudio::AudioDestinationRtAudio(AudioDeviceRenderCallback & callback, uint32_t numInputChannels, uint32_t numOutputChannels, float sampleRate)
    : _callback(callback), _numOutputChannels(numOutputChannels), _numInputChannels(numInputChannels), _sampleRate(sampleRate)
{
    return NumDefaultOutputChannels();
}

AudioDestinationWin::AudioDestinationWin(AudioIOCallback & callback, size_t numChannels, float sampleRate)
    : m_callback(callback)
    , m_renderBus(numChannels, AudioNode::ProcessingSizeInFrames, false)
{
    configure();
}

AudioDestinationWin::~AudioDestinationWin()
{
    //dac.release(); // XXX
    if (dac.isStreamOpen())
        dac.closeStream();
}

void AudioDestinationWin::configure()
=======
AudioDevice_RtAudio::AudioDevice_RtAudio(AudioDeviceRenderCallback & callback, 
    const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig) : _callback(callback)
>>>>>>> work on rtaudio backend implementation:src/backends/RtAudio/AudioDevice_RtAudio.cpp
{
    if (_dac.getDeviceCount() < 1)
    {
        LOG_ERROR("No audio devices available");
    }

    _dac.showWarnings(true);

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = dac.getDefaultOutputDevice();
    outputParams.nChannels = static_cast<unsigned int>(m_numChannels);
    outputParams.firstChannel = 0;
    auto outDeviceInfo = _dac.getDeviceInfo(outputParams.deviceId);

    LOG("Using Default Audio Device: %s", outDeviceInfo.name.c_str());

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

    // Note! RtAudio has a hard limit on a power of two buffer size, non-power of two sizes will result in
    // heap corruption, for example, when dac.stopStream() is invoked.
    uint32_t bufferFrames = 128;

    try
    {
        _dac.openStream(
            outDeviceInfo.probed && outDeviceInfo.isDefaultOutput ? &outputParams : nullptr,
            _numInputChannels > 0 ? &inputParams : nullptr,
            RTAUDIO_FLOAT32,
<<<<<<< HEAD:src/backends/windows/AudioDestinationWindows.cpp
            (unsigned int) m_sampleRate, &bufferFrames, &outputCallback, this, &options);
=======
            static_cast<unsigned int>(_sampleRate), &bufferFrames, &rt_audio_callback, this, &options);
>>>>>>> work on rtaudio backend implementation:src/backends/RtAudio/AudioDevice_RtAudio.cpp
    }
    catch (const RtAudioError & e)
    {
        LOG_ERROR(e.getMessage().c_str()); 
    }
}

<<<<<<< HEAD:src/backends/windows/AudioDestinationWindows.cpp
void AudioDestinationWin::start()
=======
AudioDevice_RtAudio::~AudioDevice_RtAudio()
>>>>>>> work on rtaudio backend implementation:src/backends/RtAudio/AudioDevice_RtAudio.cpp
{
    if (_dac.isStreamOpen()) _dac.closeStream();
    delete _renderBus;
    delete _inputBus;
}

<<<<<<< HEAD:src/backends/windows/AudioDestinationWindows.cpp
void AudioDestinationWin::stop()
=======
void AudioDevice_RtAudio::start()
>>>>>>> work on rtaudio backend implementation:src/backends/RtAudio/AudioDevice_RtAudio.cpp
{
    try { _dac.startStream(); }
    catch (const RtAudioError & e) { LOG_ERROR(e.getMessage().c_str()); }
}

void AudioDevice_RtAudio::stop()
{
    try { _dac.stopStream(); }
    catch (const RtAudioError & e) { LOG_ERROR(e.getMessage().c_str()); }
}

// Pulls on our provider to get rendered audio stream.
<<<<<<< HEAD:src/backends/windows/AudioDestinationWindows.cpp
void AudioDestinationWin::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
=======
void AudioDevice_RtAudio::render(int numberOfFrames, void * outputBuffer, void * inputBuffer)
>>>>>>> work on rtaudio backend implementation:src/backends/RtAudio/AudioDevice_RtAudio.cpp
{
    float * myOutputBufferOfFloats = (float *) outputBuffer;
    float * myInputBufferOfFloats = (float *) inputBuffer;

    if (_numOutputChannels)
    {
        for (uint32_t i = 0; i < m_numChannels; ++i)
        {
            m_renderBus.setChannelMemory(i, myOutputBufferOfFloats + i * numberOfFrames, numberOfFrames);
        }
    }

    if (_numInputChannels)
    {
        // Create
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
        {
            for (uint32_t i = 0; i < _numInputChannels; ++i)
            {
                AudioChannel * channel = _inputBus->channel(i);
                float * src = &myInputBufferOfFloats[i];
                VectorMath::vclip(src, 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), _numInputChannels, numberOfFrames);
            }
        }
        else
        {
            for (uint32_t i = 0; i < _numInputChannels; ++i)
            {
                AudioChannel * channel = _inputBus->channel(i);
                float * src = &myInputBufferOfFloats[i * numberOfFrames];
                VectorMath::vclip(src, 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
            }
        }
    }

    // render the output
    _callback.render(_inputBus, _renderBus, numberOfFrames);

    if (_numOutputChannels)
    {
        // copy the rendered audio to the destination
        if (kInterleaved)
        {
            for (uint32_t i = 0; i < _numOutputChannels; ++i)
            {
                AudioChannel * channel = _renderBus->channel(i);
                float * dst = &myOutputBufferOfFloats[i];
                // Clamp values at 0db (i.e., [-1.0, 1.0]) and also copy result to the DAC output buffer
                VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, dst, _numOutputChannels, numberOfFrames);
            }
        }
        else
        {
            for (uint32_t i = 0; i < _numOutputChannels; ++i)
            {
                AudioChannel * channel = _renderBus->channel(i);
                float * dst = &myOutputBufferOfFloats[i * numberOfFrames];
                // Clamp values at 0db (i.e., [-1.0, 1.0]) and also copy result to the DAC output buffer
                VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, dst, 1, numberOfFrames);
            }
        }
    }
}

int rt_audio_callback(void * outputBuffer, void * inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void * userData)
{
<<<<<<< HEAD:src/backends/windows/AudioDestinationWindows.cpp
=======
    AudioDevice_RtAudio * self = reinterpret_cast<AudioDevice_RtAudio *>(userData);
>>>>>>> work on rtaudio backend implementation:src/backends/RtAudio/AudioDevice_RtAudio.cpp
    float * fBufOut = (float *) outputBuffer;

    // Buffer is nBufferFrames * channels
    // NB: channel count should be set in a principled way
    memset(fBufOut, 0, sizeof(float) * nBufferFrames * self->outputChannelCount());

    AudioDestinationWin * audioDestination = static_cast<AudioDestinationWin *>(userData);
    audioDestination->render(nBufferFrames, fBufOut, inputBuffer);
    return 0;
}

}  // namespace lab
