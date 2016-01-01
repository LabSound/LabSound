// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBuffer.h"

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "internal/AudioBus.h"
#include "internal/AudioFileReader.h"

#include <memory>

namespace lab 
{

AudioBuffer::AudioBuffer(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate)
    : m_gain(1.0)
    , m_sampleRate(sampleRate)
    , m_length(numberOfFrames)
{

    if (sampleRate < 22050 || sampleRate > 96000 || numberOfChannels > AudioContext::maxNumberOfChannels || !numberOfFrames)
        throw std::invalid_argument("Invalid parameters");

    m_channels.reserve(numberOfChannels);

    for (unsigned i = 0; i < numberOfChannels; ++i) {
        std::shared_ptr<std::vector<float>> channelDataArray(new std::vector<float>());
        channelDataArray->resize(m_length);
        m_channels.push_back(channelDataArray);
    }
}

AudioBuffer::AudioBuffer(AudioBus* bus)
    : m_gain(1.0)
    , m_sampleRate(bus->sampleRate())
    , m_length(bus->length())
{
    // Copy audio data from the bus to the vector<float>s we manage.
    unsigned numberOfChannels = bus->numberOfChannels();
    
    m_channels.reserve(numberOfChannels);
    for (unsigned i = 0; i < numberOfChannels; ++i)
    {
        std::shared_ptr<std::vector<float>> channelDataArray(new std::vector<float>());

        channelDataArray->resize(m_length);

        const float *busData = bus->channel(i)->data();

        std::vector<float>& vec = *(channelDataArray.get());

        for (size_t j = 0; j < m_length; ++j)
        {
            vec[j] = busData[j];
        }
        m_channels.push_back(channelDataArray);
    }
}

AudioBuffer::~AudioBuffer()
{
    releaseMemory();
}

void AudioBuffer::releaseMemory()
{
    m_channels.clear();
}

std::shared_ptr<std::vector<float>> AudioBuffer::getChannelData(unsigned channelIndex)
{
    if (channelIndex >= m_channels.size())
        return nullptr;

    return m_channels[channelIndex];
}

void AudioBuffer::zero()
{
    for (unsigned i = 0; i < m_channels.size(); ++i) {
        if (getChannelData(i)) {
            std::vector<float>& vec = *(getChannelData(i).get());
            size_t l = length();
            for (size_t j = 0; j < l; ++j)
                vec[j] = 0.f; // @tofix - this should just be a memset
        }
    }
}

} // namespace lab
