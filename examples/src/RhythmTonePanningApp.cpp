
#include "LabSound/LabSound.h"
#include "RhythmTonePanning.h"

using namespace LabSound;

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();

    rhythmTonePanning(context, 10.0f);
    
    return 0;
}
