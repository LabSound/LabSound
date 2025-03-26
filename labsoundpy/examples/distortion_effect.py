"""
Distortion Effect Example

This example demonstrates the use of the WaveShaperNode to create a distortion effect.
It creates a simple sine wave and applies different distortion curves to it.
"""

import time
import math
import numpy as np
from labsound import AudioContext

def create_distortion_curve(amount=50):
    """
    Create a distortion curve for the WaveShaper node.
    
    Args:
        amount: The amount of distortion (higher = more distortion)
        
    Returns:
        A numpy array containing the distortion curve
    """
    # Create a curve with 'amount' of distortion
    # The curve maps input values (-1 to 1) to output values (-1 to 1)
    k = amount
    n_samples = 44100
    curve = np.array([((3 + k) * x / 4) * (1 - np.abs(x) ** (k / 3)) / (1 + k * np.abs(x)) 
                      for x in np.linspace(-1, 1, n_samples)], dtype=np.float32)
    return curve

# Create an audio context
with AudioContext() as context:
    # Create an oscillator as our sound source
    oscillator = context.create_oscillator()
    oscillator.frequency.value = 220  # A3 note
    oscillator.type = "sine"
    
    # Create a wave shaper node for distortion
    wave_shaper = context.create_wave_shaper()
    
    # Create a gain node to control the output volume
    gain = context.create_gain()
    gain.gain.value = 0.3  # Lower the volume to avoid clipping
    
    # Connect the nodes
    context.connect(oscillator, wave_shaper)
    context.connect(wave_shaper, gain)
    context.connect(gain, context.destination)
    
    # Start the oscillator
    oscillator.start()
    
    print("Demonstrating different distortion amounts...")
    
    # No distortion (linear curve)
    wave_shaper.curve = np.linspace(-1, 1, 44100).astype(np.float32)
    print("No distortion")
    time.sleep(2)
    
    # Mild distortion
    wave_shaper.curve = create_distortion_curve(5)
    print("Mild distortion")
    time.sleep(2)
    
    # Medium distortion
    wave_shaper.curve = create_distortion_curve(25)
    print("Medium distortion")
    time.sleep(2)
    
    # Heavy distortion
    wave_shaper.curve = create_distortion_curve(100)
    print("Heavy distortion")
    time.sleep(2)
    
    # Extreme distortion
    wave_shaper.curve = create_distortion_curve(1000)
    print("Extreme distortion")
    time.sleep(2)
    
    # Demonstrate different oversample settings
    print("\nDemonstrating different oversample settings...")
    
    # No oversampling
    wave_shaper.oversample = "none"
    print("Oversample: none")
    time.sleep(2)
    
    # 2x oversampling
    wave_shaper.oversample = "2x"
    print("Oversample: 2x")
    time.sleep(2)
    
    # 4x oversampling
    wave_shaper.oversample = "4x"
    print("Oversample: 4x")
    time.sleep(2)
    
    # Stop the oscillator
    oscillator.stop()
    
    # Wait a moment for the sound to finish
    time.sleep(0.5)

print("Done!")
