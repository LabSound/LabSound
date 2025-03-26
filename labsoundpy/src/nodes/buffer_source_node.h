/**
 * Header file for AudioBufferSourceNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the AudioBufferSourceNode class with nanobind.
 */
void register_buffer_source_node(nb::module_ &m);
