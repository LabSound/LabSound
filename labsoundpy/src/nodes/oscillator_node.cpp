/**
 * Implementation file for OscillatorNode bindings.
 */

#include "nodes/oscillator_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/map.h>

void register_oscillator_node(nb::module_ &m) {
    // First, register the OscillatorType enum
    nb::enum_<lab::OscillatorType>(m, "OscillatorType")
        .value("SINE", lab::OscillatorType::SINE)
        .value("SQUARE", lab::OscillatorType::SQUARE)
        .value("SAWTOOTH", lab::OscillatorType::SAWTOOTH)
        .value("TRIANGLE", lab::OscillatorType::TRIANGLE)
        .value("CUSTOM", lab::OscillatorType::CUSTOM);
    
    // Create a mapping from string to OscillatorType for Pythonic interface
    auto oscillator_type_from_string = [](const std::string& type_str) {
        static const std::map<std::string, lab::OscillatorType> type_map = {
            {"sine", lab::OscillatorType::SINE},
            {"square", lab::OscillatorType::SQUARE},
            {"sawtooth", lab::OscillatorType::SAWTOOTH},
            {"triangle", lab::OscillatorType::TRIANGLE},
            {"custom", lab::OscillatorType::CUSTOM}
        };
        
        auto it = type_map.find(type_str);
        if (it != type_map.end()) {
            return it->second;
        }
        throw nb::value_error("Invalid oscillator type: " + type_str);
    };
    
    auto oscillator_type_to_string = [](lab::OscillatorType type) {
        switch (type) {
            case lab::OscillatorType::SINE: return "sine";
            case lab::OscillatorType::SQUARE: return "square";
            case lab::OscillatorType::SAWTOOTH: return "sawtooth";
            case lab::OscillatorType::TRIANGLE: return "triangle";
            case lab::OscillatorType::CUSTOM: return "custom";
            default: return "unknown";
        }
    };
    
    // Bind the OscillatorNode class
    nb::class_<lab::OscillatorNode, lab::AudioNode, std::shared_ptr<lab::OscillatorNode>>(m, "_OscillatorNode")
        .def("start", &lab::OscillatorNode::start, nb::arg("when") = 0.0)
        .def("stop", &lab::OscillatorNode::stop, nb::arg("when") = 0.0)
        .def("set_type", &lab::OscillatorNode::setType, nb::arg("type"))
        .def("type", &lab::OscillatorNode::type)
        .def("frequency", [](lab::OscillatorNode& node) {
            return node.frequency();
        })
        .def("detune", [](lab::OscillatorNode& node) {
            return node.detune();
        })
        // Add Pythonic interface for type
        .def("set_type_string", [oscillator_type_from_string](lab::OscillatorNode& node, const std::string& type_str) {
            node.setType(oscillator_type_from_string(type_str));
        }, nb::arg("type"))
        .def("type_string", [oscillator_type_to_string](lab::OscillatorNode& node) {
            return oscillator_type_to_string(node.type());
        });
}
