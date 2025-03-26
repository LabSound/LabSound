/**
 * Implementation file for ConvolverNode bindings.
 */

#include "nodes/convolver_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

void register_convolver_node(nb::module_ &m) {
    // Bind the ConvolverNode class
    nb::class_<lab::ConvolverNode, lab::AudioScheduledSourceNode, std::shared_ptr<lab::ConvolverNode>>(m, "_ConvolverNode")
        .def("set_normalize", &lab::ConvolverNode::setNormalize, nb::arg("normalize"))
        .def("normalize", &lab::ConvolverNode::normalize)
        .def("set_impulse", [](lab::ConvolverNode& node, std::shared_ptr<lab::AudioBus> impulse) {
            node.setImpulse(impulse);
        }, nb::arg("impulse"))
        .def("get_impulse", [](lab::ConvolverNode& node) {
            return node.getImpulse();
        });
}
