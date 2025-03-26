/**
 * Implementation file for FunctionNode bindings.
 */

#include "nodes/function_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/function.h>
#include <nanobind/ndarray.h>

void register_function_node(nb::module_ &m) {
    // Bind the FunctionNode class
    nb::class_<lab::FunctionNode, lab::AudioScheduledSourceNode, std::shared_ptr<lab::FunctionNode>>(m, "_FunctionNode")
        .def("set_process_function", [](lab::FunctionNode& node, nb::function process_function) {
            // Create a lambda that will call the Python function
            node.setFunction([process_function](lab::ContextRenderLock& r, lab::FunctionNode* node, int channel, float* buffer, size_t frames) {
                // Convert the buffer to a numpy array
                auto array = nb::ndarray<nb::numpy, float, nb::shape<nb::any>>(
                    buffer, {frames}, {sizeof(float)}
                );
                
                // Call the Python function with the channel and buffer
                process_function(channel, array);
                
                // The buffer is modified in-place by the Python function
            });
        }, nb::arg("process_function"))
        .def("start", &lab::FunctionNode::start, nb::arg("when") = 0.0f)
        .def("stop", &lab::FunctionNode::stop, nb::arg("when") = 0.0f);
}
