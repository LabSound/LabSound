#ifndef AudioFileReaderMac_h
#define AudioFileReaderMac_h

#include <memory>

namespace WebCore
{

class AudioBus;

// For both create functions:
// Pass in 0.0 for sampleRate to use the file's sample-rate, otherwise a sample-rate conversion to the requested
// sampleRate will be made (if it doesn't already match the file's sample-rate).
// The created buffer will have its sample-rate set correctly to the result.
std::unique_ptr<AudioBus> MakeBusFromMemory(const void * data, size_t dataSize, bool mixToMono, float sampleRate);

std::unique_ptr<AudioBus> MakeBusFromFile(const char * filePath, bool mixToMono, float sampleRate);

// May pass in 0.0 for sampleRate in which case it will use the AudioBus's sampleRate
void writeBusToAudioFile(AudioBus* bus, const char* filePath, double fileSampleRate);

class AudioFileReader
{
    
public:
    
    AudioFileReader(const char * filePath);
    AudioFileReader(const void * data, size_t dataSize);
    
    ~AudioFileReader();

    // Returns nullptr if error
    std::unique_ptr<AudioBus> createBus(float sampleRate, bool mixToMono);
    
private:
    
    void readInternal();

};

} // namespace WebCore
 
#endif // AudioFileReaderMac_h
