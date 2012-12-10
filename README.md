LabSound
========

The WebAudio engine, factored out of WebKit.

My motivation for factoring it out is that the WebAudio engine is very well designed,
and well exercised. It does everything I would want an audio engine to do. If you have
a look at the specification <https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html>
you can see that it is an audio processing graph with a lot of default nodes, multiple
sends, and wide platform support.

WebKit was obtained from the unofficial github mirror at <https://github.com/WebKit/webkit>

The WebKit distro is about 2Gb of stuff, the audio engine is about 32Mb including
the impulse response wav files. 

I disconnected the audio engine from Javascript and from the browser infrastructure,
and also replaced a few files with new implementations where those files had an LGLPL
notice on them.

I only made an OSX main because I'm not set up for building on other platforms. If
anyone wants to modify the premake.lua file to support other platforms, please send
a pull request!

The keyword LabSound marks any places the code was modified from the source distro.

Building
--------

So far, I haven't got a handle fully on creating an xcode project using premake.

Typing 

    premake4 xcode4 
    
mostly works, but it doesn't copy the audio resources into the app bundle. At the moment,
you'll need to build the code once, then manually open the App and bundle, and copy
the audio in yourself.

    LabSound
       |
       +--- Contents
               |
               +--- Resources
                       |
                       +--- audio
                              |
                              +---- copy all the wav's here

The wavs to copy are in src/platform/audio/resources/*.wav

Usage
-----

After factoring free of WebKit, the audio engine is ridiculously easy to use. Here's a
sample that plays a sine tone for a few seconds.

    #include "AudioContext.h"
    #include "ExceptionCode.h"
    #include "MainThread.h"
    #include "OscillatorNode.h"
    
    #include <unistd.h> // for usleep
    
    using namespace WebCore;
    
    namespace WebCore
    {
        class Document { public: }; // dummy class
    }
    
    int main(int, char**)
    {
        // Initialize threads for the WTF library
        WTF::initializeThreading();
        WTF::initializeMainThread();
        
        // Create an audio context object
        Document d;
        ExceptionCode ec;
        PassRefPtr<AudioContext> context = AudioContext::create(&d, ec);
    
        PassRefPtr<OscillatorNode> oscillator = context->createOscillator();
        oscillator->connect(context->destination(), 0, 0, ec);
        oscillator->start(0);
        
        for (int i = 0; i < 300; ++i)
            usleep(10000);
        
        return 0;
    }

The key is to stick to the headers in the src/Modules/webaudio. These are the ones that
correspond to the API you'd use from javascript.

Generally, you can take any js sample code, and transliterate it exactly into C++. In some cases,
there are extra parameters to specify, such as the input and output indices in the 
oscillator example above.

Playing a sound file is slightly more involved, but not much. Replace main with: 

    int main(int, char**)
    {
        // Initialize threads for the WTF library
        WTF::initializeThreading();
        WTF::initializeMainThread();
        
        // Create an audio context object
        Document d;
        ExceptionCode ec;
        RefPtr<AudioContext> context = AudioContext::create(&d, ec);
    
        FILE* f = fopen("sample.wav", "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            int l = ftell(f);
            fseek(f, 0, SEEK_SET);
            char* a = new char[l];
            fread(a, 1, l, f);
            fclose(f);
            
            ExceptionCode ec;
            bool mixToMono = true;
            PassRefPtr<ArrayBuffer> dataFileBuffer = ArrayBuffer::create(a, l);
            delete [] a;
            RefPtr<AudioBuffer> dataBuffer = context->createBuffer(dataFileBuffer.get(), mixToMono, ec);
            RefPtr<AudioBufferSourceNode> sourceBuffer = context->createBufferSource();
            sourceBuffer->setBuffer(dataBuffer.get());
            sourceBuffer->connect(context->destination(), 0, 0, ec);
            sourceBuffer->start(0); // play now
            
            for (int i = 0; i < 300; ++i)
                usleep(10000);
        }
        return 0;
    }

Substitute your own .wav file where it says "tonbi.wav", and make sure the file is located
where fopen can find it. Compare this routine to the Loading Sounds and Playing Sounds
section on this webaudio tutorial: <http://www.html5rocks.com/en/tutorials/webaudio/intro/>

Smart Pointers and Memory Management
------------------------------------

Notice that most of the APIs in webaudio return a PassRefPtr. These are smart pointers 
that have the behavior that if they are assigned to another PassRefPtr, they will become
zero, and the pointer they were assigned to will become non zero. PassRefPtrs can be
assigned to RefPtrs, which is a smart pointer that can take ownership of memory from a
PassRefPtr, and then it will stick around until there are no more references. Assigning
one RefPtr to another will increment reference counts. Be sure to manage your pointers 
carefully. In the example above, you'll notice that the dataFileBuffer is a PassRefPtr
because it can disappear as soon as a buffer has been created from it. The dataBuffer
and the sourceBuffer both need to exist until the sound is finished playing however, 
because the buffer will play data from the databuffer.

Another thing you'll probably notice is that webaudio objects are never created via new;
instead there are either create methods on the context, or static factory methods, such
as that on ArrayBuffer. The code is designed that way so that data won't leak, and that
an application using the objects will shut down cleanly. 

Playing a SoundBuffer Multiple Times
------------------------------------

Here's an example like the Playing Sounds With Rhythm sample from HTML5 Rocks:
<http://www.html5rocks.com/en/tutorials/webaudio/intro/#toc-abstract>
    
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
    
        SoundBuffer kick(context, "kick.wav");
        SoundBuffer hihat(context, "hihat.wav");
        SoundBuffer snare(context, "snare.wav");
    
        // Create a bunch of nodes that will play two bars of a rhythm.
        float startTime = 0;
        float eighthNoteTime = 1.0f/4.0f;
        for (int bar = 0; bar < 2; ++bar) {
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

Notice that every time play is called, a new AudioBufferSourceNode is created and connected
to the context's destination node. When the node finishes, it automatically causes reference
counting decrementing that results in the node's deallocation. The play routine returns
a PassRefPtr which can be used by the caller if commands like stop need to be called on it.

This behavior is worth remembering; although a AudioBufferSourceNode can be set to loop
constantly, it doesn't have a behavior of playing, stopping at the end, and waiting to be
triggered again. Accordingly, the stop() method stops, and deallocates. The node does not
have pause and resume, or set methods to set playback at arbitrary locations.

Playing Part of a Buffer
------------------------

This variant of play starts a sound at a given offset relative to the beginning of the
sample, ends it an offfset (relative to the beginning), and optional delays
the start. If 0 is passed as end, then the sound will play to the end.

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

If you were to write a playback control, starting is simple. Pausing however is a bit
more involved. You'd have to track the time the pause was pressed, and when play is pressed,
play would be called again with the desired start time. Since the AudioBufferSourceNode
doesn't have a method to return the currently playing sample offset, you'll need to 
use a real time clock instead.

License
-------
Any bits not part of the WebKit code (such as the files in the shim) directory can
be assumed to be under a BSD 3-clause license. <http://opensource.org/licenses/BSD-3-Clause>

The license on the WebKit files is the Google/Apple BSD 2 clause modified license on
most of the Webkit sources. Note that the WTF library in particular is intended to be
used outside of WebKit.  cf <http://lists.macosforge.org/pipermail/webkit-dev/2007-November/002729.html>

