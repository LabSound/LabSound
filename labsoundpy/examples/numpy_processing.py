"""
NumPy integration example using LabSound Python bindings.

This example demonstrates how to use NumPy for audio processing with LabSound.
It creates a function node that applies a sine wave modulation to the input signal.
"""

import labsound as ls
import numpy as np
import time

def main():
    # Create an audio context with context manager
    with ls.AudioContext() as ctx:
        print("Audio context created")
        
        # Create a function node for custom processing
        func = ctx.create_function(channels=1)
        print("Function node created")
        
        # Create a phase variable to keep track of our position
        phase = 0.0
        
        # Define the processing function
        def process_audio(channel, buffer):
            nonlocal phase
            
            # Convert buffer to numpy array
            samples = np.array(buffer)
            
            # Generate a time array for this buffer
            sample_rate = 44100  # Assuming 44.1kHz sample rate
            buffer_duration = len(samples) / sample_rate
            time_points = np.linspace(phase, phase + buffer_duration, len(samples), endpoint=False)
            
            # Apply a sine wave modulation
            modulation = np.sin(2 * np.pi * 5 * time_points)  # 5 Hz modulation
            processed = samples * (0.5 + 0.5 * modulation)  # Modulate between 0 and 1
            
            # Update the buffer in-place
            buffer[:] = processed
            
            # Update phase for next buffer
            phase += buffer_duration
        
        # Assign the processing function
        func.set_process_function(process_audio)
        print("Processing function assigned")
        
        # Create an oscillator as a sound source
        osc = ctx.create_oscillator()
        osc.type = "sawtooth"
        osc.frequency.value = 220  # A3 note
        print("Oscillator created and configured")
        
        # Create a gain node for output level control
        gain = ctx.create_gain()
        gain.gain.value = 0.3  # Lower volume to avoid clipping
        print("Gain created and configured")
        
        # Connect the nodes: oscillator -> function -> gain -> destination
        osc >> func >> gain >> ctx.destination
        print("Nodes connected")
        
        # Start the oscillator and function node
        osc.start(0)
        func.start()
        print("Processing started")
        
        # Let it run for 5 seconds
        print("Running for 5 seconds...")
        ctx.wait(5)
        
        # Stop the oscillator and function node
        osc.stop(0)
        func.stop()
        print("Processing stopped")
        
        # Wait a bit for the sound to finish
        time.sleep(0.1)
        
    print("Audio context closed")

if __name__ == "__main__":
    main()
