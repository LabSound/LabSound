"""
FunctionNode class for LabSound Python bindings.
"""

from typing import Optional, Union, Callable

import numpy as np

from ..audio_node import AudioNode


class FunctionNode(AudioNode):
    """
    The FunctionNode interface represents a processing node that allows for
    custom audio processing using a Python function.
    
    This is useful for implementing custom audio effects, generators, or
    processors in Python.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new FunctionNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
    
    def set_process_function(self, process_function: Callable[[int, np.ndarray], None]):
        """
        Set the processing function for this node.
        
        The processing function is called for each channel of audio data that
        needs to be processed. It receives the channel index and a numpy array
        containing the audio data for that channel. The function should modify
        the array in-place to apply the desired processing.
        
        Args:
            process_function: A function that takes a channel index and a numpy array
                             and modifies the array in-place
        """
        self._node.set_process_function(process_function)
    
    def start(self, when: float = 0):
        """
        Start processing audio.
        
        Args:
            when: When to start processing, in seconds
        """
        self._node.start(when)
    
    def stop(self, when: float = 0):
        """
        Stop processing audio.
        
        Args:
            when: When to stop processing, in seconds
        """
        self._node.stop(when)
