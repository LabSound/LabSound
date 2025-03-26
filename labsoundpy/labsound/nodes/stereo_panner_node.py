"""
StereoPannerNode class for LabSound Python bindings.
"""

from typing import Optional, Union

from ..audio_node import AudioNode
from ..audio_param import AudioParam


class StereoPannerNode(AudioNode):
    """
    The StereoPannerNode interface represents a simple stereo panner node that
    can be used to pan an audio stream left or right.
    
    This is useful for simple stereo panning effects.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new StereoPannerNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
        self._pan = None
    
    @property
    def pan(self) -> AudioParam:
        """
        Get the pan parameter.
        
        The pan parameter represents the position of the input in the stereo image.
        A value of -1 represents full left, 0 represents center, and 1 represents
        full right.
        
        Returns:
            The pan AudioParam
        """
        if self._pan is None:
            self._pan = AudioParam(self.context, self._node.pan())
        return self._pan
