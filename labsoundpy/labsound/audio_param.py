"""
AudioParam class for LabSound Python bindings.
"""

from typing import Optional, Union


class AudioParam:
    """
    AudioParam represents an audio-related parameter, usually of an AudioNode.
    
    It can be set to a specific value or scheduled to change at specific times.
    """
    
    def __init__(self, context, param):
        """
        Initialize a new AudioParam.
        
        Args:
            context: The AudioContext this parameter belongs to
            param: The underlying C++ parameter object
        """
        self.context = context
        self._param = param
    
    @property
    def value(self) -> float:
        """
        Get the current value of this parameter.
        
        Returns:
            The current value
        """
        return self._param.value()
    
    @value.setter
    def value(self, val: float):
        """
        Set the value of this parameter immediately.
        
        Args:
            val: The new value
        """
        self._param.set_value(val)
    
    def set_value_at_time(self, value: float, time: float) -> 'AudioParam':
        """
        Schedule a parameter value change at the given time.
        
        Args:
            value: The value to set
            time: The time in seconds to set the value
            
        Returns:
            Self (for method chaining)
        """
        self._param.set_value_at_time(value, time)
        return self
    
    def linear_ramp_to_value_at_time(self, value: float, end_time: float) -> 'AudioParam':
        """
        Schedule a linear continuous change in parameter value from the current value
        to the given value at the given time.
        
        Args:
            value: The value to ramp to
            end_time: The time in seconds to reach the value
            
        Returns:
            Self (for method chaining)
        """
        self._param.linear_ramp_to_value_at_time(value, end_time)
        return self
    
    def exponential_ramp_to_value_at_time(self, value: float, end_time: float) -> 'AudioParam':
        """
        Schedule an exponential continuous change in parameter value from the current value
        to the given value at the given time.
        
        Args:
            value: The value to ramp to (must be positive)
            end_time: The time in seconds to reach the value
            
        Returns:
            Self (for method chaining)
        """
        self._param.exponential_ramp_to_value_at_time(value, end_time)
        return self
    
    def set_target_at_time(self, target: float, start_time: float, time_constant: float) -> 'AudioParam':
        """
        Start exponentially approaching the target value at the given time with the given time constant.
        
        Args:
            target: The value to approach
            start_time: The time in seconds to begin approaching the value
            time_constant: The time constant of the exponential approach
            
        Returns:
            Self (for method chaining)
        """
        self._param.set_target_at_time(target, start_time, time_constant)
        return self
    
    def cancel_scheduled_values(self, start_time: float) -> 'AudioParam':
        """
        Cancel all scheduled parameter changes with times greater than or equal to start_time.
        
        Args:
            start_time: The time in seconds to begin canceling scheduled values
            
        Returns:
            Self (for method chaining)
        """
        self._param.cancel_scheduled_values(start_time)
        return self
