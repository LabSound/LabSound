

#include "LabSoundIncludes.h"
#include "ToneAndSampleRecorded.h"
using namespace LabSound;

void toneAndSampleRecorded(RefPtr<AudioContext> context, float seconds, char const*const path) {
    //--------------------------------------------------------------
    // Play a tone and a sample at the same time to recorder node
    //
    ExceptionCode ec;
    
    RefPtr<OscillatorNode> oscillator = context->createOscillator();
    oscillator->start(0);   // play now
    
    SoundBuffer tonbi(context, "tonbi.wav");
    
    RefPtr<RecorderNode> recorder = RecorderNode::create(context.get(), 44100);
    recorder->startRecording();
    oscillator->connect(recorder.get(), 0, 0, ec);
    tonbi.play(recorder.get(), 0.0f);
    
    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
    
    recorder->stopRecording();
    std::vector<float> data;
    recorder->getData(data);
    FILE* f = fopen(path, "wb");
    if (f) {
        fwrite(&data[0], 1, data.size(), f);
        fclose(f);
    }
}
