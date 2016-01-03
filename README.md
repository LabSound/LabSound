<p align="center">
  <img src="https://raw.githubusercontent.com/LabSound/LabSound/master/assets/images/labsound_4x3.png"/>
</p>

[![Build Status](https://travis-ci.org/LabSound/LabSound.svg)](https://travis-ci.org/LabSound/LabSound)

LabSound is a graph-based audio engine built using modern C++11. As a fork of the WebAudio implementation in Chrome, LabSound implements the WebAudio API specification while extending it with new features, nodes, bugfixes, and performance improvements. 

The engine is packaged as a batteries-included static library meant for integration in many types of software: games, visualizers, interactive installations, live coding environments, VST plugins, audio editing/sequencing applications, and more. 

[LabSound homepage.](http://www.labsound.io/)

# Features

* Compatibility with the [WebAudio API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API)
* Audio asset loading via [libnyquist](https://github.com/ddiakopoulos/libnyquist)
* Binaural audio via IRCAM HRTF database
* New audio effects and generators (ADSR, noise, stereo delay, and more)
* Signal analysis (both time & frequency)
* Offline graph processing & wav export
* High-quality realtime signal resampling
* Thread safety guarantees for multi-threaded apps (e.g. gui)
* Extensible base nodes for arbitrary DSP processing
* Microphone input
* SIMD accelerated channel mixing routines
* DSP primitives including filters and delays

# Platforms

LabSound uses RtAudio as its hardware abstraction layer for realtime audio playback on desktop platforms. The repository hosts maintained project files for Visual Studio 2013, Visual Studio 2015, and XCode 7. While not presently supported, LabSound has been shown to run on other platforms including Linux (JACK via RtAudio), iOS (CoreAudio), and Android (OpenSL ES). 

# Building

Users of LabSound are expected to compile LabSound from source. While most dependencies are included as code in the repository, libnyquist is bundled as a git submodule so it is recommended that new users clone the repository with the `--recursive` option. 

Compiling LabSound and its dependencies necessitates a recent C++11 compiler. 

# Examples

LabSound is bundled with approximately 20 single-file samples. Project files can be found in the `examples\` subfolder. A separate repository hosts [more sample applications](https://github.com/LabSound/samples) that require additional dependencies. 

# Using the Library

Users should link against `liblabsound.a` on OSX and `labsound.lib` on Windows. LabSound also requires symbols from libnyquist, although both the Visual Studio solution and the XCode workspace will build this dependency alongside the core library. 

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

LabSound includes an HRTF implementation. This creates an additional dependency on a folder of impulse wav files when a `PannerNode` is configured to use `PanningMode::HRTF`. The constructor of `PannerNode` will take an additional path to the sample directory relative to the current working directory.

# WebAudio Compatibility

The W3C WebAudio specification is listed [at this URL](http://www.w3.org/TR/webaudio/). While LabSound is derived from one of the original WebAudio implementations, it does not aim to maintain full API compatibility with the spec. In many cases, LabSound has deliberately deviated from the spec for performance or usability reasons. This is expected to continue into the future as new functionality is added to the engine. Generally, one might take any WebAudio JavaScript sample code and transliterate it to LabSound with only mild effort (modulo obvious architectual considerations of JavaScript vs C++).

# License 

LabSound is released under the simplified BSD 2 clause license. All LabSound dependencies are under similar permissive licenses. Further details are located in the `LICENSE` and `COPYING` files. 
