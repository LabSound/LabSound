/**
 * Header file for ConstantSourceNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the ConstantSourceNode class with nanobind.
 */
void register_constant_source_node(nb::module_ &m);
