"""
DelayNode class for LabSound Python bindings.
"""

from typing import Optional, Union

from ..audio_node import AudioNode
from ..audio_param import AudioParam


class DelayNode(AudioNode):
    """
    The DelayNode interface represents a delay-line; an AudioNode that causes a
    delay between the arrival of an input data and its propagation to the output.
    
    This is useful for creating echo effects, for delaying a signal, or for
    creating time-based audio effects.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new DelayNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
        self._delay_time = None
    
    @property
    def delay_time(self) -> AudioParam:
        """
        Get the delay time parameter.
        
        Returns:
            The delay time AudioParam
        """
        if self._delay_time is None:
            self._delay_time = AudioParam(self.context, self._node.delay_time())
        return self._delay_time
