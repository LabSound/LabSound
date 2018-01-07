// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"

#include "internal/AudioFileReader.h"
#include "internal/ConfigMacros.h"
#include "internal/AudioFileReader.h"
#include "internal/FloatConversion.h"

#include "libnyquist/AudioDecoder.h"

namespace detail
{
    std::unique_ptr<lab::AudioBus> LoadInternal(nqr::AudioData * audioData, bool mixToMono)
    {
        size_t numSamples = audioData->samples.size();
        if (!numSamples) return nullptr;

        size_t numberOfFrames = int(numSamples / audioData->channelCount);
        const size_t busChannelCount = mixToMono ? 1 : (audioData->channelCount);
        
        std::vector<float> planarSamples(numSamples);

        // Create AudioBus where we'll put the PCM audio data
        std::unique_ptr<lab::AudioBus> audioBus(new lab::AudioBus(busChannelCount, numberOfFrames));
        audioBus->setSampleRate(audioData->sampleRate);
        
        // Deinterleave stereo into LabSound/WebAudio planar channel layout
        nqr::DeinterleaveChannels(audioData->samples.data(), planarSamples.data(), numberOfFrames, audioData->channelCount, numberOfFrames);
        
        // Mix to mono if stereo -- easier to do in place instead of using libnyquist helper functions
        // because we've already deinterleaved
        if (audioData->channelCount == 2 && mixToMono)
        {
            float * destinationMono = audioBus->channel(0)->mutableData();
            float * leftSamples = planarSamples.data();
            float * rightSamples = planarSamples.data() + numberOfFrames;
            
            for (size_t i = 0; i < numberOfFrames; i++)
                destinationMono[i] = 0.5f * (leftSamples[i] + rightSamples[i]);
        }
        else
        {
            for (size_t i = 0; i < busChannelCount; ++i)
            {
                memcpy(audioBus->channel(i)->mutableData(), planarSamples.data() + (i * numberOfFrames), numberOfFrames * sizeof(float));
            }
        }
        
        delete audioData;
        
        return audioBus;
    }
}

namespace lab
{

nqr::NyquistIO nyquistFileIO;
std::mutex g_fileIOMutex;
    
std::unique_ptr<AudioBus> MakeBusFromFile(const char * filePath, bool mixToMono)
{
    std::lock_guard<std::mutex> lock(g_fileIOMutex);
    nqr::AudioData * audioData = new nqr::AudioData();
    nyquistFileIO.Load(audioData, std::string(filePath));
    return detail::LoadInternal(audioData, mixToMono);
}

std::unique_ptr<AudioBus> MakeBusFromMemory(const std::vector<uint8_t> & buffer, std::string extension, bool mixToMono)
{
    std::lock_guard<std::mutex> lock(g_fileIOMutex);
    nqr::AudioData * audioData = new nqr::AudioData();
    nyquistFileIO.Load(audioData, extension, buffer);
    return detail::LoadInternal(audioData, mixToMono);
}
    
} // end namespace lab
