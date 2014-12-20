
#include "LabSound.h"
#include "SampleSpatialization.h"
#include <iostream>

int main(int, char**)
{
    {
        FILE* test = fopen("trainrolling.wav", "rb");
        if (!test) {
            std::cerr << "Run demo in the examples data folder" << std::endl;
            return 1;
        }
        fclose(test);
    }
    
    auto context = LabSound::init();
    sampleSpatialization(context, 10.0f);
    return 0;
}
