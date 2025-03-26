"""
AudioNode base class for LabSound Python bindings.
"""

from typing import Any, Optional, Union


class AudioNode:
    """
    Base class for all audio nodes in the LabSound graph.
    
    AudioNode represents a processing node in the audio graph. It can have inputs
    and outputs, and can be connected to other AudioNodes to form a processing chain.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new AudioNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        self.context = context
        self._node = node
        
    def __rshift__(self, other: 'AudioNode') -> 'AudioNode':
        """
        Implement the >> operator for connection.
        
        This allows for a more intuitive syntax when connecting nodes:
        source >> destination
        
        Args:
            other: The destination AudioNode
            
        Returns:
            The destination AudioNode (for chaining)
        """
        self.context.connect(self, other)
        return other
        
    def __lshift__(self, other: 'AudioNode') -> 'AudioNode':
        """
        Implement the << operator for reverse connection.
        
        This allows for a more intuitive syntax when connecting nodes in reverse:
        destination << source
        
        Args:
            other: The source AudioNode
            
        Returns:
            The source AudioNode (for chaining)
        """
        self.context.connect(other, self)
        return other
    
    def connect(self, destination: 'AudioNode', output: int = 0, input: int = 0) -> 'AudioNode':
        """
        Connect this node to a destination node.
        
        Args:
            destination: The destination AudioNode
            output: The output index on this node (default: 0)
            input: The input index on the destination node (default: 0)
            
        Returns:
            The destination AudioNode (for chaining)
        """
        self.context.connect(self, destination, output, input)
        return destination
    
    def disconnect(self):
        """
        Disconnect all outputs of this node.
        """
        self.context.disconnect(self)
