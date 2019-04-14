// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioFileReader_H
#define AudioFileReader_H

#include "LabSound/core/AudioBus.h"

#include <memory>
#include <vector>
#include <string>
#include <stdint.h>

namespace lab
{
    // Performs direct filesystem i/o to decode the file (using libnyquist)
    std::shared_ptr<AudioBus> MakeBusFromFile(const char * filePath, bool mixToMono);

    // Loads and decodes a raw binary memory chunk making use of magic numbers to determine filetype. 
    std::shared_ptr<AudioBus> MakeBusFromMemory(const std::vector<uint8_t> & buffer, bool mixToMono);

    // Loads and decodes a raw binary memory chunk where the file extension (mp3, wav, ogg, etc) is aleady known. 
    std::shared_ptr<AudioBus> MakeBusFromMemory(const std::vector<uint8_t> & buffer, std::string extension, bool mixToMono);
}

#endif
