/**
 * Header file for FunctionNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the FunctionNode class with nanobind.
 */
void register_function_node(nb::module_ &m);
