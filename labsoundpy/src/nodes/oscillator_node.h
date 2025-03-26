/**
 * Header file for OscillatorNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the OscillatorNode class with nanobind.
 */
void register_oscillator_node(nb::module_ &m);
