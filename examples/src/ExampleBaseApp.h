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
    ExceptionCode ec;
    virtual void PlayExample() = 0;
    
    float MidiToFrequency(int midiNote)
    {
        return 440.0 * pow(2.0, (midiNote - 57.0) / 12.0);
    }
    
    std::mt19937 randomgenerator;
};
