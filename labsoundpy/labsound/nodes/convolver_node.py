"""
ConvolverNode class for LabSound Python bindings.
"""

from typing import Optional, Union

from ..audio_node import AudioNode


class ConvolverNode(AudioNode):
    """
    The ConvolverNode interface represents a processing node that applies a
    linear convolution effect to the audio signal.
    
    This is commonly used for reverb effects, spatial audio, and other
    effects that can be represented as an impulse response.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new ConvolverNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
    
    @property
    def normalize(self) -> bool:
        """
        Get the normalize flag.
        
        Returns:
            True if normalization is enabled, False otherwise
        """
        return self._node.normalize()
    
    @normalize.setter
    def normalize(self, value: bool):
        """
        Set the normalize flag.
        
        Args:
            value: True to enable normalization, False to disable
        """
        self._node.set_normalize(value)
    
    def set_impulse(self, impulse):
        """
        Set the impulse response.
        
        Args:
            impulse: The impulse response as an AudioBus
        """
        self._node.set_impulse(impulse)
    
    def get_impulse(self):
        """
        Get the impulse response.
        
        Returns:
            The impulse response as an AudioBus
        """
        return self._node.get_impulse()
    
    def start(self, when: float = 0):
        """
        Start the convolution.
        
        Args:
            when: When to start, in seconds
        """
        self._node.start(when)
    
    def stop(self, when: float = 0):
        """
        Stop the convolution.
        
        Args:
            when: When to stop, in seconds
        """
        self._node.stop(when)
