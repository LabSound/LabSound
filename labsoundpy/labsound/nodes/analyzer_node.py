"""
AnalyzerNode class for LabSound Python bindings.
"""

import numpy as np
from typing import Optional, Union

from ..audio_node import AudioNode


class AnalyzerNode(AudioNode):
    """
    The AnalyzerNode interface represents a node which can provide real-time
    frequency and time-domain analysis information.
    
    This is useful for creating visualizations of audio data, such as
    frequency analyzers or waveform displays.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new AnalyzerNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
    
    @property
    def fft_size(self) -> int:
        """
        Get the current FFT size.
        
        Returns:
            The current FFT size
        """
        return self._node.fft_size()
    
    @fft_size.setter
    def fft_size(self, size: int):
        """
        Set the FFT size.
        
        Args:
            size: The FFT size, must be a power of 2 between 32 and 32768
        """
        self._node.set_fft_size(size)
    
    @property
    def frequency_bin_count(self) -> int:
        """
        Get the number of frequency bins.
        
        Returns:
            The number of frequency bins (equal to fft_size / 2)
        """
        return self._node.frequency_bin_count()
    
    @property
    def min_decibels(self) -> float:
        """
        Get the minimum decibel value.
        
        Returns:
            The minimum decibel value
        """
        return self._node.min_decibels()
    
    @min_decibels.setter
    def min_decibels(self, value: float):
        """
        Set the minimum decibel value.
        
        Args:
            value: The minimum decibel value
        """
        self._node.set_min_decibels(value)
    
    @property
    def max_decibels(self) -> float:
        """
        Get the maximum decibel value.
        
        Returns:
            The maximum decibel value
        """
        return self._node.max_decibels()
    
    @max_decibels.setter
    def max_decibels(self, value: float):
        """
        Set the maximum decibel value.
        
        Args:
            value: The maximum decibel value
        """
        self._node.set_max_decibels(value)
    
    @property
    def smoothing_time_constant(self) -> float:
        """
        Get the smoothing time constant.
        
        Returns:
            The smoothing time constant
        """
        return self._node.smoothing_time_constant()
    
    @smoothing_time_constant.setter
    def smoothing_time_constant(self, value: float):
        """
        Set the smoothing time constant.
        
        Args:
            value: The smoothing time constant, between 0 and 1
        """
        self._node.set_smoothing_time_constant(value)
    
    def get_float_frequency_data(self) -> np.ndarray:
        """
        Get the current frequency data as a float array.
        
        Returns:
            A numpy array containing the frequency data
        """
        return self._node.get_float_frequency_data()
    
    def get_byte_frequency_data(self, resample: bool = False) -> np.ndarray:
        """
        Get the current frequency data as a byte array.
        
        Args:
            resample: Whether to resample the data
            
        Returns:
            A numpy array containing the frequency data
        """
        return self._node.get_byte_frequency_data(resample)
    
    def get_float_time_domain_data(self) -> np.ndarray:
        """
        Get the current time-domain data as a float array.
        
        Returns:
            A numpy array containing the time-domain data
        """
        return self._node.get_float_time_domain_data()
    
    def get_byte_time_domain_data(self) -> np.ndarray:
        """
        Get the current time-domain data as a byte array.
        
        Returns:
            A numpy array containing the time-domain data
        """
        return self._node.get_byte_time_domain_data()
