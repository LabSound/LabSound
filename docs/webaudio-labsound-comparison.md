# WebAudio API vs LabSound API Comparison

## Introduction

This document provides a comprehensive comparison between the Web Audio API and the LabSound library, which is a C++ implementation inspired by the Web Audio API concepts with numerous extensions. Understanding the similarities and differences between these two audio frameworks will help developers transition between web and native audio development.

### Key Insights

- **Architectural Approach**: LabSound follows WebAudio's node-based architecture but implements it in a modern C++ framework with smart pointer management.
- **Extended Functionality**: LabSound includes numerous nodes beyond the standard WebAudio specification, especially for audio synthesis, analysis, and advanced effects.
- **Parameter Model**: LabSound enhances the parameter system with both `AudioParam` (automatable) and `AudioSetting` (non-automatable) parameters.
- **Connection API**: LabSound uses a context-driven connection model, contrasting with WebAudio's node-driven approach, providing more explicit control over the audio graph.
- **Audio Scheduling**: While both APIs support sample-accurate scheduling, LabSound provides more detailed control over timing and playback.
- **Native Performance**: As a native library, LabSound can offer performance advantages over browser-based WebAudio implementations.

## Core Audio Model

### AudioContext

**WebAudio**: Central object that manages and plays all sound.
**LabSound**: Implemented with similar functionality in `AudioContext` class.

Key differences:
- LabSound exposes explicit methods for connection management like `connectNode()`, `disconnectNode()`, `connectParam()`, and `disconnectParam()`
- Includes more detailed lifecycle management with `suspend()`, `resume()`, and `close()`
- Additional debugging capabilities with methods like `debugTraverse()` and `diagnose()`

### AudioNode

**WebAudio**: Base class for all audio processing modules.
**LabSound**: Implemented as `AudioNode` class with similar functionality.

Key differences:
- Object-oriented C++ implementation with more explicit memory management
- Additional methods for controlling channel count behavior
- More detailed profiling capabilities with methods like `graphTime()` and `totalTime()`
- Nodes are often constructed differently (via the AudioContext)

### Parameter Model

**WebAudio**: Uses `AudioParam` for automatable parameters.
**LabSound**: Uses both:
- `AudioParam` for automatable parameters
- `AudioSetting` for non-automatable parameters (unique to LabSound)

```cpp
// WebAudio
oscillator.frequency.value = 440;
oscillator.frequency.linearRampToValueAtTime(880, audioContext.currentTime + 1);

// LabSound
oscillator->frequency()->setValue(440.0f);
oscillator->frequency()->linearRampToValueAtTime(880.0f, context->currentTime() + 1.0f);

// LabSound setting (non-automatable)
analyser->setFftSize(2048);
```

## Standard Nodes (Available in Both WebAudio and LabSound)

### AnalyserNode

**WebAudio**: Provides real-time frequency and time-domain analysis.
**LabSound**: Implemented with similar functionality.

Parameters/Settings:
- `fftSize`: WebAudio property, LabSound method `setFftSize()`
- `frequencyBinCount`: Both have getter method
- `minDecibels`/`maxDecibels`: Both have getters/setters
- `smoothingTimeConstant`: Both have getter/setter

Methods:
- `getFloatFrequencyData()`/`getByteFrequencyData()`: Both have
- `getFloatTimeDomainData()`/`getByteTimeDomainData()`: Both have

LabSound differences:
- `getByteFrequencyData()` has an optional resampling parameter
- Constructor can take an fftSize parameter

### BiquadFilterNode

**WebAudio**: Implements different types of filters.
**LabSound**: Implemented with similar functionality.

Parameters:
- `type`: Both support filter type setting
- `frequency`: Both as AudioParam
- `detune`: Both as AudioParam
- `Q`: Both as AudioParam (named `q()` in LabSound)
- `gain`: Both as AudioParam

Methods:
- `getFrequencyResponse()`: Both have similar method

### ChannelMergerNode

**WebAudio**: Combines individual audio channels into a multi-channel stream.
**LabSound**: Implemented similarly.

Differences:
- LabSound adds `addInputs()` to dynamically add more inputs
- LabSound adds `setOutputChannelCount()` to explicitly set output channel count

### ChannelSplitterNode

**WebAudio**: Separates a multi-channel audio stream into individual channels.
**LabSound**: Implemented similarly.

Differences:
- LabSound adds `addOutputs()` to dynamically add more outputs

### ConstantSourceNode

**WebAudio**: Outputs a constant value.
**LabSound**: Implemented with similar functionality.

Parameters:
- `offset`: Both as AudioParam

### ConvolverNode

**WebAudio**: Applies a linear convolution effect for creating reverb.
**LabSound**: Implemented as a scheduled source node.

Parameters/Settings:
- `normalize`: WebAudio property, LabSound has a getter/setter
- `buffer`/`impulse`: WebAudio uses `buffer`, LabSound uses `setImpulse()`/`getImpulse()`

Differences:
- LabSound implements ConvolverNode as a subclass of `AudioScheduledSourceNode`
- LabSound allows scheduling convolution to start at specific times

### DelayNode

**WebAudio**: Delays the audio signal.
**LabSound**: Implemented with similar functionality.

Parameters:
- `delayTime`: Both as AudioParam

Differences:
- LabSound constructor accepts a maximum delay time (default: 2.0s)
- LabSound exposes `delayTime()` as an AudioSetting rather than an AudioParam

### DynamicsCompressorNode

**WebAudio**: Audio compression effect.
**LabSound**: Implemented with similar functionality.

Parameters:
- `threshold`, `knee`, `ratio`, `attack`, `release`: Both as AudioParams
- `reduction`: WebAudio has getter only, LabSound exposes as AudioParam

### GainNode

**WebAudio**: Controls volume.
**LabSound**: Implemented with similar functionality.

Parameters:
- `gain`: Both as AudioParam

### OscillatorNode

**WebAudio**: Generates periodic waveforms.
**LabSound**: Implemented with similar functionality but with extensions.

Parameters:
- `type`: Both support waveform type selection
- `frequency`: Both as AudioParam
- `detune`: Both as AudioParam

LabSound additions:
- `amplitude`: Controls output level (not in WebAudio)
- `bias`: Controls DC offset (not in WebAudio)

### PannerNode

**WebAudio**: Spatializes audio in 3D space.
**LabSound**: Implemented with similar functionality.

Parameters:
- All standard 3D audio positioning parameters are present in both
- Both expose position, orientation, velocity as X/Y/Z components
- Both support distance models, cone effects, etc.

LabSound additions:
- Exposes `distanceGain` and `coneGain` as additional AudioParams

### StereoPannerNode

**WebAudio**: Simple stereo panning.
**LabSound**: Implemented with similar functionality.

Parameters:
- `pan`: Both as AudioParam

### WaveShaperNode

**WebAudio**: Applies non-linear waveshaping distortion.
**LabSound**: Implemented with similar functionality.

Parameters/Settings:
- `curve`: Both (WebAudio property, LabSound method `setCurve()`)
- `oversample`: Both (enum values for different oversampling levels)

### AudioBufferSourceNode vs SampledAudioNode

**WebAudio**: `AudioBufferSourceNode` for playing back audio buffers.
**LabSound**: Implemented as `SampledAudioNode` with different API.

Parameters:
- `playbackRate`: Both as AudioParam
- `detune`: Both as AudioParam
- `loop`, `loopStart`, `loopEnd`: WebAudio properties
- LabSound uses a different scheduling model with loop count

LabSound differences:
- More detailed scheduling API with multiple `schedule()` and `start()` methods
- `getCursor()` method for tracking playback position
- `dopplerRate` parameter for Doppler effect adjustments
- No direct equivalent to WebAudio's loop properties; uses loop count in scheduling

## WebAudio Nodes Missing from LabSound

These WebAudio nodes don't have direct equivalents in LabSound:

1. **AudioWorkletNode** - This is a web-specific concept for custom audio processing. The equivalent LabSound node is the `FunctionNode`. Developers needing AudioWorkletNode-like functionality should base it on the FunctionNode.

2. **IIRFilterNode** - A relatively recent addition to WebAudio for creating custom filter responses. This could be a candidate for future LabSound implementation. Currently, custom filtering needs can be addressed with BiquadFilterNode or custom processing.

3. **MediaElementAudioSourceNode** - Web-specific node for connecting HTML media elements to the audio graph. In LabSound, similar functionality can be achieved using the `SampledAudioNode` with appropriate audio data loading.

4. **MediaStreamAudioSourceNode/MediaStreamAudioDestinationNode** - Web-specific nodes for working with the MediaStream API. These would need to be emulated in LabSound with custom implementations based on the target platform's audio capture and playback capabilities.

5. **PeriodicWave** - Used in WebAudio for creating custom oscillator waveforms. LabSound may have equivalent functionality, but it's not explicitly represented in the current API. This could be a candidate for future implementation.

6. **ScriptProcessorNode** - Deprecated in WebAudio and replaced by AudioWorkletNode. LabSound's `FunctionNode` provides similar capabilities with better performance.

## LabSound-Specific Nodes (Extensions)

These nodes are unique to LabSound and not part of the standard WebAudio API:

### ADSRNode

Envelope generator with Attack, Decay, Sustain, and Release phases.

Parameters/Settings:
- `gate()`: Control signal for triggering the envelope
- `oneShot()`: Boolean for one-shot vs. sustained mode
- Time/level parameters for each phase of the envelope

### BPMDelayNode

A delay node that synchronizes with musical tempo.

Methods:
- `SetTempo()`: Sets the tempo in BPM
- `SetDelayIndex()`: Sets delay time based on musical note values (e.g., quarter note, eighth note)

### ClipNode

Audio clipper/limiter with different clipping modes.

Parameters:
- `aVal()`: Min value in CLIP mode, gain in TANH mode
- `bVal()`: Max value in CLIP mode, input gain in TANH mode
- Supports both hard clipping and tanh-based soft clipping

### DiodeNode

Models vacuum tube diode distortion.

Settings:
- `distortion()`: Controls the amount of distortion
- `vb()` and `vl()`: Shape controls for the distortion curve

### FunctionNode

Programmable node that allows custom audio processing functions.

Methods:
- `setFunction()`: Sets a callback function that generates or processes audio

### GranulationNode

Granular synthesis processor.

Parameters/Settings:
- `grainSourceBus`: Audio source for granulation
- `windowFunc`: Windowing function for grains
- `numGrains`: Number of simultaneous grains
- Various parameters for grain duration, position, and playback frequency

### NoiseNode

Generates different types of noise.

Settings:
- `type()`: Selects noise type (WHITE, PINK, BROWN)

### PeakCompNode

Stereo peak compressor for dynamic range control.

Parameters:
- `threshold()`, `ratio()`, `attack()`, `release()`, `makeup()`, `knee()`

### PingPongDelayNode

Stereo delay effect that bounces between left and right channels.

Methods:
- `SetTempo()`, `SetFeedback()`, `SetLevel()`, `SetDelayIndex()`

### PolyBLEPNode

High-quality band-limited oscillator with anti-aliasing.

Parameters:
- `type()`: Various waveform types including antialiased versions
- `amplitude()` and `frequency()`

### PowerMonitorNode

Monitors audio power levels.

Methods:
- `db()`: Returns the current power level in decibels
- `windowSize()`: Controls the analysis window size

### PWMNode

Pulse Width Modulation node.

### RecorderNode

Records audio signals to memory or disk.

Methods:
- `startRecording()`, `stopRecording()`
- `recordedLengthInSeconds()`
- `createBusFromRecording()`, `writeRecordingToWav()`

### SfxrNode

Sound effect generator based on the popular SFXR synthesizer.

Parameters:
- Numerous parameters for controlling the generated sound
- Preset functions like `coin()`, `laser()`, `explosion()`, etc.

### SpatializationNode

Extended 3D audio spatialization with occlusion.

Features:
- Support for occlusion objects in the 3D environment
- Methods to add and manage occluders

### SpectralMonitorNode

Provides detailed spectral analysis.

Methods:
- `spectralMag()`: Returns the magnitude spectrum
- `windowSize()`: Controls the analysis window size

### SupersawNode

Supersaw oscillator for electronic music sounds.

Parameters:
- `sawCount()`: Number of detuned sawtooth oscillators
- `frequency()` and `detune()`

## Connection Models

Both APIs use connection models for creating audio processing chains, but with different syntax:

```cpp
// WebAudio
sourceNode.connect(destinationNode);

// LabSound
context->connect(destinationNode, sourceNode);
// or
context->connectNode(sourceNode, destinationNode);
```

LabSound adds:
- Direct parameter connection: `context->connectParam(param, driver)`
- More detailed disconnection methods
- Synchronization: `context->synchronizeConnections()`

## Memory Management

LabSound uses modern C++ memory management:
- Uses `std::shared_ptr` for most objects
- Audio nodes are often created using factories or constructors that take the AudioContext
- Smart pointer semantics for automatic resource management

## Audio Processing Model

Both use a similar audio processing graph model:
- Real-time audio processing with a sample-accurate scheduler
- Both support multichannel audio
- Processing happens in discrete blocks/buffers
- LabSound adds more detailed profiling and debugging capabilities

## Cross-Platform Considerations

When porting web audio applications to native using LabSound:

1. **Audio Resource Loading**: Replace web-specific resource loading with platform-appropriate file I/O
2. **User Interaction**: Map web event handling to native UI frameworks
3. **Audio Capture**: Replace MediaStream concepts with platform-specific audio capture
4. **Threading Model**: Be aware of threading differences between browser JS and native C++
5. **Memory Management**: Take advantage of C++ memory management for better performance

## Performance Optimization

LabSound offers several advantages for performance-critical applications:

1. **Native Execution**: Avoids browser sandboxing overhead
2. **Fine Control**: More detailed control over buffer sizes and processing
3. **Extended Nodes**: Specialized nodes can handle common tasks more efficiently
4. **Custom Processing**: FunctionNode allows highly optimized C++ processing functions
5. **Memory Efficiency**: Smart pointer management reduces memory overhead

## Conclusion

LabSound provides a familiar programming model for developers accustomed to the WebAudio API while offering the performance and flexibility advantages of a native C++ library. By understanding the mapping between these two APIs, developers can effectively leverage their Web Audio knowledge in native application development.
