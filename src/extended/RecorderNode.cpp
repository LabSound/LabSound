// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/LabSound.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/extended/RecorderNode.h"
#include "internal/Assertions.h"
#include "LabSound/extended/Registry.h"

#include "libnyquist/Encoders.h"

using namespace lab;


AudioNodeDescriptor * RecorderNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr};
    return &d;
}

RecorderNode::RecorderNode(AudioContext& ac)
    : AudioNode(ac, *desc())
{
    m_sampleRate = ac.sampleRate();
    m_channelInterpretation = ChannelInterpretation::Discrete;
    addInput("in");
    _channelCount = 1;
    initialize();
}

RecorderNode::RecorderNode(AudioContext & ac, const AudioStreamConfig & outConfig)
    : AudioNode(ac, *desc())
{
    m_sampleRate = outConfig.desired_samplerate;
    m_channelInterpretation = ChannelInterpretation::Discrete;
    addInput("in");
    _channelCount = 1;
    initialize();
}

RecorderNode::~RecorderNode()
{
    uninitialize();
}


void RecorderNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * dstBus = outputBus(r);
    AudioBus * srcBus = inputBus(r, 0);

    bool has_input = srcBus != nullptr && srcBus->numberOfChannels() > 0;
    if ((!isInitialized() || !has_input) && dstBus)
    {
        dstBus->zero();
    }

    if (!has_input)
    {
        // nothing to record.
        if (dstBus)
            dstBus->zero();
        return;
    }

    // the recorder will conform the number of output channels to the number of input
    // in order that it can function as a pass-through node.
    const int inputBusNumChannels = srcBus->numberOfChannels();
    int outputBusNumChannels = inputBusNumChannels;
    if (dstBus)
    {
        outputBusNumChannels = dstBus->numberOfChannels();

        if (inputBusNumChannels != outputBusNumChannels)
        {
            dstBus->setNumberOfChannels(r, inputBusNumChannels);
            outputBusNumChannels = inputBusNumChannels;
        }
    }

    if (m_recording)
    {
        const int numChannels = std::min(inputBusNumChannels, outputBusNumChannels);

        std::vector<const float*> channels;
        for (int i = 0; i < numChannels; ++i)
        {
            channels.push_back(srcBus->channel(i)->data());
        }

        if (m_data.size() < numChannels)
        {
            // allocate the recording buffers lazily when the number of input channels is finally known
            for (int i = 0; i < numChannels; ++i)
            {
                m_data.emplace_back(std::vector<float>());
                m_data[i].reserve(1024 * 1024);
            }
        }

        // copy the output. @TODO this should be a memcpy
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        for (int c = 0; c < numChannels; ++c)
        {
            for (int i = 0; i < bufferSize; ++i)
                m_data[c].push_back(channels[c][i]);
        }
    }

    // pass through 
    if (dstBus)
        dstBus->copyFrom(*srcBus);
}


float RecorderNode::recordedLengthInSeconds() const
{
    size_t recordedChannelCount = m_data.size();
    if (!recordedChannelCount) 
        return 0;

    size_t numSamples = m_data[0].size();
    return numSamples / m_sampleRate;
}


bool RecorderNode::writeRecordingToWav(const std::string & filenameWithWavExtension, bool mixToMono)
{
    size_t recordedChannelCount = m_data.size();
    if (!recordedChannelCount) return false;
    size_t numSamples = m_data[0].size();
    if (!numSamples) return false;

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
                dst[i] += clear_data[j][i];
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
                *dst++ = clear_data[j][i];
        }
    }

    fileData->sampleRate = static_cast<int>(m_sampleRate);
    fileData->sourceFormat = nqr::PCM_FLT;

    nqr::EncoderParams params = {static_cast<int>(recordedChannelCount), nqr::PCM_FLT, nqr::DITHER_NONE};
    bool result = nqr::EncoderError::NoError == nqr::encode_wav_to_disk(params, fileData.get(), filenameWithWavExtension);
    return result;
}

std::unique_ptr<AudioBus> RecorderNode::createBusFromRecording(bool mixToMono)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    int numSamples = static_cast<int>(m_data[0].size());
    if (!numSamples) return {};

    size_t recordedChannelCount = m_data.size();
    const int result_channel_count = mixToMono ? 1 : (int) recordedChannelCount;

    // Create AudioBus where we'll put the PCM audio data
    std::unique_ptr<lab::AudioBus> result_audioBus(new lab::AudioBus(result_channel_count, numSamples));
    result_audioBus->setSampleRate(m_sampleRate);

    // Mix channels to mono if requested, and there's more than one input channel.
    if (recordedChannelCount > 1 && mixToMono)
    {
        float* destinationMono = result_audioBus->channel(0)->mutableData();

        for (int i = 0; i < numSamples; i++)
        {
            destinationMono[i] = 0;
            for (size_t j = 0; j < result_channel_count; ++j)
                destinationMono[i] += m_data[j][i];
            destinationMono[i] *= 1.f / static_cast<float>(result_channel_count);
        }
    }
    else
    {
        for (int i = 0; i < result_channel_count; ++i)
        {
            memcpy(result_audioBus->channel(i)->mutableData(), m_data[0].data(), numSamples * sizeof(float));
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
