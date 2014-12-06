
#include "LabSound.h"
#include "Rhythm.h"

using namespace LabSound;

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();
    rhythm(context, 3.0f);

    return 0;
}
