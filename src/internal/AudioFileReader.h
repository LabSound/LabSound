#ifndef AudioFileReader_H
#define AudioFileReader_H

#include <memory>

namespace WebCore
{

class AudioBus;

std::unique_ptr<AudioBus> MakeBusFromFile(const char * filePath, bool mixToMono, float sampleRate);
 
} // end namespace WebCore

#endif // AudioFileReader_H
