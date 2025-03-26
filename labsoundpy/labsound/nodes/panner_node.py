"""
PannerNode class for LabSound Python bindings.
"""

from typing import Optional, Union, Tuple

from ..audio_node import AudioNode
from ..audio_param import AudioParam


class PannerNode(AudioNode):
    """
    The PannerNode interface represents a processing node which positions/spatializes
    an incoming audio stream in three-dimensional space.
    
    This is useful for creating 3D audio effects and spatial audio.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new PannerNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
        self._distance_gain = None
        self._cone_gain = None
    
    def set_position(self, x: float, y: float, z: float):
        """
        Set the position of the audio source in 3D space.
        
        Args:
            x: The x coordinate
            y: The y coordinate
            z: The z coordinate
        """
        self._node.set_position(x, y, z)
    
    @property
    def position(self) -> Tuple[float, float, float]:
        """
        Get the position of the audio source in 3D space.
        
        Returns:
            A tuple of (x, y, z) coordinates
        """
        return (
            self._node.position_x(),
            self._node.position_y(),
            self._node.position_z()
        )
    
    def set_orientation(self, x: float, y: float, z: float):
        """
        Set the orientation of the audio source in 3D space.
        
        Args:
            x: The x component of the orientation vector
            y: The y component of the orientation vector
            z: The z component of the orientation vector
        """
        self._node.set_orientation(x, y, z)
    
    @property
    def orientation(self) -> Tuple[float, float, float]:
        """
        Get the orientation of the audio source in 3D space.
        
        Returns:
            A tuple of (x, y, z) components of the orientation vector
        """
        return (
            self._node.orientation_x(),
            self._node.orientation_y(),
            self._node.orientation_z()
        )
    
    def set_velocity(self, x: float, y: float, z: float):
        """
        Set the velocity of the audio source in 3D space.
        
        Args:
            x: The x component of the velocity vector
            y: The y component of the velocity vector
            z: The z component of the velocity vector
        """
        self._node.set_velocity(x, y, z)
    
    @property
    def velocity(self) -> Tuple[float, float, float]:
        """
        Get the velocity of the audio source in 3D space.
        
        Returns:
            A tuple of (x, y, z) components of the velocity vector
        """
        return (
            self._node.velocity_x(),
            self._node.velocity_y(),
            self._node.velocity_z()
        )
    
    @property
    def distance_gain(self) -> AudioParam:
        """
        Get the distance gain parameter.
        
        Returns:
            The distance gain AudioParam
        """
        if self._distance_gain is None:
            self._distance_gain = AudioParam(self.context, self._node.distance_gain())
        return self._distance_gain
    
    @property
    def cone_gain(self) -> AudioParam:
        """
        Get the cone gain parameter.
        
        Returns:
            The cone gain AudioParam
        """
        if self._cone_gain is None:
            self._cone_gain = AudioParam(self.context, self._node.cone_gain())
        return self._cone_gain
    
    @property
    def panning_model(self) -> str:
        """
        Get the panning model.
        
        Returns:
            The panning model as a string
        """
        return self._node.panning_model_string()
    
    @panning_model.setter
    def panning_model(self, model: str):
        """
        Set the panning model.
        
        Args:
            model: The panning model as a string ('equalpower' or 'HRTF')
        """
        self._node.set_panning_model_string(model)
    
    @property
    def distance_model(self) -> str:
        """
        Get the distance model.
        
        Returns:
            The distance model as a string
        """
        return self._node.distance_model_string()
    
    @distance_model.setter
    def distance_model(self, model: str):
        """
        Set the distance model.
        
        Args:
            model: The distance model as a string ('linear', 'inverse', or 'exponential')
        """
        self._node.set_distance_model_string(model)
    
    @property
    def ref_distance(self) -> float:
        """
        Get the reference distance.
        
        Returns:
            The reference distance
        """
        return self._node.ref_distance()
    
    @ref_distance.setter
    def ref_distance(self, distance: float):
        """
        Set the reference distance.
        
        Args:
            distance: The reference distance
        """
        self._node.set_ref_distance(distance)
    
    @property
    def max_distance(self) -> float:
        """
        Get the maximum distance.
        
        Returns:
            The maximum distance
        """
        return self._node.max_distance()
    
    @max_distance.setter
    def max_distance(self, distance: float):
        """
        Set the maximum distance.
        
        Args:
            distance: The maximum distance
        """
        self._node.set_max_distance(distance)
    
    @property
    def rolloff_factor(self) -> float:
        """
        Get the rolloff factor.
        
        Returns:
            The rolloff factor
        """
        return self._node.rolloff_factor()
    
    @rolloff_factor.setter
    def rolloff_factor(self, factor: float):
        """
        Set the rolloff factor.
        
        Args:
            factor: The rolloff factor
        """
        self._node.set_rolloff_factor(factor)
    
    @property
    def cone_inner_angle(self) -> float:
        """
        Get the cone inner angle.
        
        Returns:
            The cone inner angle in degrees
        """
        return self._node.cone_inner_angle()
    
    @cone_inner_angle.setter
    def cone_inner_angle(self, angle: float):
        """
        Set the cone inner angle.
        
        Args:
            angle: The cone inner angle in degrees
        """
        self._node.set_cone_inner_angle(angle)
    
    @property
    def cone_outer_angle(self) -> float:
        """
        Get the cone outer angle.
        
        Returns:
            The cone outer angle in degrees
        """
        return self._node.cone_outer_angle()
    
    @cone_outer_angle.setter
    def cone_outer_angle(self, angle: float):
        """
        Set the cone outer angle.
        
        Args:
            angle: The cone outer angle in degrees
        """
        self._node.set_cone_outer_angle(angle)
    
    @property
    def cone_outer_gain(self) -> float:
        """
        Get the cone outer gain.
        
        Returns:
            The cone outer gain
        """
        return self._node.cone_outer_gain()
    
    @cone_outer_gain.setter
    def cone_outer_gain(self, gain: float):
        """
        Set the cone outer gain.
        
        Args:
            gain: The cone outer gain
        """
        self._node.set_cone_outer_gain(gain)
