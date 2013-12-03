
#include "LabSound/LabSound.h"

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();

    toneAndSample(context, 3.0f);
    
    return 0;
}
