"""
LabSound Python Bindings

A Pythonic interface to the LabSound audio engine.
"""

from ._version import __version__

# Import core components
from .audio_context import AudioContext
from .audio_node import AudioNode
from .audio_param import AudioParam

# Import specific node types
from .nodes.oscillator_node import OscillatorNode
from .nodes.gain_node import GainNode
from .nodes.biquad_filter_node import BiquadFilterNode
from .nodes.buffer_source_node import AudioBufferSourceNode
from .nodes.analyzer_node import AnalyzerNode
from .nodes.function_node import FunctionNode
from .nodes.destination_node import AudioDestinationNode
from .nodes.channel_merger_node import ChannelMergerNode
from .nodes.channel_splitter_node import ChannelSplitterNode
from .nodes.constant_source_node import ConstantSourceNode
from .nodes.convolver_node import ConvolverNode
from .nodes.delay_node import DelayNode
from .nodes.dynamics_compressor_node import DynamicsCompressorNode
from .nodes.panner_node import PannerNode
from .nodes.stereo_panner_node import StereoPannerNode
from .nodes.wave_shaper_node import WaveShaperNode

# Expose key classes at the top level
__all__ = [
    'AudioContext',
    'AudioNode',
    'AudioParam',
    'OscillatorNode',
    'GainNode',
    'BiquadFilterNode',
    'AudioBufferSourceNode',
    'AnalyzerNode',
    'FunctionNode',
    'AudioDestinationNode',
    'ChannelMergerNode',
    'ChannelSplitterNode',
    'ConstantSourceNode',
    'ConvolverNode',
    'DelayNode',
    'DynamicsCompressorNode',
    'PannerNode',
    'StereoPannerNode',
    'WaveShaperNode',
]
