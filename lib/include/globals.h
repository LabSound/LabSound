#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <memory>
#include "LabSound/extended/LabSound.h"

using namespace std;

namespace webaudio {
  extern unique_ptr<lab::AudioContext> defaultAudioContext;
}

#endif