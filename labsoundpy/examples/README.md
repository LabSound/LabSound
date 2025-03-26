# LabSoundPy Examples

This directory contains example scripts demonstrating how to use the LabSound Python bindings.

## Running Examples

You can run the examples directly using Python:

```bash
# Run the simple oscillator example
python examples/simple_oscillator.py

# Run the NumPy processing example
python examples/numpy_processing.py
```

## Example Descriptions

### Simple Oscillator (`simple_oscillator.py`)

This example demonstrates the basic usage of LabSoundPy. It creates a sine wave oscillator, connects it to a gain node, and plays it for 2 seconds.

Key concepts demonstrated:
- Creating an AudioContext
- Creating and configuring nodes (OscillatorNode, GainNode)
- Connecting nodes
- Starting and stopping audio playback
- Using the context manager for resource management

### NumPy Processing (`numpy_processing.py`)

This example demonstrates how to use NumPy for audio processing with LabSoundPy. It creates a function node that applies a sine wave modulation to the input signal.

Key concepts demonstrated:
- Creating a FunctionNode for custom audio processing
- Converting audio buffers to NumPy arrays
- Applying NumPy-based signal processing
- Updating audio buffers in-place
- Maintaining state between processing callbacks

## Creating Your Own Examples

When creating your own examples, follow these guidelines:

1. Start with an AudioContext using the context manager (`with` statement)
2. Create and configure the necessary nodes
3. Connect the nodes to form a processing chain
4. Start audio playback
5. Wait for the desired duration
6. Stop audio playback
7. Clean up resources (handled automatically by the context manager)

## Dependencies

The examples require:

- Python 3.7+
- LabSoundPy installed (either in development mode or from a package)
- NumPy (for the NumPy processing example)
