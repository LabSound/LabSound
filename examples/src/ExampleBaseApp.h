#pragma once

#include "LabSound/extended/LabSound.h"

#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include <random>
#include <functional>
#include <array>
#include <algorithm>
#include <stdint.h>

// In the future, this class could do all kinds of clever things, like setting up the context,
// handling recording functionality, etc.

struct LabSoundExampleApp
{
    virtual void PlayExample() = 0;
    
    float MidiToFrequency(int midiNote)
    {
        return 440.0f * pow(2.0f, (midiNote - 57.0f) / 12.0f);
    }
    
    std::mt19937 randomgenerator;
};
