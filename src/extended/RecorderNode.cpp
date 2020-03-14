// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/LabSound.h"

#include "LabSound/extended/RecorderNode.h"
#include "internal/Assertions.h"

#include "libnyquist/Encoders.h"

using namespace lab;

RecorderNode::RecorderNode(AudioContext& r, int channelCount)
    : AudioBasicInspectorNode(r, channelCount)
{
    m_sampleRate = r.sampleRate();

    m_channelCount = channelCount;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Discrete;
    initialize();
}

RecorderNode::RecorderNode(AudioContext & ac, const AudioStreamConfig outConfig)
    : AudioBasicInspectorNode(ac, outConfig.desired_channels)
{
    m_sampleRate = outConfig.desired_samplerate;
    m_channelCount = outConfig.desired_channels;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Discrete;
    initialize();
}

RecorderNode::~RecorderNode()
{
    uninitialize();
}

void RecorderNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * outputBus = output(0)->bus(r);

    if (!isInitialized() || !input(0)->isConnected())
    {
        if (outputBus)
        {
            outputBus->zero();
        }
        return;
    }

    AudioBus * bus = input(0)->bus(r);
    bool isBusGood = bus && (bus->numberOfChannels() > 0) && (bus->channel(0)->length() >= bufferSize);

    ASSERT(isBusGood);
    if (!isBusGood)
    {
        outputBus->zero();
        return;
    }

    if (m_recording)
    {
        std::vector<const float *> channels;
        const int inputBusNumChannels = bus->numberOfChannels();

        for (int i = 0; i < inputBusNumChannels; ++i)
        {
            channels.push_back(bus->channel(i)->data());
        }

        if (!m_data.size())
        {
            // allocate the recording buffers lazily when the number of input channels is finally known
            for (int i = 0; i < inputBusNumChannels; ++i)
                m_data.emplace_back(std::vector<float>());
            for (int i = 0; i < inputBusNumChannels; ++i)
                m_data[i].reserve(1024 * 1024);
        }

        // mix down the output, or interleave the output
        // use the tightest loop possible since this is part of the processing step
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        for (int c = 0; c < inputBusNumChannels; ++c)
        {
            for (int i = 0; i < bufferSize; ++i)
                m_data[c].push_back(channels[c][i]);
        }
    }

    // the behavior of copyFrom is that
    // for in-place processing, our override of pullInputs() will just pass the audio data
    // through unchanged if the channel count matches from input to output
    // (resulting in inputBus == outputBus). Otherwise, do an up-mix to stereo.
    if (bus != outputBus)
    {
        outputBus->copyFrom(*bus);
    }
}

float RecorderNode::recordedLengthInSeconds() const
{
    size_t recordedChannelCount = m_data.size();
    if (!recordedChannelCount) return 0;
    size_t numSamples = m_data[0].size();
    return numSamples / m_sampleRate;
}


void RecorderNode::writeRecordingToWav(const std::string & filenameWithWavExtension, bool mixToMono)
{
    size_t recordedChannelCount = m_data.size();
    if (!recordedChannelCount) return;
    size_t numSamples = m_data[0].size();
    if (!numSamples) return;

    std::unique_ptr<nqr::AudioData> fileData(new nqr::AudioData());

    std::vector<std::vector<float>> clear_data;
    for (int i = 0; i < recordedChannelCount; ++i)
        clear_data.emplace_back(std::vector<float>());

    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_data.swap(clear_data);
    }

    if (recordedChannelCount == 1)
    {
        // only one channel recorded
        fileData->samples.resize(numSamples);
        fileData->channelCount = 1;
        float* dst = fileData->samples.data();
        memcpy(dst, clear_data[0].data(), sizeof(float) * numSamples);
        for (size_t i = 0; i < numSamples; i++)
        {
            for (size_t j = 0; j < recordedChannelCount; ++j)
                *dst++ = clear_data[j][i];
        }
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
            for (size_t j = 0; j < m_channelCount; ++j)
                dst[i] += clear_data[j][i];
            dst[i] *= 1.f / static_cast<float>(m_channelCount);
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
                *dst++ = clear_data[j][i];
        }
    }

    fileData->sampleRate = static_cast<int>(m_sampleRate);
    fileData->sourceFormat = nqr::PCM_FLT;

    nqr::EncoderParams params = {static_cast<int>(recordedChannelCount), nqr::PCM_FLT, nqr::DITHER_NONE};
    const int encoder_status = nqr::encode_wav_to_disk(params, fileData.get(), filenameWithWavExtension);
}

std::shared_ptr<AudioBus> RecorderNode::createBusFromRecording(bool mixToMono)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    int numSamples = static_cast<int>(m_data[0].size());
    if (!numSamples) return {};

    const int result_channel_count = mixToMono ? 1 : m_channelCount;

    // Create AudioBus where we'll put the PCM audio data
    std::shared_ptr<lab::AudioBus> result_audioBus(new lab::AudioBus(result_channel_count, numSamples));
    result_audioBus->setSampleRate(m_sampleRate);

    // Mix channels to mono if requested, and there's more than one input channel.
    if (m_channelCount > 1 && mixToMono)
    {
        float* destinationMono = result_audioBus->channel(0)->mutableData();

        for (int i = 0; i < numSamples; i++)
        {
            destinationMono[i] = 0;
            for (size_t j = 0; j < m_channelCount; ++j)
                destinationMono[i] += m_data[j][i];
            destinationMono[i] *= 1.f / static_cast<float>(m_channelCount);
        }
    }
    else
    {
        for (int i = 0; i < result_channel_count; ++i)
        {
            std::memcpy(result_audioBus->channel(i)->mutableData(), m_data[0].data(), numSamples * sizeof(float));
        }
    }

    for (int i = 0; i < m_data.size(); ++i)
        m_data[i].clear();

    return result_audioBus;
}


void RecorderNode::reset(ContextRenderLock & r)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (int i = 0; i < m_data.size(); ++i)
        m_data[i].clear();
}
