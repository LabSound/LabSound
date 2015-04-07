#pragma once

#include "LabSound.h"
#include "LabSoundIncludes.h"
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
    std::array<int, 8> majorScale = {0, 2, 4, 5, 7, 9, 11, 12};
    std::array<int, 8> naturalMinorScale = {0, 2, 3, 5, 7, 9, 11, 12};
    std::array<int, 6> pentatonicMajor = { 0, 2, 4, 7, 9, 12 };
    std::array<int, 8> pentatonicMinor = { 0, 3, 5, 7, 10, 12 };
    std::array<int, 8> delayTimes = { 266, 533, 399 };
    
    ExceptionCode ec;
    virtual void PlayExample() = 0;
    
    float MidiToFrequency(int midiNote)
    {
        return 440.0 * pow(2.0, (midiNote - 57.0) / 12.0);
    }
    
    std::mt19937 randomgenerator;
};
