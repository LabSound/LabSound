"""
Basic tests for LabSound Python bindings.
"""

import unittest
import labsound as ls
import time

class TestAudioContext(unittest.TestCase):
    """Test the AudioContext class."""
    
    def test_create_context(self):
        """Test creating an AudioContext."""
        ctx = ls.AudioContext()
        self.assertIsNotNone(ctx)
        self.assertIsNotNone(ctx.destination)
        
    def test_context_manager(self):
        """Test using the AudioContext as a context manager."""
        with ls.AudioContext() as ctx:
            self.assertIsNotNone(ctx)
            self.assertIsNotNone(ctx.destination)
            
    def test_current_time(self):
        """Test the current_time property."""
        with ls.AudioContext() as ctx:
            t1 = ctx.current_time
            time.sleep(0.1)
            t2 = ctx.current_time
            self.assertGreater(t2, t1)


class TestOscillatorNode(unittest.TestCase):
    """Test the OscillatorNode class."""
    
    def test_create_oscillator(self):
        """Test creating an OscillatorNode."""
        with ls.AudioContext() as ctx:
            osc = ctx.create_oscillator()
            self.assertIsNotNone(osc)
            
    def test_oscillator_type(self):
        """Test setting and getting the oscillator type."""
        with ls.AudioContext() as ctx:
            osc = ctx.create_oscillator()
            
            # Test all types
            for type_str in ["sine", "square", "sawtooth", "triangle"]:
                osc.type = type_str
                self.assertEqual(osc.type, type_str)
                
    def test_oscillator_frequency(self):
        """Test setting and getting the oscillator frequency."""
        with ls.AudioContext() as ctx:
            osc = ctx.create_oscillator()
            
            # Test setting frequency
            osc.frequency.value = 440
            self.assertEqual(osc.frequency.value, 440)
            
            # Test setting frequency with automation
            osc.frequency.linear_ramp_to_value_at_time(880, 1.0)
            
    def test_start_stop(self):
        """Test starting and stopping the oscillator."""
        with ls.AudioContext() as ctx:
            osc = ctx.create_oscillator()
            osc.start(0)
            osc.stop(0.1)


class TestGainNode(unittest.TestCase):
    """Test the GainNode class."""
    
    def test_create_gain(self):
        """Test creating a GainNode."""
        with ls.AudioContext() as ctx:
            gain = ctx.create_gain()
            self.assertIsNotNone(gain)
            
    def test_gain_value(self):
        """Test setting and getting the gain value."""
        with ls.AudioContext() as ctx:
            gain = ctx.create_gain()
            
            # Test setting gain
            gain.gain.value = 0.5
            self.assertEqual(gain.gain.value, 0.5)
            
            # Test setting gain with automation
            gain.gain.linear_ramp_to_value_at_time(0.0, 1.0)


class TestConnections(unittest.TestCase):
    """Test connecting nodes."""
    
    def test_connect_nodes(self):
        """Test connecting nodes using the connect method."""
        with ls.AudioContext() as ctx:
            osc = ctx.create_oscillator()
            gain = ctx.create_gain()
            
            # Connect using the connect method
            osc.connect(gain)
            gain.connect(ctx.destination)
            
    def test_connect_operator(self):
        """Test connecting nodes using the >> operator."""
        with ls.AudioContext() as ctx:
            osc = ctx.create_oscillator()
            gain = ctx.create_gain()
            
            # Connect using the >> operator
            osc >> gain >> ctx.destination
            
    def test_disconnect(self):
        """Test disconnecting nodes."""
        with ls.AudioContext() as ctx:
            osc = ctx.create_oscillator()
            gain = ctx.create_gain()
            
            # Connect and then disconnect
            osc >> gain
            osc.disconnect()


if __name__ == "__main__":
    unittest.main()
