"""
Simple oscillator example using LabSound Python bindings.

This example creates a sine wave oscillator, connects it to a gain node,
and plays it for 2 seconds.
"""

import labsound as ls
import time

def main():
    # Create an audio context with context manager
    with ls.AudioContext() as ctx:
        print("Audio context created")
        
        # Create an oscillator node
        osc = ctx.create_oscillator()
        print("Oscillator created")
        
        # Create a gain node
        gain = ctx.create_gain()
        print("Gain created")
        
        # Set oscillator parameters
        osc.type = "sine"  # Using string instead of enum
        osc.frequency.value = 440  # A4 note
        print("Oscillator configured")
        
        # Set gain parameter
        gain.gain.value = 0.5  # Half volume
        print("Gain configured")
        
        # Connect nodes: oscillator -> gain -> destination
        osc >> gain >> ctx.destination
        print("Nodes connected")
        
        # Start the oscillator
        osc.start(0)  # Start immediately
        print("Oscillator started")
        
        # Let it play for 2 seconds
        print("Playing for 2 seconds...")
        ctx.wait(2)
        
        # Stop the oscillator
        osc.stop(0)
        print("Oscillator stopped")
        
        # Wait a bit for the sound to finish
        time.sleep(0.1)
        
    print("Audio context closed")

if __name__ == "__main__":
    main()
