/**
 * Implementation file for ChannelSplitterNode bindings.
 */

#include "nodes/channel_splitter_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

void register_channel_splitter_node(nb::module_ &m) {
    // Bind the ChannelSplitterNode class
    nb::class_<lab::ChannelSplitterNode, lab::AudioNode, std::shared_ptr<lab::ChannelSplitterNode>>(m, "_ChannelSplitterNode")
        .def("add_outputs", &lab::ChannelSplitterNode::addOutputs, nb::arg("count"));
}
