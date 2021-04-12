

## v1.0.1
# 11 April 2021

- New platforms: Linux, Raspberry Pi, iOS, Android, in addition to macOS and Windows.
- various fixes to regressions in SampledAudioNode
- add accessors to expose node parameters for integration into DAWs
- various platform fixes for iOS and Android
- Github Actions now managing the CI

## v1.0
# 15 Jan 2021

This 1.0 release finalizes the API with functionality that allows full introspection of all nodes and attributes. To go with this 1.0, LabSoundGraphToy is also released at https://github.com/LabSound/LabSoundGraphToy to enable exploration of the library, its nodes, and its functionality.


- Much thread safety introduced
- ADSRNode now has a gate param that can be used to schedule note on and off
- Profiling system added
- Node scheduling reworked
- Audio popping mitigated
- RecorderNode defers mix to down to save
- RecorderNode create bus from recording
- SfxrNode exposes presets as settings
- SampledAudioNode has scheduled voices
- AudioNodes all take an AudioContext& during construction
- Offline rendering works consistently with online rendering, including automatic pull nodes
- Automatic pull nodes work reliably
- Updated use of data types to conform to LabSound conventions
- New GranulationNode
- fixed almost all of the compiler warnings
- removed bang in favor of an onStart callback that allows rescheduling in the processing call


## v0.14.0 Release Candidate
# 15 Mar 2020

- Removed many custom backends in favor of RtAudio and miniaudio
- Consolidated demos into a single header
- clang format applied to sources
- cloning method added to AudioBus
- Added Input and Output configuration structs to allow selection of devices
- names added for params and settings for use in tools
- Bus added as available Setting type
- ADSRNode timing fixed
- AnalyserNode can render output to any desired output buffer size by binning and interpolation
- AudioDevice nodes added in major backend refactoring
- ConvolverNode - new implementation
- DiodeNode is now derived from AudioNode
- OscillatorNode - new implementation
- SuperSaw is now scheduled

### Releases
------------

## v0.13.0 Release Candidate
# 18 Aug 2019

- AudioSettings. AudioParams already exist for rate varying values on Audio Nodes. AudioParams are registered on nodes by name, and can be looked up by name. Non-rate varying parameters such as filter modes, buffer sizes, and so on could not be similarly looked up by name. AudioSettings are introduced in this release for non-rate varying parameters, such as filter modes, buffer sizes and so on. Settings for each AudioNode are documented in its corresponding header file. A vector of param or setting names can be fetched from an AudioNode and used to populate a user interface. Each fetched AudioSetting can be queried as to whether its last set type was a Uint32 or a Float, in order that user interfaces can self configure appropriately.

- Added some nullchecks to AudioChannel memory management as pointed out by harmonicvision

## v0.12.0 Beta
# 4 Jan 2018

- clean up how AudioNodes are connected to one another. Formerly, the connect(...) method was called directly on an AudioNode, taking the graph lock and effecting changes to the AudioContext itself. This is no longer the case. AudioContext is now the arbiter of all graph changes.

## v0.11.0 Alpha
# 4 Jan 2018

- Numerous bug and maintenance fixes.
- A cmake build system.

## v0.10.0 Alpha
# 2 Jan 2016

- Several breaking API changes related to AudioContext creation and PannerNode initialization.

## v0.9.0 Alpha
# 2 Jan 2016

- Maintenance vs2015
