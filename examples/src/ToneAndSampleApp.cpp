#include "LabSoundIncludes.h"
#include "ToneAndSample.h"

using namespace LabSound;

int main(int, char**) {
    auto context = LabSound::init();
    toneAndSample(context, 3.0f);
    return 0;
}
