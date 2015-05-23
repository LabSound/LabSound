/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSound/core/AudioBuffer.h"

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "internal/AudioBus.h"
#include "internal/AudioFileReader.h"

#include <memory>

namespace WebCore 
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
            // @todo this should just be a memset
            for (size_t j = 0; j < l; ++j)
                vec[j] = 0.f;
        }
    }
}

} // namespace WebCore
