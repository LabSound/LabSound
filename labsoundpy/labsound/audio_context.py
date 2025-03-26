"""
AudioContext class for LabSound Python bindings.
"""

import time
from typing import Optional, Union, Type

# Import the C++ bindings
from ._core import _AudioContext

# Import node types (will be used for factory methods)
# These will be imported inside methods to avoid circular imports


class AudioContext:
    """
    The AudioContext interface represents an audio-processing graph built from audio
    modules linked together, each represented by an AudioNode.
    
    This is the main entry point for working with the LabSound audio engine.
    """
    
    def __init__(self, sample_rate: int = 44100, channels: int = 2, buffer_size: int = 256):
        """
        Initialize a new AudioContext.
        
        Args:
            sample_rate: The sample rate in Hz (default: 44100)
            channels: The number of output channels (default: 2)
            buffer_size: The audio buffer size in frames (default: 256)
        """
        self._context = _AudioContext(sample_rate, channels, buffer_size)
        self._destination = None  # Will be initialized on first access
        
    def __enter__(self):
        """
        Context manager entry point - starts the audio context.
        """
        # Start the audio context
        self._context.start()
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Context manager exit point - stops the audio context.
        """
        # Stop the audio context
        self._context.stop()
    
    @property
    def destination(self):
        """
        Returns the AudioDestinationNode for this context, which represents the final
        destination for all audio in this context.
        """
        if self._destination is None:
            # Import here to avoid circular imports
            from .nodes.destination_node import AudioDestinationNode
            self._destination = AudioDestinationNode(self, self._context.destination())
        return self._destination
    
    @property
    def current_time(self) -> float:
        """
        Returns the current audio context time in seconds.
        """
        return self._context.current_time()
    
    def wait(self, seconds: float):
        """
        Block until the specified time has passed.
        
        Args:
            seconds: The time to wait in seconds
        """
        # Simple implementation using Python's time.sleep
        # In a more advanced implementation, this could sync with the audio clock
        time.sleep(seconds)
    
    def connect(self, source, destination, output: int = 0, input: int = 0):
        """
        Connect an audio source to an audio destination.
        
        Args:
            source: The source AudioNode
            destination: The destination AudioNode
            output: The output index on the source (default: 0)
            input: The input index on the destination (default: 0)
        """
        self._context.connect(source._node, destination._node, output, input)
    
    def disconnect(self, node):
        """
        Disconnect all outputs of an AudioNode.
        
        Args:
            node: The AudioNode to disconnect
        """
        self._context.disconnect(node._node)
    
    # Factory methods for creating audio nodes
    
    def create_oscillator(self):
        """
        Create a new OscillatorNode.
        
        Returns:
            An OscillatorNode instance
        """
        from .nodes.oscillator_node import OscillatorNode
        return OscillatorNode(self, self._context.create_oscillator())
    
    def create_gain(self):
        """
        Create a new GainNode.
        
        Returns:
            A GainNode instance
        """
        from .nodes.gain_node import GainNode
        return GainNode(self, self._context.create_gain())
    
    def create_biquad_filter(self):
        """
        Create a new BiquadFilterNode.
        
        Returns:
            A BiquadFilterNode instance
        """
        from .nodes.biquad_filter_node import BiquadFilterNode
        return BiquadFilterNode(self, self._context.create_biquad_filter())
    
    def create_buffer_source(self, file_path: Optional[str] = None):
        """
        Create a new AudioBufferSourceNode.
        
        Args:
            file_path: Optional path to an audio file to load
            
        Returns:
            An AudioBufferSourceNode instance
        """
        from .nodes.buffer_source_node import AudioBufferSourceNode
        node = AudioBufferSourceNode(self, self._context.create_buffer_source())
        
        if file_path:
            node.load_file(file_path)
            
        return node
    
    def create_analyzer(self):
        """
        Create a new AnalyzerNode.
        
        Returns:
            An AnalyzerNode instance
        """
        from .nodes.analyzer_node import AnalyzerNode
        return AnalyzerNode(self, self._context.create_analyzer())
    
    def create_function(self, channels: int = 1):
        """
        Create a new FunctionNode for custom audio processing.
        
        Args:
            channels: The number of channels to process
            
        Returns:
            A FunctionNode instance
        """
        from .nodes.function_node import FunctionNode
        return FunctionNode(self, self._context.create_function(channels))
    
    def create_channel_merger(self, inputs: int = 2):
        """
        Create a new ChannelMergerNode.
        
        Args:
            inputs: The number of input channels
            
        Returns:
            A ChannelMergerNode instance
        """
        from .nodes.channel_merger_node import ChannelMergerNode
        return ChannelMergerNode(self, self._context.create_channel_merger(inputs))
    
    def create_channel_splitter(self, outputs: int = 2):
        """
        Create a new ChannelSplitterNode.
        
        Args:
            outputs: The number of output channels
            
        Returns:
            A ChannelSplitterNode instance
        """
        from .nodes.channel_splitter_node import ChannelSplitterNode
        return ChannelSplitterNode(self, self._context.create_channel_splitter(outputs))
    
    def create_constant_source(self):
        """
        Create a new ConstantSourceNode.
        
        Returns:
            A ConstantSourceNode instance
        """
        from .nodes.constant_source_node import ConstantSourceNode
        return ConstantSourceNode(self, self._context.create_constant_source())
    
    def create_convolver(self):
        """
        Create a new ConvolverNode.
        
        Returns:
            A ConvolverNode instance
        """
        from .nodes.convolver_node import ConvolverNode
        return ConvolverNode(self, self._context.create_convolver())
    
    def create_delay(self, max_delay_time: float = 2.0):
        """
        Create a new DelayNode.
        
        Args:
            max_delay_time: The maximum delay time in seconds
            
        Returns:
            A DelayNode instance
        """
        from .nodes.delay_node import DelayNode
        return DelayNode(self, self._context.create_delay(max_delay_time))
    
    def create_dynamics_compressor(self):
        """
        Create a new DynamicsCompressorNode.
        
        Returns:
            A DynamicsCompressorNode instance
        """
        from .nodes.dynamics_compressor_node import DynamicsCompressorNode
        return DynamicsCompressorNode(self, self._context.create_dynamics_compressor())
    
    def create_panner(self):
        """
        Create a new PannerNode.
        
        Returns:
            A PannerNode instance
        """
        from .nodes.panner_node import PannerNode
        return PannerNode(self, self._context.create_panner())
    
    def create_stereo_panner(self):
        """
        Create a new StereoPannerNode.
        
        Returns:
            A StereoPannerNode instance
        """
        from .nodes.stereo_panner_node import StereoPannerNode
        return StereoPannerNode(self, self._context.create_stereo_panner())
    
    def create_wave_shaper(self):
        """
        Create a new WaveShaperNode.
        
        Returns:
            A WaveShaperNode instance
        """
        from .nodes.wave_shaper_node import WaveShaperNode
        return WaveShaperNode(self, self._context.create_wave_shaper())
