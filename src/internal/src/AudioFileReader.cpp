#include "internal/AudioFileReader.h"

#include "internal/ConfigMacros.h"
#include "internal/AudioBus.h"
#include "internal/AudioFileReader.h"
#include "internal/FloatConversion.h"

//#include "libnyquist headers here"

namespace WebCore {

AudioFileReader::AudioFileReader(const char * filePath)
{
   // Read Internal
}

AudioFileReader::AudioFileReader(const void * data, size_t dataSize)
{
    // Read Internal
}

AudioFileReader::~AudioFileReader()
{

}

std::unique_ptr<AudioBus> AudioFileReader::createBus(float sampleRate, bool mixToMono)
{
    // Todo: the following should be moved into ReadInternal.
    // Then, copy data into the audio bus and perform any mixing/normalization operations that we
    // didn't do when we read the file or memory into LabSound-friendly internal formats
    
    /*
    // Number of channels
    size_t numberOfChannels = m_fileDataFormat.mChannelsPerFrame;
    
    // Sample-rate
    double fileSampleRate = m_fileDataFormat.mSampleRate;

    // Samples per channel
    size_t numberOfFrames = static_cast<size_t>(numberOfFrames);

    size_t busChannelCount = mixToMono ? 1 : numberOfChannels;

    // Create AudioBus where we'll put the PCM audio data
    std::unique_ptr<AudioBus> audioBus(new AudioBus(busChannelCount, numberOfFrames));
    audioBus->setSampleRate(narrowPrecisionToFloat(fileSampleRate));

    // De-interleave libnyquist output => channelData
    
    // Planar
    for (int c = 0; c < numberOfChannels; ++c)
    {
        AudioFloatArray channelData; // .data()
        channelData.allocate(numberOfFrames);
        
    }

    
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
        for (size_t i = 0; i < numberOfChannels; ++i)
        {
            audioBus->channel(i)->mutableData();
        }
    }

    if (mixToMono && numberOfChannels == 2)
    {
        // Mix stereo down to mono
    }
    */
    
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
