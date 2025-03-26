"""
AudioDestinationNode class for LabSound Python bindings.
"""

from typing import Optional, Union

from ..audio_node import AudioNode


class AudioDestinationNode(AudioNode):
    """
    The AudioDestinationNode interface represents the end destination of an audio
    graph in a given context — usually the speakers of your device.
    
    This is the final node in the audio graph, and it cannot be connected to other nodes.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new AudioDestinationNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
    
    @property
    def max_channel_count(self) -> int:
        """
        Get the maximum number of channels that this destination can support.
        
        Returns:
            The maximum number of channels
        """
        return self._node.max_channel_count()
    
    def set_channel_count(self, count: int):
        """
        Set the number of channels for this destination.
        
        Args:
            count: The number of channels
        """
        self._node.set_channel_count(count)
