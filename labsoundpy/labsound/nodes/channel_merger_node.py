"""
ChannelMergerNode class for LabSound Python bindings.
"""

from typing import Optional, Union

from ..audio_node import AudioNode


class ChannelMergerNode(AudioNode):
    """
    The ChannelMergerNode interface represents a node which combines individual
    audio channels into a multi-channel stream.
    
    This is useful for combining mono audio streams into stereo or surround sound.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new ChannelMergerNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
    
    def add_inputs(self, count: int):
        """
        Add additional input channels to the node.
        
        Args:
            count: The number of input channels to add
        """
        self._node.add_inputs(count)
    
    def set_output_channel_count(self, count: int):
        """
        Set the number of output channels.
        
        Args:
            count: The number of output channels
        """
        self._node.set_output_channel_count(count)
