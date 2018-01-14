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
    std::unique_ptr<AudioBus> MakeBusFromFile(const char * filePath, bool mixToMono);
    std::unique_ptr<AudioBus> MakeBusFromMemory(const std::vector<uint8_t> & buffer, std::string extension, bool mixToMono);
}

#endif
