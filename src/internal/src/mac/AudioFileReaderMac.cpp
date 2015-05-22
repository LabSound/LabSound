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

#include "internal/ConfigMacros.h"
#include "internal/mac/AudioFileReaderMac.h"
#include "internal/AudioBus.h"
#include "internal/AudioFileReader.h"
#include "internal/FloatConversion.h"
#include <CoreFoundation/CoreFoundation.h>

namespace WebCore {

AudioFileReader::AudioFileReader(const char * filePath)
{
    
}

AudioFileReader::AudioFileReader(const void* data, size_t dataSize)
{

}

AudioFileReader::~AudioFileReader()
{

}

std::unique_ptr<AudioBus> AudioFileReader::createBus(float sampleRate, bool mixToMono)
{

    // Number of channels
    size_t numberOfChannels = m_fileDataFormat.mChannelsPerFrame;
    
    // Sample-rate
    double fileSampleRate = m_fileDataFormat.mSampleRate;

    // Samples per channel
    size_t numberOfFrames = static_cast<size_t>(numberOfFrames);

    size_t busChannelCount = mixToMono ? 1 : numberOfChannels;

    // Create AudioBus where we'll put the PCM audio data
    std::unique_ptr<AudioBus> audioBus(new AudioBus(busChannelCount, numberOfFrames));
    audioBus->setSampleRate(narrowPrecisionToFloat(fileSampleRate)); // save for later

    // Only allocated in the mixToMono case
    AudioFloatArray bufL;
    AudioFloatArray bufR;
    float * bufferL = 0;
    float * bufferR = 0;
    
    // Setup AudioBufferList in preparation for reading
    AudioBufferList * bufferList = createAudioBufferList(numberOfChannels);

    if (mixToMono && numberOfChannels == 2)
    {
        bufL.allocate(numberOfFrames);
        bufR.allocate(numberOfFrames);
        bufferL = bufL.data();
        bufferR = bufR.data();

        bufferList->mBuffers[0].mNumberChannels = 1;
        bufferList->mBuffers[0].mDataByteSize = numberOfFrames * sizeof(float);
        bufferList->mBuffers[0].mData = bufferL;

        bufferList->mBuffers[1].mNumberChannels = 1;
        bufferList->mBuffers[1].mDataByteSize = numberOfFrames * sizeof(float);
        bufferList->mBuffers[1].mData = bufferR;
    }
    else
    {
        ASSERT(!mixToMono || numberOfChannels == 1);

        // for True-stereo (numberOfChannels == 4)
        for (size_t i = 0; i < numberOfChannels; ++i)
        {
            audioBus->channel(i)->mutableData();
        }
    }

    if (mixToMono && numberOfChannels == 2)
    {
        // Mix stereo down to mono
        float * destL = audioBus->channel(0)->mutableData();
        for (size_t i = 0; i < numberOfFrames; i++)
            destL[i] = 0.5f * (bufferL[i] + bufferR[i]);
    }

    return audioBus;
}
    
std::unique_ptr<AudioBus> MakeBusFromMemory(const void* data, size_t dataSize, bool mixToMono, float sampleRate)
{
    AudioFileReader reader(data, dataSize);
    return reader.createBus(sampleRate, mixToMono);
}

std::unique_ptr<AudioBus> MakeBusFromFile(const char* filePath, bool mixToMono, float sampleRate)
{
    AudioFileReader reader(filePath);
    return reader.createBus(sampleRate, mixToMono);
}
    


} // WebCore
