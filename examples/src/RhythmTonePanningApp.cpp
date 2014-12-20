
#include "LabSoundIncludes.h"
#include "RhythmTonePanning.h"

using namespace LabSound;

int main(int, char**)
{
    auto context = LabSound::init();
    rhythmTonePanning(context, 10.0f);
    return 0;
}
