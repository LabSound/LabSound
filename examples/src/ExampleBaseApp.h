#pragma once

#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

struct LabSoundExampleApp
{
    ExceptionCode ec;
    virtual void PlayExample() = 0;
};
