"""
AudioBufferSourceNode class for LabSound Python bindings.
"""

from typing import Optional, Union

from ..audio_node import AudioNode
from ..audio_param import AudioParam


class AudioBufferSourceNode(AudioNode):
    """
    The AudioBufferSourceNode interface represents an audio source consisting of
    in-memory audio data, stored in an AudioBuffer.
    
    This is useful for playing back pre-loaded audio samples or generated audio data.
    """
    
    def __init__(self, context, node):
        """
        Initialize a new AudioBufferSourceNode.
        
        Args:
            context: The AudioContext this node belongs to
            node: The underlying C++ node object
        """
        super().__init__(context, node)
        self._playback_rate = None
        self._detune = None
        self._doppler_rate = None
    
    @property
    def playback_rate(self) -> AudioParam:
        """
        Get the playback rate parameter.
        
        The playback rate parameter controls the speed at which the audio is played.
        A value of 1.0 represents normal speed, 0.5 represents half speed, and 2.0
        represents double speed.
        
        Returns:
            The playback rate AudioParam
        """
        if self._playback_rate is None:
            self._playback_rate = AudioParam(self.context, self._node.playback_rate())
        return self._playback_rate
    
    @property
    def detune(self) -> AudioParam:
        """
        Get the detune parameter.
        
        The detune parameter represents detuning of playback in cents (1/100 of a semitone).
        
        Returns:
            The detune AudioParam
        """
        if self._detune is None:
            self._detune = AudioParam(self.context, self._node.detune())
        return self._detune
    
    @property
    def doppler_rate(self) -> AudioParam:
        """
        Get the Doppler rate parameter.
        
        The Doppler rate parameter controls the Doppler effect for the audio source.
        
        Returns:
            The Doppler rate AudioParam
        """
        if self._doppler_rate is None:
            self._doppler_rate = AudioParam(self.context, self._node.doppler_rate())
        return self._doppler_rate
    
    def set_buffer(self, buffer):
        """
        Set the audio buffer to play.
        
        Args:
            buffer: The audio buffer to play
        """
        self._node.set_buffer(buffer)
    
    def get_buffer(self):
        """
        Get the current audio buffer.
        
        Returns:
            The current audio buffer
        """
        return self._node.get_buffer()
    
    def start(self, when: float = 0, offset: float = 0, duration: float = 0):
        """
        Start playing the audio buffer.
        
        Args:
            when: When to start playing, in seconds
            offset: The offset into the buffer to start playing, in seconds
            duration: The duration to play, in seconds (0 means play to the end)
        """
        self._node.start(when, offset, duration)
    
    def stop(self, when: float = 0):
        """
        Stop playing the audio buffer.
        
        Args:
            when: When to stop playing, in seconds
        """
        self._node.stop(when)
    
    def set_loop(self, loop: bool, loop_start: float = 0, loop_end: float = 0):
        """
        Set the looping parameters.
        
        Args:
            loop: Whether to loop the audio
            loop_start: The start time of the loop, in seconds
            loop_end: The end time of the loop, in seconds
        """
        self._node.set_loop(loop, loop_start, loop_end)
    
    @property
    def is_looping(self) -> bool:
        """
        Check if the audio is looping.
        
        Returns:
            True if the audio is looping, False otherwise
        """
        return self._node.is_looping()
    
    @property
    def loop_start(self) -> float:
        """
        Get the loop start time.
        
        Returns:
            The loop start time, in seconds
        """
        return self._node.loop_start()
    
    @property
    def loop_end(self) -> float:
        """
        Get the loop end time.
        
        Returns:
            The loop end time, in seconds
        """
        return self._node.loop_end()
    
    @property
    def is_playing(self) -> bool:
        """
        Check if the audio is currently playing.
        
        Returns:
            True if the audio is playing, False otherwise
        """
        return self._node.is_playing()
    
    def get_cursor(self) -> float:
        """
        Get the current playback position.
        
        Returns:
            The current playback position, in seconds
        """
        return self._node.get_cursor()
    
    def load_file(self, file_path: str):
        """
        Load an audio file into the buffer.
        
        Args:
            file_path: The path to the audio file
        """
        # This would need to be implemented in the C++ bindings
        # For now, we'll just raise a NotImplementedError
        raise NotImplementedError("Loading audio files is not yet implemented")
