"""
DynamicsCompressorNode class for LabSound Python bindings.
"""

from typing import Optional, Union

from ..audio_node import AudioNode
from ..audio_param import AudioParam


class DynamicsCompressorNode(AudioNode):
    """
    The DynamicsCompressorNode interface provides a compression effect, which
    lowers the volume of the loudest parts of the signal and raises the volume
    of the quietest parts.
    
    This is useful for controlling the dynamic range of audio, preventing
    clipping, and creating various audio effects.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new DynamicsCompressorNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
        self._threshold = None
        self._knee = None
        self._ratio = None
        self._attack = None
        self._release = None
        self._reduction = None
    
    @property
    def threshold(self) -> AudioParam:
        """
        Get the threshold parameter.
        
        The threshold parameter represents the decibel value above which
        compression will be applied to the signal.
        
        Returns:
            The threshold AudioParam
        """
        if self._threshold is None:
            self._threshold = AudioParam(self.context, self._node.threshold())
        return self._threshold
    
    @property
    def knee(self) -> AudioParam:
        """
        Get the knee parameter.
        
        The knee parameter represents the decibel value that determines how
        gradually the compression starts as the signal level approaches the
        threshold.
        
        Returns:
            The knee AudioParam
        """
        if self._knee is None:
            self._knee = AudioParam(self.context, self._node.knee())
        return self._knee
    
    @property
    def ratio(self) -> AudioParam:
        """
        Get the ratio parameter.
        
        The ratio parameter represents the amount of compression applied to the
        signal above the threshold, expressed as a ratio of input to output.
        
        Returns:
            The ratio AudioParam
        """
        if self._ratio is None:
            self._ratio = AudioParam(self.context, self._node.ratio())
        return self._ratio
    
    @property
    def attack(self) -> AudioParam:
        """
        Get the attack parameter.
        
        The attack parameter represents the time in seconds it takes for the
        compression to start reducing the signal after it exceeds the threshold.
        
        Returns:
            The attack AudioParam
        """
        if self._attack is None:
            self._attack = AudioParam(self.context, self._node.attack())
        return self._attack
    
    @property
    def release(self) -> AudioParam:
        """
        Get the release parameter.
        
        The release parameter represents the time in seconds it takes for the
        compression to stop reducing the signal after it falls below the threshold.
        
        Returns:
            The release AudioParam
        """
        if self._release is None:
            self._release = AudioParam(self.context, self._node.release())
        return self._release
    
    @property
    def reduction(self) -> AudioParam:
        """
        Get the reduction parameter.
        
        The reduction parameter represents the amount of gain reduction currently
        applied by the compressor to the signal, in decibels.
        
        Returns:
            The reduction AudioParam
        """
        if self._reduction is None:
            self._reduction = AudioParam(self.context, self._node.reduction())
        return self._reduction
