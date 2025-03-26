/**
 * Header file for GainNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the GainNode class with nanobind.
 */
void register_gain_node(nb::module_ &m);
