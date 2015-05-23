#include "internal/AudioFileReader.h"

#include "internal/ConfigMacros.h"
#include "internal/AudioBus.h"
#include "internal/AudioFileReader.h"
#include "internal/FloatConversion.h"

#include "libnyquist/AudioDecoder.h"

namespace WebCore
{

nqr::NyquistIO nyquistFileIO;
std::mutex g_fileIOMutex;
    
std::unique_ptr<AudioBus> MakeBusFromFile(const char * filePath, bool mixToMono, float sampleRate)
{
    std::lock_guard<std::mutex> lock(g_fileIOMutex);
    
    nqr::AudioData * audioData = new nqr::AudioData();
    
    // Perform audio decode
    int result = nyquistFileIO.Load(audioData, std::string(filePath));
    
    // Check OK
    if (result == nqr::IOError::NoError)
    {
        
        size_t numSamples = audioData->samples.size();
        size_t numberOfFrames = int(numSamples / audioData->channelCount);
        size_t busChannelCount = mixToMono ? 1 : (audioData->channelCount);
        
        std::vector<float> planarSamples(numSamples);
        
        // Deinterleave into LabSound/WebAudio planar channel layout
        nqr::DeinterleaveChannels(audioData->samples.data(), planarSamples.data(), numberOfFrames, audioData->channelCount, numberOfFrames);
        
        // Create AudioBus where we'll put the PCM audio data
        std::unique_ptr<AudioBus> audioBus(new AudioBus(busChannelCount, numberOfFrames));
        audioBus->setSampleRate(audioData->sampleRate);
        
        for (size_t i = 0; i < busChannelCount; ++i)
        {
            memcpy(audioBus->channel(i)->mutableData(), planarSamples.data() + ( (i) * numberOfFrames), numberOfFrames * sizeof(float));
        }
        
        delete audioData;
        
        return audioBus;
    }
    else
    {
        throw std::runtime_error("Nyquist File IO Error: " + std::to_string(result));
    }
    
    
    // Todo: the following should be moved into ReadInternal.
    // Then, copy data into the audio bus and perform any mixing/normalization operations that we
    // didn't do when we read the file or memory into LabSound-friendly internal formats
    
    /*

     // De-interleave libnyquist output => channelData
     
     
     //
     // Only allocated if mono
     AudioFloatArray monoMix;
     
     if (mixToMono && numberOfChannels == 2)
     {
     monoMix.allocate(numberOfFrames);
     }
     else
     {
     ASSERT(!mixToMono || numberOfChannels == 1);
     
     // for True-stereo (numberOfChannels == 4)

     }
     
     if (mixToMono && numberOfChannels == 2)
     {
     // Mix stereo down to mono
     }
     */
    
}
    
} // end namespace WebCore
