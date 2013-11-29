LabSound
========

The WebAudio engine, factored out of WebKit.

My motivation for factoring it out is that the WebAudio engine is very well designed,
and well exercised. It does everything I would want an audio engine to do. If you have
a look at the specification <https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html>
you can see that it is an audio processing graph with a lot of default nodes, multiple
sends, and wide platform support. See also <http://docs.webplatform.org/wiki/apis/webaudio>

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

Building on OSX using premake
-----------------------------

The premake support for xcode4 is very preliminary, and is apparently going to be
rewritten soon. Hopefully the build set up will be better then.

Typing 

    premake4 xcode4 
    
mostly works, but it doesn't copy the audio resources into the app bundle. At the moment,
you'll need to build the code once, then manually open the App and bundle, and copy
the audio in yourself.


Building on OSX using supplied xcode workspace
----------------------------------------------

Open and build in the usual way.


Copying the HRTF database
-------------------------

    Application
       |
       +--- Contents
               |
               +--- Resources
                       |
                       +--- HRTF
                              |
                              +---- copy all the wav's here

The wavs to copy are in src/platform/audio/resources/*.wav

You'll need to do this manually for each sample you compile. Since these samples
comprise 512k of data, I think I'm going to work out how not to have to do this, but
allow it to work this way for when bundling in the resources does make sense.

Note also that the sample data needs to be in the current working directory when you
run the samples.

As an alternative, if the HRTF folder is not found in the resource bundle, LabSound will 
check in the cwd for a folder called HRTF, and if found it will fetch the data from there.

Usage
-----

After factoring free of WebKit, the audio engine is easy to use. Here's a
sample that plays a sine tone for a few seconds.

    #include "LabSound.h"
    #include "OscillatorNode.h"
    
    #include <unistd.h> // for usleep
    
    using namespace WebCore;

    int main(int, char**)
    {
        LabSound::AudioContextPtr audioContext = LabSound::init();
        PassRefPtr<OscillatorNode> oscillator = context->createOscillator();
        LabSound::connect(oscillator.get(), context->destination());
        oscillator->start(0);
        
        for (int i = 0; i < 300; ++i)
            usleep(10000);
        
        return 0;
    }

The key is to use the interfaces in LabSound.h, and if what you need isn't there, stick to the 
headers in the src/Modules/webaudio. These are the ones that correspond to the API you'd use 
from javascript.

Generally, you can take any js sample code, and transliterate it exactly into C++. In some cases,
there are extra parameters to specify, such as the input and output indices in the 
oscillator example above.

Playing a sound file is slightly more involved, but not much. Replace main with: 

    int main(int, char**)
    {
        LabSound::AudioContextPtr audioContext = LabSound::init();
    
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
            LabSound::SoundBuffer* dataBuffer = context->createBuffer(dataFileBuffer.get(), mixToMono, ec);
            LabSound::AudioSoundBufferPtr sourceBuffer = context->createBufferSource();
            sourceBuffer->setBuffer(dataBuffer.get());
            LabSound::connect(sourceBuffer, context->destination());
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
    
    class SoundBuffer
    {
    public:
        LabSound::SoundBuffer* audioBuffer;
        LabSound::AudioContextPtr context;
    
        SoundBuffer(LabSound::AudioContextPtr context, const char* path)
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
                LabSound::AudioSoundBufferPtr sourceBuffer = context->createBufferSource();
                
                // Connect the source node to the parsed audio data for playback
                sourceBuffer->setBuffer(audioBuffer.get());
                
                // bus the sound to the mixer.
                LabSound::connect(sourceBuffer, context->destination());
                sourceBuffer->start(when);
                return sourceBuffer;
            }
            return 0;
        }
        
    };
    
    int main(int, char**)
    {
        LabSound::AudioContextPtr context = LabSound::init();
    
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

    LabSound::AudioSoundBufferPtr play(float start, float end, float when = 0.0f)
    {
        if (audioBuffer) {
            if (end == 0)
                end = audioBuffer->duration();
            
            LabSound::AudioSoundBufferPtr sourceBuffer;
            sourceBuffer = context->createBufferSource();
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(audioBuffer.get());
            
            // bus the sound to the mixer.
            LabSound::connect(sourceBuffer, context->destination());
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

Bussing
-------

Let's modify the play routine to allow bussing to different nodes.

    LabSound::AudioSoundBufferPtr play(AudioNode* outputNode, float when = 0.0f)
    {
        if (audioBuffer) {
            RefPtr<AudioBufferSourceNode> sourceBuffer;
            sourceBuffer = context->createBufferSource();
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(audioBuffer.get());
            
            // bus the sound to the mixer.
            ExceptionCode ec;
            LabSound::connect(sourceBuffer, outputNode);
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

Now, we can write a routine that fades from one source to another. Following 
<http://www.html5rocks.com/en/tutorials/webaudio/intro/#toc-xfade-ep> let's implement
equal power cross fading. We'll create two gain nodes, and route them to the context's
destinationNode. We'll create an oscillator and bus it to one gain node, and we'll play
the drum notes from a previous demo into the other. We'll ramp one up whilst ramping the
other down.
    
    int main(int, char**)
    {
        LabSound::AudioContextPtr context = LabSound::init();
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
        
        // update gain at 10ms intervals, using equal power cross fading
        for (float i = 0; i < 10.0f; i += 0.01f) {
            float t1 = i / 10.0f;
            float t2 = 1.0f - t1;
            float gain1 = cosf(t1 * 0.5f * M_PI);
            float gain2 = cosf(t2 * 0.5f * M_PI);
            oscGain->gain()->setValue(gain1);
            drumGain->gain()->setValue(gain2);
            usleep(10000);
        }
        
        return 0;
    }

Filtering
---------
A filter node can be inserted into a processing graph. A simple modification to the drums
sample demonstrates it by subjecting the drums to a 500Hz lowpass filter:

    SoundBuffer kick(context, "kick.wav");
    SoundBuffer hihat(context, "hihat.wav");
    SoundBuffer snare(context, "snare.wav");
    
    RefPtr<BiquadFilterNode > filter = context->createBiquadFilter();
    filter->setType(BiquadFilterNode::LOWPASS, ec);
    filter->frequency()->setValue(500.0f);
    filter->connect(context->destination(), 0, 0, ec);
    
    float startTime = 0;
    float eighthNoteTime = 1.0f/4.0f;
    for (int bar = 0; bar < 2; bar++) {
        float time = startTime + bar * 8 * eighthNoteTime;
        // Play the bass (kick) drum on beats 1, 5
        kick.play(filter.get(), time);
        kick.play(filter.get(), time + 4 * eighthNoteTime);
        
        // Play the snare drum on beats 3, 7
        snare.play(filter.get(), time + 2 * eighthNoteTime);
        snare.play(filter.get(), time + 6 * eighthNoteTime);
        
        // Play the hi-hat every eighth note.
        for (int i = 0; i < 8; ++i) {
            hihat.play(filter.get(), time + i * eighthNoteTime);
        }
    }
    for (float i = 0; i < 5.0f; i += 0.01f) {
        usleep(10000);
    } 

Refer to the headers for interfaces on the biquad filter node.

Spatialization
--------------

The spatialization API is straight forward.

    SoundBuffer train(context, "trainrolling.wav");
    RefPtr<PannerNode> panner = context->createPanner();
    panner->connect(context->destination(), 0, 0, ec);
    PassRefPtr<AudioBufferSourceNode> trainNode = train.play(panner.get(), 0.0f);
    trainNode->setLooping(true);
    context->listener()->setPosition(0, 0, 0);
    panner->setVelocity(15, 0, 0);
    for (float i = 0; i < 10.0f; i += 0.01f) {
        float x = (i - 5.0f) / 5.0f;
        // keep it a bit up and in front, because if it goes right through the listener
        // at (0, 0, 0) it abruptly switches from left to right.
        panner->setPosition(x, 0.1f, 0.1f);
        usleep(10000);
    }
    
This sample moves a sample from left to right versus a listener located at (0, 0, 0). 
An artificially high velocity is put on the panner node to exaggerate the Doppler shift.
It appears that the spatialization is limited to stereo, with no support for surround
encoding. It does however support Head Related Transfer Functions, which through headphones
is probably the best way to hear spatialized audio.

Live Audio Input
----------------

The default input device can be hooked up as follows:

    RefPtr<MediaStreamAudioSourceNode> input = context->createMediaStreamSource(new MediaStream(), ec);
    input->connect(context->destination(), 0, 0, ec);

This simple example merely wires the live audio input directly to the output.

Reverb - Impulse Response Convolution
-------------------------------------

Reverb effects are generated by convolving the input buffer with an impulse response
recording. Those recordings are generally created by recording an impulse in a specific
environment. This sample plays a sound through a convolution:

    SoundBuffer ir(context, "impulse-responses/tim-warehouse/cardiod-rear-35-10/cardiod-rear-levelled.wav");    
    SoundBuffer sample(context, "human-voice.mp4");
    
    RefPtr<ConvolverNode> convolve = context->createConvolver();
    convolve->setBuffer(ir.audioBuffer.get());
    RefPtr<GainNode> wetGain = context->createGain();
    wetGain->gain()->setValue(2.f);
    RefPtr<GainNode> dryGain = context->createGain();
    dryGain->gain()->setValue(1.f);
    
    convolve->connect(wetGain.get(), 0, 0, ec);
    wetGain->connect(context->destination(), 0, 0, ec);
    dryGain->connect(context->destination(), 0, 0, ec);
    
    sample.play(convolve.get(), 0);

Note the technique of mixing wet (convolved) and dry (unprocessed) signals into the 
destination. This simulates listening to a sound in an environment with echo.

The sounds referenced in this sample come from the Chromium sources. Sample sounds are here
<http://chromium.googlecode.com/svn/trunk/samples/audio/sounds/>, and there are a great many 
impulse response files available here:
<http://chromium.googlecode.com/svn/trunk/samples/audio/impulse-responses/>
Note that a few of the samples there have a different license than the license covering the
rest of the Chrome codebase.

Passing the output of the live audio into the reverb node doesn't sound right, more work
is necessary.

Headers
-------
Headers required by various portions of this tutorial are:

    // For starting the WTF library
    #include <wtf/ExportMacros.h>
    #include "MainThread.h"
    
    // webaudio specific headers
    #include "AudioBufferSourceNode.h"
    #include "AudioContext.h"
    #include "BiquadFilterNode.h"
    #include "ExceptionCode.h"
    #include "GainNode.h"
    #include "MediaStream.h"
    #include "MediaStreamAudioSourceNode.h"
    #include "OscillatorNode.h"
    #include "PannerNode.h"

License
-------
Any bits not part of the WebKit code (such as the files in the shim) directory can
be assumed to be under a BSD 3-clause license. <http://opensource.org/licenses/BSD-3-Clause>

The license on the WebKit files is the Google/Apple BSD 2 clause modified license on
most of the Webkit sources. Note that the WTF library in particular is intended to be
used outside of WebKit.  cf <http://lists.macosforge.org/pipermail/webkit-dev/2007-November/002729.html>

