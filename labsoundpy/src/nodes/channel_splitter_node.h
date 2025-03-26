/**
 * Header file for ChannelSplitterNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the ChannelSplitterNode class with nanobind.
 */
void register_channel_splitter_node(nb::module_ &m);
