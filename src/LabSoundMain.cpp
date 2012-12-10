
#include "AudioContext.h"
#include "ExceptionCode.h"
#include "MainThread.h"

#include "AudioBufferSourceNode.h"
#include "GainNode.h"
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
    
    PassRefPtr<AudioBufferSourceNode> play(AudioNode* outputNode, float when = 0.0f)
    {
        if (audioBuffer) {
            RefPtr<AudioBufferSourceNode> sourceBuffer;
            sourceBuffer = context->createBufferSource();
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(audioBuffer.get());
            
            // bus the sound to the mixer.
            ExceptionCode ec;
            sourceBuffer->connect(outputNode, 0, 0, ec);
            sourceBuffer->start(when);
            return sourceBuffer;
        }
        return 0;
    }
    
    PassRefPtr<AudioBufferSourceNode> play(float when = 0.0f)
    {
        if (audioBuffer) {
            return play(context->destination(), when);
        }
        return 0;
    }
    
    // This variant starts a sound at a given offset relative to the beginning of the
    // sample, ends it an offfset (relative to the beginning), and optional delays
    // the start. If 0 is passed as end, then the sound will play to the end.
    PassRefPtr<AudioBufferSourceNode> play(float start, float end, float when = 0.0f)
    {
        if (audioBuffer) {
            if (end == 0)
                end = audioBuffer->duration();
            
            RefPtr<AudioBufferSourceNode> sourceBuffer;
            sourceBuffer = context->createBufferSource();
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(audioBuffer.get());
            
            // bus the sound to the mixer.
            ExceptionCode ec;
            sourceBuffer->connect(context->destination(), 0, 0, ec);
            sourceBuffer->startGrain(when, start, end - start);
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
#elif 0
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
#else
    RefPtr<OscillatorNode> oscillator = context->createOscillator();

    SoundBuffer kick(context, "kick.wav");
    SoundBuffer hihat(context, "hihat.wav");
    SoundBuffer snare(context, "snare.wav");
    
    RefPtr<GainNode> oscGain = context->createGain();
    oscillator->connect(oscGain.get(), 0, 0, ec);
    oscGain->connect(context->destination(), 0, 0, ec);
    oscGain->gain()->setValue(1.0f);
    oscillator->start(0);
    
    RefPtr<GainNode> drumGain = context->createGain();
    drumGain->connect(context->destination(), 0, 0, ec);
    drumGain->gain()->setValue(1.0f);
    
    float startTime = 0;
    float eighthNoteTime = 1.0f/4.0f;
    for (int bar = 0; bar < 10; bar++) {
        float time = startTime + bar * 8 * eighthNoteTime;
        // Play the bass (kick) drum on beats 1, 5
        kick.play(drumGain.get(), time);
        kick.play(drumGain.get(), time + 4 * eighthNoteTime);
        
        // Play the snare drum on beats 3, 7
        snare.play(drumGain.get(), time + 2 * eighthNoteTime);
        snare.play(drumGain.get(), time + 6 * eighthNoteTime);
        
        // Play the hi-hat every eighth note.
        for (int i = 0; i < 8; ++i) {
            hihat.play(drumGain.get(), time + i * eighthNoteTime);
        }
    }
    
    // update gain at 10ms intervals
    for (float i = 0; i < 10.0f; i += 0.01f) {
        float t1 = i / 10.0f;
        float t2 = 1.0f - t1;
        float gain1 = cosf(t1 * 0.5f * M_PI);
        float gain2 = cosf(t2 * 0.5f * M_PI);
        oscGain->gain()->setValue(gain1);
        drumGain->gain()->setValue(gain2);
        usleep(10000);
    }
#endif
    
    return 0;
}
