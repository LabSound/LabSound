"""
ChannelSplitterNode class for LabSound Python bindings.
"""

from typing import Optional, Union

from ..audio_node import AudioNode


class ChannelSplitterNode(AudioNode):
    """
    The ChannelSplitterNode interface represents a node which separates a
    multi-channel audio stream into individual channels.
    
    This is useful for processing individual channels of a stereo or surround
    sound audio stream.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new ChannelSplitterNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
    
    def add_outputs(self, count: int):
        """
        Add additional output channels to the node.
        
        Args:
            count: The number of output channels to add
        """
        self._node.add_outputs(count)
