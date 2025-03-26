"""
WaveShaperNode class for LabSound Python bindings.
"""

import numpy as np
from typing import Optional, Union

from ..audio_node import AudioNode


class WaveShaperNode(AudioNode):
    """
    The WaveShaperNode interface represents a non-linear distorter that uses a
    curve to apply a waveshaping distortion to the signal.
    
    This is useful for distortion effects, audio compression, and other
    non-linear audio processing.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new WaveShaperNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
    
    @property
    def curve(self) -> np.ndarray:
        """
        Get the curve used for the waveshaping effect.
        
        Returns:
            A numpy array containing the curve data
        """
        return self._node.curve()
    
    @curve.setter
    def curve(self, curve: np.ndarray):
        """
        Set the curve used for the waveshaping effect.
        
        Args:
            curve: A numpy array containing the curve data
        """
        # Ensure the curve is a float32 numpy array
        curve = np.asarray(curve, dtype=np.float32)
        self._node.set_curve(curve)
    
    @property
    def oversample(self) -> str:
        """
        Get the oversampling mode.
        
        Returns:
            The oversampling mode as a string ('none', '2x', or '4x')
        """
        return self._node.oversample_string()
    
    @oversample.setter
    def oversample(self, oversample: str):
        """
        Set the oversampling mode.
        
        Args:
            oversample: The oversampling mode as a string ('none', '2x', or '4x')
        """
        self._node.set_oversample_string(oversample)
