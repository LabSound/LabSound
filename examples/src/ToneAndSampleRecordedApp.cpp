
#include "LabSound/LabSound.h"

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();

    toneAndSampleRecorded(context, 3.0f, "toneAndSample.raw");
    
    return 0;
}
