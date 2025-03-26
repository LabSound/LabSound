/**
 * Implementation file for DynamicsCompressorNode bindings.
 */

#include "nodes/dynamics_compressor_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

void register_dynamics_compressor_node(nb::module_ &m) {
    // Bind the DynamicsCompressorNode class
    nb::class_<lab::DynamicsCompressorNode, lab::AudioNode, std::shared_ptr<lab::DynamicsCompressorNode>>(m, "_DynamicsCompressorNode")
        .def("threshold", [](lab::DynamicsCompressorNode& node) {
            return node.threshold();
        })
        .def("knee", [](lab::DynamicsCompressorNode& node) {
            return node.knee();
        })
        .def("ratio", [](lab::DynamicsCompressorNode& node) {
            return node.ratio();
        })
        .def("attack", [](lab::DynamicsCompressorNode& node) {
            return node.attack();
        })
        .def("release", [](lab::DynamicsCompressorNode& node) {
            return node.release();
        })
        .def("reduction", [](lab::DynamicsCompressorNode& node) {
            return node.reduction();
        });
}
