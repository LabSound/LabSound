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

Generally, you can take any js sample code, and transliterate it exactly into C++. In some cases,
there are extra parameters to specify, such as the input and output indices in the 
oscillator example above.

License
-------
Any bits not part of the WebKit code (such as the files in the shim) directory can
be assumed to be under a BSD 3-clause license. <http://opensource.org/licenses/BSD-3-Clause>

The license on the WebKit files is the Google/Apple BSD 2 clause modified license on
most of the Webkit sources. Note that the WTF library in particular is intended to be
used outside of WebKit.  cf <http://lists.macosforge.org/pipermail/webkit-dev/2007-November/002729.html>

