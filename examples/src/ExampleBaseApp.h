#pragma once

#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <stdint.h>

// In the future, this class could do all kinds of clever things, like setting up the context,
// handling recording functionality, etc.

struct LabSoundExampleApp
{
    ExceptionCode ec;
    virtual void PlayExample() = 0;
};
