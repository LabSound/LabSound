# LabSoundPy

Python bindings for the LabSound audio engine.

## Overview

LabSoundPy provides a Pythonic interface to LabSound's audio processing capabilities, making it as simple and intuitive to use as JavaScript-based Web Audio API while leveraging Python's strengths and ecosystem.

## Features

- **Pythonic Interface**: A wrapper that feels natural to Python developers
- **Graph-Based Audio Processing**: Node-based programming model for audio processing
- **NumPy Integration**: Seamless integration with NumPy for audio data manipulation
- **Comprehensive Node Types**: Support for all standard Web Audio API node types
- **Custom Processing**: Create custom audio processing with Python functions
- **Context Management**: Use Python's context managers for resource handling

## Installation

```bash
# Coming soon
pip install labsoundpy
```

## Quick Start

```python
import labsound as ls

# Create an audio context with context manager
with ls.AudioContext() as ctx:
    # Create nodes
    osc = ctx.create_oscillator()
    gain = ctx.create_gain()
    
    # Set parameters
    osc.frequency.value = 440  # A4 note
    gain.gain.value = 0.5      # Half volume
    
    # Connect nodes
    ctx.connect(osc, gain)
    ctx.connect(gain, ctx.destination)
    
    # Start and stop the oscillator
    osc.start(0)
    osc.stop(2)
    
    # Wait for completion
    ctx.wait(2.5)
```

## Node Types

LabSoundPy supports the following node types:

- **AnalyzerNode**: For real-time frequency and time-domain analysis
- **AudioBufferSourceNode**: For playing back pre-recorded audio
- **BiquadFilterNode**: For filtering audio with various filter types
- **ChannelMergerNode**: For combining multiple audio channels
- **ChannelSplitterNode**: For splitting audio into multiple channels
- **ConstantSourceNode**: For generating a constant value
- **ConvolverNode**: For applying convolution effects (e.g., reverb)
- **DelayNode**: For delaying audio
- **DynamicsCompressorNode**: For dynamic range compression
- **FunctionNode**: For custom audio processing with Python functions
- **GainNode**: For controlling audio volume
- **OscillatorNode**: For generating audio tones
- **PannerNode**: For 3D audio positioning
- **StereoPannerNode**: For simple stereo panning
- **WaveShaperNode**: For non-linear distortion effects

## Examples

### Spatial Audio

```python
import time
import math
from labsound import AudioContext

# Create an audio context
with AudioContext() as context:
    # Create an oscillator as our sound source
    oscillator = context.create_oscillator()
    oscillator.frequency.value = 440  # A4 note
    
    # Create a panner node for 3D positioning
    panner = context.create_panner()
    panner.panning_model = "HRTF"  # Head-related transfer function
    
    # Connect the oscillator to the panner, and the panner to the destination
    context.connect(oscillator, panner)
    context.connect(panner, context.destination)
    
    # Start the oscillator
    oscillator.start()
    
    # Move the sound source in a circle around the listener
    radius = 5  # 5 units away from the listener
    for i in range(100):
        angle = (i * 0.1) % (2 * math.pi)
        x = radius * math.sin(angle)
        z = radius * math.cos(angle)
        
        # Update the panner position
        panner.set_position(x, 0, z)
        
        # Wait a bit before the next update
        time.sleep(0.1)
    
    # Stop the oscillator
    oscillator.stop()
```

### Custom Processing with NumPy

```python
import numpy as np
from labsound import AudioContext

# Create an audio context
with AudioContext() as context:
    # Create an oscillator as our sound source
    oscillator = context.create_oscillator()
    oscillator.frequency.value = 220  # A3 note
    
    # Create a function node for custom processing
    function_node = context.create_function(channels=1)
    
    # Define a custom processing function
    def bit_crusher(channel, buffer):
        # Apply a bit crusher effect (reduces bit depth)
        bits = 4  # Reduced bit depth
        step = 2.0 ** (bits - 1)
        
        for i in range(len(buffer)):
            # Quantize the sample to the reduced bit depth
            buffer[i] = np.floor(buffer[i] * step) / step
    
    # Set the processing function
    function_node.set_process_function(bit_crusher)
    
    # Connect the nodes
    context.connect(oscillator, function_node)
    context.connect(function_node, context.destination)
    
    # Start the oscillator
    oscillator.start()
    
    # Let it play for 3 seconds
    context.wait(3)
```

## Documentation

For more detailed documentation, see the [API Reference](docs/api_reference.md) and [Examples](examples/).

## Development

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/labsoundpy.git
cd labsoundpy

# Install development dependencies
pip install -e ".[dev]"

# Build the extension
python setup.py build_ext --inplace
```

### Running Tests

```bash
pytest
```

## License

LabSoundPy is licensed under the same license as LabSound. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

LabSoundPy is built on top of [LabSound](https://github.com/LabSound/LabSound), a C++ audio engine.
