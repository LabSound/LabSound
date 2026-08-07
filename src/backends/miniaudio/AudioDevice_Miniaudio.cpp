// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#define LABSOUND_ENABLE_LOGGING

#include "LabSound/backends/AudioDevice_Miniaudio.h"

#include "internal/Assertions.h"

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNode.h"

#include "LabSound/extended/Logging.h"
#include "LabSound/extended/VectorMath.h"

#include <assert.h>

//#define MA_DEBUG_OUTPUT
#define MINIAUDIO_IMPLEMENTATION

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
//#define MA_DEBUG_OUTPUT
#include "miniaudio.h"
#include <atomic>
#include <cstring>

#include "miniaudio.h"

#include <mutex>
#include <set>

namespace lab
{

////////////////////////////////////////////////////
//   Platform/backend specific static functions   //
////////////////////////////////////////////////////

const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;
const int kRenderQuantum = AudioNode::ProcessingSizeInFrames;

/// @TODO - the AudioDeviceInfo wants to support specific sample rates, but miniaudio only tells min and max
///         miniaudio also has a concept of minChannels, which LabSound ignores


namespace
{
    // Reference-count the lifetime of the shared ma_context. g_context is a process-wide singleton (it
    // hosts device enumeration and is passed to every ma_device_init), but the original
    // ~AudioDevice_Miniaudio unconditionally called ma_context_uninit(&g_context). A page with two
    // AudioContexts (e.g. SFX + music, which browsers allow) has two devices sharing it: the first device
    // destroyed tears the shared context down, and the second device's destructor tears it down again.
    // ma_context_uninit() does not zero the struct, so the second pass disconnects an already-freed backend
    // context and PulseAudio aborts (`pa_atomic_load(&(c)->_ref) >= 1` failed at context.c); meanwhile any
    // device still alive after the first teardown keeps running against a dangling context. Instead: init
    // on demand when a reference is taken, and uninit only when the last reference is returned.
    // g_context_mutex guards the ref count / init state (devices may be created and destroyed on different
    // threads). g_devices is a pure value cache (strings/floats), independent of the context lifetime, so
    // it is kept across inits.
    ma_context g_context;
    static std::vector<AudioDeviceInfo> g_devices;
    static bool g_must_init = true;
    static int g_context_refs = 0;
    static bool g_probed = false;
    static std::mutex g_context_mutex;

    // Caller must hold g_context_mutex.
    bool init_context_locked()
    {
        if (!g_must_init)
            return true;

        LOG_TRACE("[LabSound] init_context() must_init");
        if (ma_context_init(NULL, 0, NULL, &g_context) != MA_SUCCESS)
        {
            LOG_ERROR("[LabSound] init_context(): Failed to initialize miniaudio context");
            return false;
        }
        LOG_TRACE("[LabSound] init_context() succeeded");
        g_must_init = false;
        return true;
    }

    // Caller must hold g_context_mutex. Uninit only when the last reference is returned.
    void release_context_locked()
    {
        if (g_context_refs <= 0)
            return;
        if (--g_context_refs == 0 && !g_must_init)
        {
            ma_context_uninit(&g_context);
            g_must_init = true;
        }
    }

    // Take a shared-context reference (initializing if needed). Each success must be paired with one
    // release_context().
    bool acquire_context()
    {
        std::lock_guard<std::mutex> lock(g_context_mutex);
        if (!init_context_locked())
            return false;
        ++g_context_refs;
        return true;
    }

    void release_context()
    {
        std::lock_guard<std::mutex> lock(g_context_mutex);
        release_context_locked();
    }
}

void PrintAudioDeviceList()
{
    if (!g_devices.size())
        LOG_INFO("No devices detected");
    else
        for (auto & device : g_devices)
        {
            LOG_INFO("[%d] %s\n----------------------\n", device.index, device.identifier.c_str());
            LOG_INFO("   ins:%d outs:%d default_in:%s default:out:%s\n", device.num_input_channels, device.num_output_channels, device.is_default_input?"yes":"no", device.is_default_output?"yes":"no");
            LOG_INFO("    nominal samplerate: %f\n", device.nominal_samplerate);
            for (float f : device.supported_samplerates)
                LOG_INFO("        %f\n", f);
        }
}

std::vector<AudioDeviceInfo> AudioDevice_Miniaudio::MakeAudioDeviceList()
{
    // Hold a context reference for the duration of enumeration (otherwise, concurrently, another thread's
    // device destructor could tear the context down mid-probe).
    std::lock_guard<std::mutex> lock(g_context_mutex);
    if (g_probed)
        return g_devices;

    if (!init_context_locked())
        return {};

    ++g_context_refs;
    struct ScopedRelease
    {
        ~ScopedRelease() { release_context_locked(); }
    } scopedRelease;

    ma_result result;
    ma_device_info * pPlaybackDeviceInfos;
    ma_uint32 playbackDeviceCount;
    ma_device_info * pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;
    ma_uint32 iDevice;

    result = ma_context_get_devices(&g_context,
                                    &pPlaybackDeviceInfos,
                                    &playbackDeviceCount,
                                    &pCaptureDeviceInfos,
                                    &captureDeviceCount);

    if (result != MA_SUCCESS)
    {
        LOG_ERROR("Failed to retrieve audio device information.\n");
        return {};
    }

    std::set<float> playbackRates;

    for (iDevice = 0; iDevice < playbackDeviceCount; ++iDevice)
    {
        AudioDeviceInfo lab_device_info;
        lab_device_info.index = (int32_t) g_devices.size();
        lab_device_info.identifier = pPlaybackDeviceInfos[iDevice].name;

        if (ma_context_get_device_info(
                    &g_context, 
                    ma_device_type_playback,
                    &pPlaybackDeviceInfos[iDevice].id, 
                    &pPlaybackDeviceInfos[iDevice])
            != MA_SUCCESS)
            continue;

        lab_device_info.num_input_channels = 0;
        lab_device_info.num_output_channels = 0;
        for (uint32_t i = 0; i < pPlaybackDeviceInfos[iDevice].nativeDataFormatCount; ++i)
        {
            playbackRates.insert(static_cast<float>(pPlaybackDeviceInfos[iDevice].nativeDataFormats[i].sampleRate));
            if (pPlaybackDeviceInfos[iDevice].nativeDataFormats[i].channels > lab_device_info.num_output_channels)
                lab_device_info.num_output_channels = pPlaybackDeviceInfos[iDevice].nativeDataFormats[i].channels;
        }
        for (auto r : playbackRates)
        {
            lab_device_info.supported_samplerates.push_back(r);
        }

        lab_device_info.nominal_samplerate = playbackRates.size() > 0 ? lab_device_info.supported_samplerates.back() : 0;

        lab_device_info.is_default_output = pPlaybackDeviceInfos[iDevice].isDefault;
        lab_device_info.is_default_input = false;

        g_devices.push_back(lab_device_info);
    }

    for (iDevice = 0; iDevice < captureDeviceCount; ++iDevice)
    {
        AudioDeviceInfo lab_device_info;
        lab_device_info.index = (int32_t) g_devices.size();
        lab_device_info.identifier = pCaptureDeviceInfos[iDevice].name;

        if (ma_context_get_device_info(
                    &g_context, 
                    ma_device_type_capture,
                    &pCaptureDeviceInfos[iDevice].id,
                    &pCaptureDeviceInfos[iDevice])
            != MA_SUCCESS)
            continue;

        lab_device_info.num_input_channels = 0;
        lab_device_info.num_output_channels = 0;
        for (uint32_t i = 0; i < pCaptureDeviceInfos[iDevice].nativeDataFormatCount; ++i)
        {
            playbackRates.insert(static_cast<float>(pPlaybackDeviceInfos[iDevice].nativeDataFormats[i].sampleRate));
            if (pCaptureDeviceInfos[iDevice].nativeDataFormats[i].channels > lab_device_info.num_input_channels)
                lab_device_info.num_input_channels = pCaptureDeviceInfos[iDevice].nativeDataFormats[i].channels;
        }
        for (auto r : playbackRates)
        {
            lab_device_info.supported_samplerates.push_back(r);
        }

        lab_device_info.nominal_samplerate = playbackRates.size() > 0 ? lab_device_info.supported_samplerates.back() : 0;
        lab_device_info.is_default_output = false;
        lab_device_info.is_default_input = pCaptureDeviceInfos[iDevice].isDefault;

        g_devices.push_back(lab_device_info);
    }

    g_probed = true; // only cache on a successful probe (a failed probe retries next time; the original
                     // code permanently returned an empty list after one failure)
    return g_devices;
}

namespace
{
    void outputCallback(ma_device * pDevice, void * pOutput, const void * pInput, ma_uint32 frameCount)
    {
        // Buffer is nBufferFrames * channels, interleaved
        float * fBufOut = (float *) pOutput;
        AudioDevice_Miniaudio * ad = reinterpret_cast<AudioDevice_Miniaudio *>(pDevice->pUserData);
        memset(fBufOut, 0, sizeof(float) * frameCount * ad->getOutputConfig().desired_channels);
        ad->render(frameCount, pOutput, const_cast<void *>(pInput));
    }
}

AudioDevice_Miniaudio::AudioDevice_Miniaudio(const AudioStreamConfig & _inputConfig,
                                             const AudioStreamConfig & _outputConfig)
: AudioDevice(_inputConfig, _outputConfig)
{
    _device = new ma_device();
    auto device_list = MakeAudioDeviceList();
    PrintAudioDeviceList();

    //ma_device_config deviceConfig = ma_device_config_init(ma_device_type_duplex);
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = _outConfig.desired_channels;
    deviceConfig.sampleRate = static_cast<int>(_outConfig.desired_samplerate);
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = _inConfig.desired_channels;
    deviceConfig.dataCallback = outputCallback;
    deviceConfig.performanceProfile = ma_performance_profile_low_latency;
    deviceConfig.pUserData = this;

#ifdef __WINDOWS_WASAPI__
    deviceConfig.wasapi.noAutoConvertSRC = true;
#endif

    // Hold a shared-context reference for the device's lifetime (returned in the destructor; the last
    // device out calls uninit).
    if (!acquire_context())
    {
        LOG_ERROR("Unable to initialize miniaudio context");
        return;
    }
    _holdsContextRef = true;

    if (ma_device_init(&g_context, &deviceConfig, _device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to open audio playback device");
        release_context(); // failed to open the device: return the reference immediately rather than
                           // leaving the shared context tied to a half-dead device
        _holdsContextRef = false;
        return;
    }
    _deviceInitialized = true;

    authoritativeDeviceSampleRateAtRuntime = _outConfig.desired_samplerate;

    samplingInfo.epoch[0] = samplingInfo.epoch[1] = std::chrono::high_resolution_clock::now();

    _ring = new lab::RingBufferT<float>();
    _ring->resize(static_cast<int>(authoritativeDeviceSampleRateAtRuntime));  // ad hoc. hold one second
    _scratch = reinterpret_cast<float *>(malloc(sizeof(float) * kRenderQuantum * _inConfig.desired_channels));
}

AudioDevice_Miniaudio::~AudioDevice_Miniaudio()
{
    // (1) Only tear down a ma_device that actually initialized (on a failed open, _device is a zeroed
    // struct and stop()/uninit() on it just logs "Unable to stop audio device" noise). (2) Return the
    // shared-context reference instead of unconditionally uninit-ing it: with multiple coexisting devices
    // the original code let the first device destroyed tear down a context others were still using, and the
    // second destructor tore it down again -> PulseAudio abort (see the note above).
    if (_deviceInitialized)
    {
        stop();
        ma_device_uninit(_device);
        _deviceInitialized = false;
    }
    if (_holdsContextRef)
    {
        release_context();
        _holdsContextRef = false;
    }
    delete _renderBus;
    delete _inputBus;
    delete _ring;
    if (_scratch)
        free(_scratch);
    delete _device;
}

void AudioDevice_Miniaudio::backendReinitialize()
{
    auto device_list = MakeAudioDeviceList();
    PrintAudioDeviceList();

    //ma_device_config deviceConfig = ma_device_config_init(ma_device_type_duplex);
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = _outConfig.desired_channels;
    deviceConfig.sampleRate = static_cast<int>(_outConfig.desired_samplerate);
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = _inConfig.desired_channels;
    deviceConfig.dataCallback = outputCallback;
    deviceConfig.performanceProfile = ma_performance_profile_low_latency;
    deviceConfig.pUserData = this;

#ifdef __WINDOWS_WASAPI__
    deviceConfig.wasapi.noAutoConvertSRC = true;
#endif

    // As in the constructor, reinitialization must hold a shared-context reference (not yet held if the
    // constructor's device open failed).
    if (!_holdsContextRef)
    {
        if (!acquire_context())
        {
            LOG_ERROR("Unable to initialize miniaudio context");
            return;
        }
        _holdsContextRef = true;
    }

    if (ma_device_init(&g_context, &deviceConfig, _device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to open audio playback device");
        return;
    }
    _deviceInitialized = true;
}


void AudioDevice_Miniaudio::start()
{
    ASSERT(authoritativeDeviceSampleRateAtRuntime != 0.f);  // something went very wrong
    if (ma_device_start(_device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to start audio device");
    }
}

void AudioDevice_Miniaudio::stop()
{
    if (ma_device_stop(_device) != MA_SUCCESS)
    {
        LOG_ERROR("Unable to stop audio device");
    }
}

bool AudioDevice_Miniaudio::isRunning() const
{
    return ma_device_is_started(_device);
}


// Pulls on our provider to get rendered audio stream.
int AudioDevice_Miniaudio::render(int numberOfFrames_, void * outputBuffer, void * inputBuffer)
{
    int numberOfFrames = numberOfFrames_;
    if (!_renderBus)
    {
        _renderBus = new AudioBus(_outConfig.desired_channels, kRenderQuantum, true);
        _renderBus->setSampleRate(authoritativeDeviceSampleRateAtRuntime);
    }

    if (!_inputBus && _inConfig.desired_channels)
    {
        _inputBus = new AudioBus(_inConfig.desired_channels, kRenderQuantum, true);
        _inputBus->setSampleRate(authoritativeDeviceSampleRateAtRuntime);
    }

    float * pIn = static_cast<float *>(inputBuffer);
    float * pOut = static_cast<float *>(outputBuffer);

    int in_channels = _inConfig.desired_channels;
    int out_channels = _outConfig.desired_channels;

    if (pIn && numberOfFrames * in_channels)
        _ring->write(pIn, numberOfFrames * in_channels);

    while (numberOfFrames > 0)
    {
        if (_remainder > 0)
        {
            // copy samples to output buffer. There might have been some rendered frames
            // left over from the previous numberOfFrames, so start by moving those into
            // the output buffer.

            // miniaudio expects interleaved data, this loop leverages the vclip operation
            // to copy, interleave, and clip in one pass.

            int samples = _remainder < numberOfFrames ? _remainder : numberOfFrames;
            for (int i = 0; i < out_channels; ++i)
            {
                int src_stride = 1;  // de-interleaved
                int dst_stride = out_channels;  // interleaved
                AudioChannel * channel = _renderBus->channel(i);
                VectorMath::vclip(channel->data() + kRenderQuantum - _remainder, src_stride,
                                  &kLowThreshold, &kHighThreshold,
                                  pOut + i, dst_stride, samples);
            }
            pOut += out_channels * samples;

            numberOfFrames -= samples;  // deduct samples actually copied to output
            _remainder -= samples;  // deduct samples remaining from last render() invocation
        }
        else
        {
            if (in_channels)
            {
                // miniaudio provides the input data in interleaved form, vclip is used here to de-interleave

                _ring->read(_scratch, in_channels * kRenderQuantum);
                for (int i = 0; i < in_channels; ++i)
                {
                    int src_stride = in_channels;  // interleaved
                    int dst_stride = 1;  // de-interleaved
                    AudioChannel * channel = _inputBus->channel(i);
                    VectorMath::vclip(_scratch + i, src_stride,
                                      &kLowThreshold, &kHighThreshold,
                                      channel->mutableData(), dst_stride, kRenderQuantum);
                }
            }

            // Update sampling info for use by the render graph
            const int32_t index = 1 - (samplingInfo.current_sample_frame & 1);
            const uint64_t t = samplingInfo.current_sample_frame & ~1;
            samplingInfo.sampling_rate = authoritativeDeviceSampleRateAtRuntime;
            samplingInfo.current_sample_frame = t + kRenderQuantum + index;
            samplingInfo.current_time = samplingInfo.current_sample_frame / static_cast<double>(samplingInfo.sampling_rate);
            samplingInfo.epoch[index] = std::chrono::high_resolution_clock::now();

            // generate new data
            _destinationNode->render(sourceProvider(), _inputBus, _renderBus, kRenderQuantum, samplingInfo);
            _remainder = kRenderQuantum;
        }
    }
    return numberOfFrames_;
}

}  // namespace lab
