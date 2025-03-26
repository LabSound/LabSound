/**
 * Main binding file for LabSound Python bindings.
 * This file defines the module and includes the other binding files.
 */

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

// Include LabSound headers
#include "LabSound/LabSound.h"

// Include binding headers
#include "audio_context.h"
#include "audio_node.h"
#include "audio_param.h"
#include "nodes/oscillator_node.h"
#include "nodes/gain_node.h"
#include "nodes/biquad_filter_node.h"
#include "nodes/buffer_source_node.h"
#include "nodes/analyzer_node.h"
#include "nodes/function_node.h"
#include "nodes/destination_node.h"
#include "nodes/channel_merger_node.h"
#include "nodes/channel_splitter_node.h"
#include "nodes/constant_source_node.h"
#include "nodes/convolver_node.h"
#include "nodes/delay_node.h"
#include "nodes/dynamics_compressor_node.h"
#include "nodes/panner_node.h"
#include "nodes/stereo_panner_node.h"
#include "nodes/wave_shaper_node.h"

namespace nb = nanobind;

NB_MODULE(_core, m) {
    m.doc() = "LabSound Python bindings";
    
    // Register the AudioContext class
    register_audio_context(m);
    
    // Register the AudioNode base class
    register_audio_node(m);
    
    // Register the AudioParam class
    register_audio_param(m);
    
    // Register specific node types
    register_oscillator_node(m);
    register_gain_node(m);
    register_biquad_filter_node(m);
    register_buffer_source_node(m);
    register_analyzer_node(m);
    register_function_node(m);
    register_destination_node(m);
    register_channel_merger_node(m);
    register_channel_splitter_node(m);
    register_constant_source_node(m);
    register_convolver_node(m);
    register_delay_node(m);
    register_dynamics_compressor_node(m);
    register_panner_node(m);
    register_stereo_panner_node(m);
    register_wave_shaper_node(m);
    
    // Add version information
    m.attr("__version__") = "0.1.0";
}
