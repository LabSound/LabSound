"""
Spatial Audio Example

This example demonstrates the use of the PannerNode to create a 3D audio experience.
It creates a sound source that moves in a circle around the listener.
"""

import time
import math
import numpy as np
from labsound import AudioContext

# Create an audio context
with AudioContext() as context:
    # Create an oscillator as our sound source
    oscillator = context.create_oscillator()
    oscillator.frequency.value = 440  # A4 note
    oscillator.type = "sine"
    
    # Create a panner node for 3D positioning
    panner = context.create_panner()
    
    # Set panner properties
    panner.panning_model = "HRTF"  # Head-related transfer function for realistic 3D audio
    panner.distance_model = "inverse"
    panner.ref_distance = 1
    panner.max_distance = 10000
    panner.rolloff_factor = 1
    
    # Connect the oscillator to the panner, and the panner to the destination
    context.connect(oscillator, panner)
    context.connect(panner, context.destination)
    
    # Start the oscillator
    oscillator.start()
    
    print("Moving sound source in a circle around the listener...")
    print("Press Ctrl+C to stop")
    
    try:
        # Move the sound source in a circle around the listener
        radius = 5  # 5 units away from the listener
        for i in range(1000):  # Run for a while
            # Calculate position on the circle
            angle = (i * 0.05) % (2 * math.pi)
            x = radius * math.sin(angle)
            z = radius * math.cos(angle)
            y = 0  # Keep at the same height as the listener
            
            # Update the panner position
            panner.set_position(x, y, z)
            
            # Print the current position (only occasionally to avoid console spam)
            if i % 10 == 0:
                print(f"Sound source position: ({x:.2f}, {y:.2f}, {z:.2f})")
            
            # Wait a bit before the next update
            time.sleep(0.05)
    
    except KeyboardInterrupt:
        print("\nStopping...")
    
    finally:
        # Stop the oscillator
        oscillator.stop()
        
        # Wait a moment for the sound to finish
        time.sleep(0.5)

print("Done!")
