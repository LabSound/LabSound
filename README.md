# LabSound

[![Build Status](https://travis-ci.org/ddiakopoulos/LabSound.svg?branch=master)](https://travis-ci.org/ddiakopoulos/LabSound)

LabSound is a graph-based audio engine built in modern C++11. As a fork of the WebAudio implementation in Chrome (Webkit), LabSound implements the WebAudio API specification while extending it with new features, nodes, bugfixes, and performance improvements. 

The engine is packaged as a batteries-included static library meant for integration in many types of software: games, interactive audio visualizers, audio editing and sequencing applications, and more. 

[LabSound homepage.](http://www.labsound.io/)

# Features

* Compatibility with the [WebAudio API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API)
* Audio asset loading via [libnyquist](https://github.com/ddiakopoulos/libnyquist)
* Binaural audio / HRTF processing
* New audio effects and generators (ADSR, noise, stereo delay, and more)
* Signal analysis (both time & frequency)
* Offline graph processing & wav export
* High-quality realtime signal resampling
* Thread safety guarantees for multi-threaded apps
* Extensible base nodes for arbitrary DSP processing
* Microphone input
* SIMD accelerated channel mixing routines
* DSP primitives including filters and delays

# Platforms

LabSound uses RtAudio as its hardware abstraction layer for realtime audio playback. The repository hosts maintained project files for Visual Studio 2013, Visual Studio 2015, and XCode 7. While not presently supported, LabSound has been shown previously to run on other platforms including Linux (JACK via RtAudio), iOS (CoreAudio), and Android (OpenSL ES). 

Compiling LabSound and its dependencies require a recent C++11 compiler. 

# Building

Users of LabSound are expected to compile LabSound from source. While most dependencies are included as code in the repository, libnyquist is bundled as a git submodule so it is recommended that new users clone the repository with the `--recursive` option. 

# Examples

LabSound is bundled with approximately 20 single-file samples. Platform-specific files for the example project can be found in the `examples\` subfolder. Generally, one might take any WebAudio JavaScript sample code and transliterate it to LabSound with only mild effort (modulo obvious architectual considerations of JavaScript vs C++).

# Using the Library

Users should link against `liblabsound.a` on OSX and `labsound.lib` on Windows. LabSound also requires symbols from libnyquist, although both included Visual Studio and XCode projects will build this dependency alongside LabSound. 

On OSX, new applications also require the following frameworks:
+ Cocoa
+ Accelerate
+ CoreAudio
+ AudioUnit
+ AudioToolbox
+ libnyquist.a

On Windows, new applications also require the following libraries: 
+ dsound.lib
+ dxguid.lib
+ winmm.lib
+ libnyquist.lib

For convenience, `LabSound.h` is used as an index header file with all public nodes included for easy application development. 

LabSound includes an HRTF implementation. This creates an additional dependency on a folder of impulse wav files. These files should be located in a directory called `hrtf/` in the current working directory of the application. 

# License 

LabSound is released under the simplified BSD 2 clause license. All LabSound dependencies are under similar permissive licenses. Further details are located in the `LICENSE` and `COPYING` files. 
