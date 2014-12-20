
#include "LabSoundIncludes.h"
#include "LiveReverbRecording.h"

int main(int, char**) {
    auto context = LabSound::init();
    liveReverbRecording(context, 10.0f, "liveReverb.raw");
    return 0;
}
