# Python Bindings for LabSound: Technical Design Document

## Overview

This document outlines the technical design for creating Python bindings for LabSound using nanobind. The goal is to provide a Pythonic interface to LabSound's audio processing capabilities, making it as simple and intuitive to use as JavaScript-based Web Audio API while leveraging Python's strengths and ecosystem.

## Core Design Principles

1. **Pythonic Interface**: Create a wrapper that feels natural to Python developers rather than just exposing the C++ API directly.

2. **Graph-Based Simplicity**: Preserve the node-based programming model while taking advantage of Python's syntax.

3. **Context Management**: Use Python's context managers (`with` statements) for resource handling.

4. **Duck Typing**: Leverage Python's dynamic typing to simplify connection interfaces.

5. **Automatic Memory Management**: Use nanobind's memory management features to handle the lifetimes of nodes and connections.

## User Experience Examples

### Basic Audio Generation

```python
import labsound as ls

# Context creation with context manager
with ls.AudioContext() as ctx:
    # Node creation - factory methods on the context
    osc = ctx.create_oscillator()
    gain = ctx.create_gain()
    
    # Pythonic parameter setting
    osc.type = "sine"  # Instead of enum, use strings
    osc.frequency.value = 440
    gain.gain.value = 0.5
    
    # Simplified connections (source -> destination)
    osc >> gain >> ctx.destination
    
    # Scheduling with timeline in seconds
    osc.start(0)
    osc.stop(2)
    
    # Wait for completion (blocking in this example)
    ctx.wait(2.5)
```

### Custom Processing with NumPy Integration

```python
import labsound as ls
import numpy as np

with ls.AudioContext() as ctx:
    # Create a function node for custom processing
    func = ctx.create_function(channels=1)
    
    # Using NumPy for signal processing
    def process_audio(channel, buffer):
        # Convert buffer to numpy array
        samples = np.array(buffer)
        # Apply some processing
        processed = np.sin(samples * np.pi * 2)
        # Update the buffer
        buffer[:] = processed
    
    # Assign the processing function
    func.set_process_function(process_audio)
    
    # Connect to output
    func >> ctx.destination
    
    # Start processing
    func.start()
    
    # Let it run for 5 seconds
    ctx.wait(5)
```

### Audio File Processing

```python
import labsound as ls
import matplotlib.pyplot as plt

with ls.AudioContext() as ctx:
    # Load audio file
    source = ctx.create_buffer_source("path/to/audio.wav")
    
    # Create processing chain
    filter = ctx.create_biquad_filter()
    filter.type = "lowpass"
    filter.frequency.value = 1000
    
    # Create analyzer for visualization
    analyzer = ctx.create_analyzer()
    
    # Connect the chain
    source >> filter >> analyzer >> ctx.destination
    
    # Start playback
    source.start(0)
    
    # Visualize while playing
    while source.is_playing:
        # Get frequency data
        freq_data = analyzer.get_frequency_data()
        
        # Plot using matplotlib
        plt.clf()
        plt.plot(freq_data)
        plt.pause(0.05)
```

## Technical Architecture

### Core Components

#### 1. AudioContext Class

```python
class AudioContext:
    def __init__(self, sample_rate=44100, channels=2, buffer_size=256):
        # Initialize LabSound AudioContext
        
    def __enter__(self):
        # Start context
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        # Stop and clean up context
        
    # Factory methods for all node types
    def create_oscillator(self):
        # Create and return OscillatorNode
        
    def create_gain(self):
        # Create and return GainNode
    
    # ... other node creation methods
    
    def connect(self, source, destination, output=0, input=0):
        # Connect source to destination
        
    def disconnect(self, node):
        # Disconnect node
        
    def current_time(self):
        # Return current audio context time
        
    def wait(self, seconds):
        # Block until the specified time has passed
```

#### 2. Node Base Class

```python
class AudioNode:
    def __init__(self, context, node_ptr):
        # Initialize with reference to context and C++ node
        
    def __rshift__(self, other):
        # Implement the >> operator for connection
        self.context.connect(self, other)
        return other
        
    def __lshift__(self, other):
        # Implement the << operator for reverse connection
        self.context.connect(other, self)
        return other
```

#### 3. Parameter Interface

```python
class AudioParam:
    def __init__(self, param_ptr):
        # Initialize with reference to C++ AudioParam
        
    @property
    def value(self):
        # Get current value
        
    @value.setter
    def value(self, val):
        # Set value
        
    def linear_ramp_to_value_at_time(self, value, end_time):
        # Schedule parameter change
        
    # ... other automation methods
```

#### 4. Specific Node Classes

Each node type will be wrapped with a specific Python class that inherits from AudioNode but adds node-specific methods and properties.

Example for OscillatorNode:

```python
class OscillatorNode(AudioNode):
    def __init__(self, context, node_ptr):
        super().__init__(context, node_ptr)
        
    @property
    def frequency(self):
        # Return AudioParam for frequency
        
    @property
    def detune(self):
        # Return AudioParam for detune
        
    @property
    def type(self):
        # Get oscillator type
        
    @type.setter
    def type(self, type_str):
        # Set oscillator type from string
        
    def start(self, when=0):
        # Start oscillator at specified time
        
    def stop(self, when=0):
        # Stop oscillator at specified time
```

### nanobind Implementation

#### 1. Core Binding Module

```cpp
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

#include "LabSound/LabSound.h"

namespace nb = nanobind;

NB_MODULE(labsound_core, m) {
    // Bind AudioContext
    nb::class_<lab::AudioContext, std::shared_ptr<lab::AudioContext>>(m, "_AudioContext")
        .def(nb::init<>())
        .def("currentTime", &lab::AudioContext::currentTime)
        // ... other methods
        ;
    
    // Bind AudioNode base class
    nb::class_<lab::AudioNode, std::shared_ptr<lab::AudioNode>>(m, "_AudioNode")
        // ... methods
        ;
    
    // Bind specific node types
    nb::class_<lab::OscillatorNode, lab::AudioNode, 
               std::shared_ptr<lab::OscillatorNode>>(m, "_OscillatorNode")
        .def("start", &lab::OscillatorNode::start)
        .def("stop", &lab::OscillatorNode::stop)
        // ... other methods
        ;
    
    // ... bind other node types
    
    // Bind AudioParam
    nb::class_<lab::AudioParam, std::shared_ptr<lab::AudioParam>>(m, "_AudioParam")
        .def("setValue", &lab::AudioParam::setValue)
        .def("linearRampToValueAtTime", &lab::AudioParam::linearRampToValueAtTime)
        // ... other methods
        ;
}
```

#### 2. Python Layer

The Python layer builds on top of the core bindings to provide the Pythonic interface described above.

#### 3. NumPy Integration

```cpp
// In the C++ bindings
#include <nanobind/ndarray.h>

// Function to convert between audio buffer and numpy array
m.def("buffer_to_ndarray", [](std::shared_ptr<lab::AudioBus> bus, int channel) {
    float* data = bus->channel(channel)->mutableData();
    size_t size = bus->length();
    
    // Create a numpy array that references the data
    return nb::ndarray<nb::numpy, float, nb::shape<nb::any>>(
        data, {size}, {sizeof(float)}
    );
});
```

## Memory Management Strategy

1. **Shared Ownership**: Use `std::shared_ptr` on the C++ side and let nanobind manage the Python reference count.

2. **Context Lifetime**: Ensure the AudioContext outlives any nodes created from it.

3. **Reference Cycles**: Carefully manage potential reference cycles between nodes and parameters.

4. **GIL Management**: Release the Python GIL during audio processing to avoid blocking the audio thread.

## Error Handling

1. **Exception Translation**: Convert C++ exceptions to appropriate Python exceptions.

2. **Audio Thread Safety**: Protect against exceptions in audio callback functions.

3. **Parameter Validation**: Validate parameters on the Python side before passing to C++.

## Integration with Python Ecosystem

### NumPy/SciPy Integration

- Direct conversion between audio buffers and NumPy arrays
- Use SciPy's signal processing functions with LabSound data

### Matplotlib/Visualization

- Helper functions for visualizing waveforms and spectra
- Real-time plotting of audio analysis

### Jupyter Notebook Support

- Interactive widgets for controlling audio parameters
- Audio playback in notebooks
- Visualization of audio processing chains

### Machine Learning Integration

- Interface with TensorFlow/PyTorch for audio ML models
- Support for real-time ML-based audio processing

## Performance Considerations

1. **GIL Impacts**: Measure and document the impact of Python's Global Interpreter Lock on audio performance.

2. **Buffer Management**: Optimize buffer copying between C++ and Python.

3. **Callback Overhead**: Minimize overhead for Python callbacks in the audio thread.

4. **Profiling**: Provide tools to profile Python audio code.

## Testing Strategy

1. **Unit Tests**: For all Python wrapper classes and methods

2. **Integration Tests**: For complete audio processing chains

3. **Performance Tests**: To ensure real-time audio performance is maintained

4. **Example Validation**: Ensure all documentation examples work as expected

## Documentation Plan

1. **API Reference**: Comprehensive documentation for all Python classes and methods

2. **Tutorials**: Step-by-step guides for common tasks

3. **Examples**: A collection of example scripts demonstrating various capabilities

4. **Migration Guide**: For Web Audio API developers moving to Python

## Implementation Roadmap

1. **Phase 1**: Core AudioContext and basic nodes (Oscillator, Gain, etc.)

2. **Phase 2**: Complete standard node set and parameter automation

3. **Phase 3**: NumPy integration and custom processing

4. **Phase 4**: Advanced nodes and ecosystem integration

5. **Phase 5**: Optimization, documentation, and distribution

## Conclusion

This Python binding for LabSound has the potential to bring the power of LabSound's audio processing capabilities to a wider audience of Python developers. By focusing on a Pythonic interface and integration with the Python ecosystem, we can create a powerful tool for audio research, production, and education.

The combination of LabSound's performance with Python's ease of use and rich ecosystem could make this an especially valuable tool for rapid prototyping of audio applications, audio machine learning research, and interactive audio education.
