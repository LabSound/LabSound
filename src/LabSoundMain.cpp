
#include "AudioContext.h"
#include "ExceptionCode.h"
#include "MainThread.h"

#include "AudioBufferSourceNode.h"
#include "OscillatorNode.h"

#include <unistd.h>

using namespace WebCore;

namespace WebCore
{
    class Document { public: };
}

class Sound
{
    RefPtr<AudioBuffer> audioBuffer;
    RefPtr<AudioBufferSourceNode> sourceBuffer;

public:
    Sound(RefPtr<AudioContext> context, const char* path)
    {
        FILE* f = fopen("tonbi.wav", "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            int l = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t* data = new uint8_t[l];
            fread(data, 1, l, f);
            fclose(f);
            
            ExceptionCode ec;
            bool mixToMono = true;
            PassRefPtr<ArrayBuffer> fileDataBuffer = ArrayBuffer::create(data, l);
            delete [] data;

            // create an audio buffer from the file data. The file data will be
            // parsed, and does not need to be retained.
            audioBuffer = context->createBuffer(fileDataBuffer.get(), mixToMono, ec);
            sourceBuffer = context->createBufferSource();
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(audioBuffer.get());
            
            // bus the sound to the mixer.
            sourceBuffer->connect(context->destination(), 0, 0, ec);
        }
    }
    
    ~Sound()
    {
        void stop();
    }
    
    void play(float when = 0.0f)
    {
        if (sourceBuffer)
            sourceBuffer->start(when);
    }
    
    void stop(float when = 0.0f)
    {
        if (sourceBuffer)
            sourceBuffer->stop(when);
    }
    
};

int main(int, char**)
{
    // Initialize threads for the WTF library
    WTF::initializeThreading();
    WTF::initializeMainThread();
    
    // Create an audio context object
    Document d;
    ExceptionCode ec;
    RefPtr<AudioContext> context = AudioContext::create(&d, ec);
#if 0
    RefPtr<OscillatorNode> oscillator = context->createOscillator();
    oscillator->connect(context->destination(), 0, 0, ec);
    oscillator->start(0);   // play now
    for (int i = 0; i < 300; ++i)
        usleep(10000);
#else
    Sound tonbi(context, "tonbi.wav");
    Sound tonbi2(context, "tonbi.wav");
    tonbi.play(0);
    tonbi2.play(1.25f);
    for (int i = 0; i < 300; ++i)
        usleep(10000);
#endif
    
    return 0;
}
