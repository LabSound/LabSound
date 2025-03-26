"""
ConstantSourceNode class for LabSound Python bindings.
"""

from typing import Optional, Union

from ..audio_node import AudioNode
from ..audio_param import AudioParam


class ConstantSourceNode(AudioNode):
    """
    The ConstantSourceNode interface represents an audio source that outputs
    a constant value.
    
    This is useful for controlling audio parameters with a constant value or
    for creating DC offset.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new ConstantSourceNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
        self._offset = None
    
    @property
    def offset(self) -> AudioParam:
        """
        Get the offset parameter.
        
        Returns:
            The offset AudioParam
        """
        if self._offset is None:
            self._offset = AudioParam(self.context, self._node.offset())
        return self._offset
    
    def start(self, when: float = 0):
        """
        Start outputting the constant value.
        
        Args:
            when: When to start outputting, in seconds
        """
        self._node.start(when)
    
    def stop(self, when: float = 0):
        """
        Stop outputting the constant value.
        
        Args:
            when: When to stop outputting, in seconds
        """
        self._node.stop(when)
