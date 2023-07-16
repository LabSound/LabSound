// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/LabSound.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/extended/RecorderNode.h"
#include "internal/Assertions.h"
#include "LabSound/extended/Registry.h"

#include "libnyquist/Encoders.h"

using namespace lab;

struct RecorderNode::RecorderSettings {
    RecorderSettings() = default;
    ~RecorderSettings() = default;

    bool recording = false;
    float recordingSampleRate = 44100.f;
    int numChannelsToRecord = 1;

    std::vector<std::vector<float>> data;  // non-interleaved
    mutable std::recursive_mutex mutex;
};

AudioNodeDescriptor * RecorderNode::desc()
{
    static AudioNodeDescriptor d { nullptr, nullptr, 1 };
    return &d;
}

RecorderNode::RecorderNode(AudioContext& r, int channelCount)
: AudioNode(r, *desc())
, _settings(new RecorderSettings())
{
    _settings->recordingSampleRate = r.sampleRate();
    _settings->numChannelsToRecord = channelCount;
    _self->m_channelInterpretation = ChannelInterpretation::Discrete;
    initialize();
}

RecorderNode::RecorderNode(AudioContext & ac, const AudioStreamConfig& recordingConfig)
: AudioNode(ac, *desc())
, _settings(new RecorderSettings())
{
    _settings->recordingSampleRate = recordingConfig.desired_samplerate;
    _settings->numChannelsToRecord = recordingConfig.desired_channels;
    _self->m_channelInterpretation = ChannelInterpretation::Discrete;
    initialize();
}

RecorderNode::~RecorderNode()
{
    uninitialize();
}

void RecorderNode::startRecording() { _settings->recording = true; }
void RecorderNode::stopRecording()  { _settings->recording = false; }

void RecorderNode::process(ContextRenderLock & r, int bufferSize)
{
    auto outputBus = _self->output;
    auto inputBus = summedInput();
    const int inputBusNumChannels = inputBus->numberOfChannels();
    bool has_input = inputBus && input(0)->isConnected() && inputBusNumChannels > 0;
    if (!has_input)
    {
        // nothing to record.
        if (outputBus)
            outputBus->zero();
        return;
    }

    if (_settings->recording) {
        std::lock_guard<std::recursive_mutex> lock(_settings->mutex);
        
        if (_settings->data.size() < inputBusNumChannels) {
            for (int i = 0; i < inputBusNumChannels; ++i) {
                _settings->data.emplace_back(std::vector<float>());
                _settings->data[i].reserve(1024 * 1024);
            }
        }

        for (int c = 0; c < inputBusNumChannels; ++c) {
            // relying on the doubling behavior of push_back to keep increasing
            // the size of thebuffer.
            const float* channelData = inputBus->channel(c)->data();
            for (int i = 0; i < bufferSize; ++i)
                _settings->data[c].push_back(channelData[i]);
        }
    }

    // if the recorder node is being used as a pass through, conform the
    // number of outputs to match the number of inputs.
    if (outputBus) {
        if (inputBusNumChannels != outputBus->numberOfChannels()) {
            output()->setNumberOfChannels(r, inputBusNumChannels);
            outputBus = output();
        }
        outputBus->copyFrom(*inputBus);
    }
}


float RecorderNode::recordedLengthInSeconds() const
{
    return _settings->data[0].size() / _settings->recordingSampleRate;
}


bool RecorderNode::writeRecordingToWav(const std::string & filenameWithWavExtension, bool mixToMono)
{
    std::lock_guard<std::recursive_mutex> lock(_settings->mutex);
    int recordedChannelCount = (int) _settings->data.size();
    if (!recordedChannelCount) return false;
    int numSamples = (int) _settings->data[0].size();
    if (!numSamples) return false;
    const int writingChannelCount = mixToMono ? 1 : recordedChannelCount;

    std::unique_ptr<nqr::AudioData> fileData(new nqr::AudioData());

    if (recordedChannelCount == 1)
    {
        // only one channel recorded
        fileData->samples.resize(numSamples);
        fileData->channelCount = 1;
        float* dst = fileData->samples.data();
        memcpy(dst, _settings->data[0].data(), sizeof(float) * numSamples);
    }
    else if (recordedChannelCount > 1 && mixToMono)
    {
        // Mix channels to mono if requested, and there's more than one input channel.
        fileData->samples.resize(numSamples);
        fileData->channelCount = 1;
        float* dst = fileData->samples.data();
        for (size_t i = 0; i < numSamples; i++)
        {
            dst[i] = 0;
            for (size_t j = 0; j < recordedChannelCount; ++j)
                dst[i] += _settings->data[j][i];
            dst[i] *= 1.f / static_cast<float>(recordedChannelCount);
        }
    }
    else
    {
        // straight through
        fileData->samples.resize(numSamples * recordedChannelCount);
        fileData->channelCount = static_cast<int>(recordedChannelCount);
        float* dst = fileData->samples.data();
        for (size_t i = 0; i < numSamples; i++)
        {
            for (size_t j = 0; j < recordedChannelCount; ++j)
                *dst++ = _settings->data[j][i];
        }
    }

    fileData->sampleRate = static_cast<int>(_settings->recordingSampleRate);
    fileData->sourceFormat = nqr::PCM_FLT;

    nqr::EncoderParams params = {static_cast<int>(recordedChannelCount), nqr::PCM_FLT, nqr::DITHER_NONE};
    bool result = nqr::EncoderError::NoError == nqr::encode_wav_to_disk(params, fileData.get(), filenameWithWavExtension);

    for (int i = 0; i < _settings->data.size(); ++i)
        _settings->data[i].clear();

    return result;
}

std::unique_ptr<AudioBus> RecorderNode::createBusFromRecording(bool mixToMono)
{
    std::lock_guard<std::recursive_mutex> lock(_settings->mutex);
    int recordedChannelCount = (int) _settings->data.size();
    if (!recordedChannelCount) return {};
    int numSamples = (int) _settings->data[0].size();
    if (!numSamples) return {};
    const int writingChannelCount = mixToMono ? 1 : recordedChannelCount;

    // Create AudioBus where we'll put the PCM audio data
    std::unique_ptr<lab::AudioBus> result_audioBus(new lab::AudioBus(writingChannelCount, numSamples));
    result_audioBus->setSampleRate(_settings->recordingSampleRate);

    // Mix channels to mono if requested, and there's more than one input channel.
    if (mixToMono) {
        float* destinationMono = result_audioBus->channel(0)->mutableData();

        for (int i = 0; i < numSamples; i++)
        {
            destinationMono[i] = 0;
            for (size_t j = 0; j < recordedChannelCount; ++j)
                destinationMono[i] += _settings->data[j][i];
            destinationMono[i] *= 1.f / static_cast<float>(recordedChannelCount);
        }
    }
    else
    {
        for (int i = 0; i < writingChannelCount; ++i)
        {
            memcpy(result_audioBus->channel(i)->mutableData(), _settings->data[0].data(), numSamples * sizeof(float));
        }
    }

    for (int i = 0; i < _settings->data.size(); ++i)
        _settings->data[i].clear();

    return result_audioBus;
}


void RecorderNode::reset(ContextRenderLock& r)
{
    AudioNode::reset(r);
    std::lock_guard<std::recursive_mutex> lock(_settings->mutex);
    for (int i = 0; i < _settings->data.size(); ++i)
        _settings->data[i].clear();
}
