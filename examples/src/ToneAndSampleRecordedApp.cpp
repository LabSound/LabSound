
#include "LabSound.h"

int main(int, char**) {
    auto context = LabSound::init();
    toneAndSampleRecorded(context, 3.0f, "toneAndSample.raw");
    return 0;
}
