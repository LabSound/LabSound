#ifndef AudioFileReader_H
#define AudioFileReader_H

#include <memory>
#include <vector>
#include <stdint.h>

namespace WebCore
{

class AudioBus;

std::unique_ptr<AudioBus> MakeBusFromFile(const char * filePath, bool mixToMono, float sampleRate);
std::unique_ptr<AudioBus> MakeBusFromMemory(const std::vector<uint8_t> & buffer, bool mixToMono, float sampleRate);

} // end namespace WebCore

#endif // AudioFileReader_H
