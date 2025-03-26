/**
 * Header file for DynamicsCompressorNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the DynamicsCompressorNode class with nanobind.
 */
void register_dynamics_compressor_node(nb::module_ &m);
