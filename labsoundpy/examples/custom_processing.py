"""
Custom Audio Processing Example

This example demonstrates the use of the FunctionNode to create custom audio processing effects.
It creates a simple sine wave and applies various custom processing functions to it.
"""

import time
import math
import numpy as np
from labsound import AudioContext

# Create an audio context
with AudioContext() as context:
    # Create an oscillator as our sound source
    oscillator = context.create_oscillator()
    oscillator.frequency.value = 220  # A3 note
    oscillator.type = "sine"
    
    # Create a function node for custom processing
    # We'll use 1 channel for simplicity
    function_node = context.create_function(channels=1)
    
    # Create a gain node to control the output volume
    gain = context.create_gain()
    gain.gain.value = 0.5  # Lower the volume to avoid clipping
    
    # Connect the nodes
    context.connect(oscillator, function_node)
    context.connect(function_node, gain)
    context.connect(gain, context.destination)
    
    # Start the oscillator
    oscillator.start()
    
    print("Demonstrating different custom processing effects...")
    
    # 1. Pass-through (no processing)
    def pass_through(channel, buffer):
        # Do nothing, just pass the audio through unchanged
        pass
    
    function_node.set_process_function(pass_through)
    print("No processing (pass-through)")
    time.sleep(3)
    
    # 2. Tremolo effect (amplitude modulation)
    def tremolo(channel, buffer):
        # Apply a tremolo effect (amplitude modulation)
        # This varies the volume of the audio at a regular rate
        rate = 5  # Hz
        depth = 0.5  # 0-1, how deep the effect is
        
        for i in range(len(buffer)):
            # Calculate the modulation factor
            mod = 1.0 - depth * (0.5 + 0.5 * math.sin(2 * math.pi * rate * i / 44100))
            # Apply the modulation
            buffer[i] *= mod
    
    function_node.set_process_function(tremolo)
    print("Tremolo effect")
    time.sleep(3)
    
    # 3. Bit crusher effect (reduces bit depth)
    def bit_crusher(channel, buffer):
        # Apply a bit crusher effect (reduces bit depth)
        # This creates a lo-fi, retro sound
        bits = 4  # Reduced bit depth (lower = more distortion)
        
        # Calculate the step size based on bit depth
        step = 2.0 ** (bits - 1)
        
        for i in range(len(buffer)):
            # Quantize the sample to the reduced bit depth
            buffer[i] = math.floor(buffer[i] * step) / step
    
    function_node.set_process_function(bit_crusher)
    print("Bit crusher effect")
    time.sleep(3)
    
    # 4. Ring modulation
    def ring_modulation(channel, buffer):
        # Apply ring modulation
        # This multiplies the audio by a sine wave, creating metallic sounds
        freq = 600  # Modulation frequency in Hz
        
        for i in range(len(buffer)):
            # Calculate the modulation factor
            mod = math.sin(2 * math.pi * freq * i / 44100)
            # Apply the modulation
            buffer[i] *= mod
    
    function_node.set_process_function(ring_modulation)
    print("Ring modulation effect")
    time.sleep(3)
    
    # 5. Noise gate
    def noise_gate(channel, buffer):
        # Apply a noise gate
        # This silences audio below a threshold
        threshold = 0.1
        
        for i in range(len(buffer)):
            # If the sample is below the threshold, silence it
            if abs(buffer[i]) < threshold:
                buffer[i] = 0
    
    function_node.set_process_function(noise_gate)
    print("Noise gate effect")
    time.sleep(3)
    
    # 6. Reverse buffer
    def reverse_buffer(channel, buffer):
        # Reverse the audio buffer
        # This creates a backwards effect
        buffer_copy = buffer.copy()
        for i in range(len(buffer)):
            buffer[i] = buffer_copy[len(buffer) - 1 - i]
    
    function_node.set_process_function(reverse_buffer)
    print("Reverse buffer effect")
    time.sleep(3)
    
    # Stop the oscillator
    oscillator.stop()
    
    # Wait a moment for the sound to finish
    time.sleep(0.5)

print("Done!")
