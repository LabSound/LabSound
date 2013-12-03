
#include "LabSound/LabSound.h"

// Examples
#include "ReverbSample.h"

using namespace LabSound;

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();

    reverbSample(context, 10.0f);
    
    return 0;
}
