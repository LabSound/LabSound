
#include "LabSoundIncludes.h"
#include "RhythmFiltered.h"

using namespace LabSound;

int main(int, char**) {
    auto context = LabSound::init();
    rhythmFiltered(context, 3.0f);
    return 0;
}
