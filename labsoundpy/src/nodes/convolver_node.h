/**
 * Header file for ConvolverNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the ConvolverNode class with nanobind.
 */
void register_convolver_node(nb::module_ &m);
