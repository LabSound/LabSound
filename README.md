# LabSound

LabSound is the WebAudio implementation factored out of WebKit. All copyleft sources have been removed
so that the entire library can be released under MIT/BSD style licenses.

The motivation behind LabSound is that the [WebAudio specification](https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html) is a well-designed API for an audio engine. Coupled with a well executed implementation provided by WebKit contributors, LabSound is a powerful and extensible audio engine for C++. 

LabSound’s internals were disconnected from Javascript and the browser infrastructure. The cross-platform nature of WebKit means that LabSound enjoys wide platform support across OSX 10.6+, Windows 7+, and although presently untested, Linux and iOS. 

One of LabSound’s goals is to provide higher-level DSP and signal generation nodes for use with the engine. To this end, integrations with PureData (PD), SFXR, and the STK are either complete or in progress.

The keyword LabSound marks any places the code was modified from the original WebKit source. The engine has been slowly integrating newer C++11 features while trying to remain as compiler-portable as possible. All exposed interfaces have been renamed into the LabSound namespace. 

## Building

Currently, this repository does not host precompiled binaries. Premake4 has been used previously generate platform-specific project files, although both the supplied XCode Workspace and Visual Studio Solution file are kept up-to-date. Please see the section below for your target platform. 

## Usage

It is only necessary to have the `includes/` directory in your path. For convenience, `LabSound.h` additionally includes all WebAudio and LabSound nodes for quick and easy application development. Don't forget to link against the built LabSound library. 

On OSX, user projects will also need the following frameworks:
+   Cocoa
+   Accelerate
+   CoreAudio
+   AudioUnit
+   AudioToolbox

On Windows, user projects will also need the following dependencies: 
+   dsound.lib
+   dxguid.lib
+   winmm.lib

LabSound includes WebAudio HRTF implementation. This creates an additional dependency on a folder of HRTF impulse wav files. These files should be located in a directory called `resources/` in the current working directory of the application. 

## Building with XCode on OSX

Premake4 was used to generate the platform-specific project files. On OSX, the both the LabSound project and Examples workspace should build from a clean checkout. 

## Building with Sublime on OSX

To do a build in Sublime using xctool, xctool must be in the path. If it isn't in the 
launchctl path, one way to add it is in Sublime's Python console, eg -

os.environ["PATH"] = os.environ["PATH"]+":/Users/dp/local/bin"

xcodebuild works as well, but xctool is much more user friendly.


## Building with Visual Studio 2013 on Windows 

Windows requires number of additional dependencies that are included, under their respective license terms, in this repository. These include RTAudio, KissFFT, and libsndfile. Of these, RTAudio and KissFFT have nonrestrictive BSD-compatible licenses, although libsndfile is LGPL. The functionality of KissFFT is cross platform, although the Windows ifft implementation produces noisy output when generating the wavetables for the standard oscillators. 


## Examples + Wiki

LabSound comes with a full compliment of examples to get started. Generally, you may take any WebAudio JavaScript sample code and transliterate it exactly into C++. In some cases, there are extra parameters to specify such as the input and output indices for many types of nodes. 

The Github Wiki has additional examples and information. 

## License 

Any bits not part of the WebKit code (such as the files in the shim) directory can
be assumed to be under a BSD 3-clause license. <http://opensource.org/licenses/BSD-3-Clause>

The license on the WebKit files is the Google/Apple BSD 2 clause modified license on
most of the Webkit sources. Note that the WTF library in particular is intended to be
used outside of WebKit. cf <http://lists.macosforge.org/pipermail/webkit-dev/2007-November/002729.html>
