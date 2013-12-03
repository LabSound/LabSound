
#include "LabSound/LabSound.h"
#include "LabSound/AudioContext.h"
#include "SampleSpatialization.h"

using namespace LabSound;

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();
    sampleSpatialization(context, 10.0f);
    return 0;
}
