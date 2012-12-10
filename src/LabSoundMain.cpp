
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

class SoundBuffer
{
public:
    RefPtr<AudioBuffer> audioBuffer;
    RefPtr<AudioContext> context;

    SoundBuffer(RefPtr<AudioContext> context, const char* path)
    : context(context)
    {
        FILE* f = fopen(path, "rb");
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
        }
    }
    
    ~SoundBuffer()
    {
    }
    
    PassRefPtr<AudioBufferSourceNode> play(float when = 0.0f)
    {
        if (audioBuffer) {
            RefPtr<AudioBufferSourceNode> sourceBuffer;
            sourceBuffer = context->createBufferSource();
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(audioBuffer.get());
            
            // bus the sound to the mixer.
            ExceptionCode ec;
            sourceBuffer->connect(context->destination(), 0, 0, ec);
            sourceBuffer->start(when);
            return sourceBuffer;
        }
        return 0;
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
    for (int i = 0; i < 100; ++i)
        usleep(10000);

    SoundBuffer tonbi(context, "tonbi.wav");
    tonbi.play(0);

    for (int i = 0; i < 300; ++i)
        usleep(10000);
#endif
    SoundBuffer kick(context, "kick.wav");
    SoundBuffer hihat(context, "hihat.wav");
    SoundBuffer snare(context, "snare.wav");

    float startTime = 0;
    float eighthNoteTime = 1.0f/4.0f;
    for (int bar = 0; bar < 2; bar++) {
        float time = startTime + bar * 8 * eighthNoteTime;
        // Play the bass (kick) drum on beats 1, 5
        kick.play(time);
        kick.play(time + 4 * eighthNoteTime);
        
        // Play the snare drum on beats 3, 7
        snare.play(time + 2 * eighthNoteTime);
        snare.play(time + 6 * eighthNoteTime);
        
        // Play the hi-hat every eighth note.
        for (int i = 0; i < 8; ++i) {
            hihat.play(time + i * eighthNoteTime);
        }
    }
    for (int i = 0; i < 300; ++i)
        usleep(10000);
    
    return 0;
}
