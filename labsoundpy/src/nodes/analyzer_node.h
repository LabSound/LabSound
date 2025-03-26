/**
 * Header file for AnalyzerNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the AnalyzerNode class with nanobind.
 */
void register_analyzer_node(nb::module_ &m);
