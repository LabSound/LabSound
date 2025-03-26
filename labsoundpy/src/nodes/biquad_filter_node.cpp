/**
 * Implementation file for BiquadFilterNode bindings.
 */

#include "nodes/biquad_filter_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/map.h>

void register_biquad_filter_node(nb::module_ &m) {
    // First, register the BiquadFilterType enum
    nb::enum_<lab::BiquadFilterType>(m, "BiquadFilterType")
        .value("LOWPASS", lab::BiquadFilterType::LOWPASS)
        .value("HIGHPASS", lab::BiquadFilterType::HIGHPASS)
        .value("BANDPASS", lab::BiquadFilterType::BANDPASS)
        .value("LOWSHELF", lab::BiquadFilterType::LOWSHELF)
        .value("HIGHSHELF", lab::BiquadFilterType::HIGHSHELF)
        .value("PEAKING", lab::BiquadFilterType::PEAKING)
        .value("NOTCH", lab::BiquadFilterType::NOTCH)
        .value("ALLPASS", lab::BiquadFilterType::ALLPASS);
    
    // Create a mapping from string to BiquadFilterType for Pythonic interface
    auto filter_type_from_string = [](const std::string& type_str) {
        static const std::map<std::string, lab::BiquadFilterType> type_map = {
            {"lowpass", lab::BiquadFilterType::LOWPASS},
            {"highpass", lab::BiquadFilterType::HIGHPASS},
            {"bandpass", lab::BiquadFilterType::BANDPASS},
            {"lowshelf", lab::BiquadFilterType::LOWSHELF},
            {"highshelf", lab::BiquadFilterType::HIGHSHELF},
            {"peaking", lab::BiquadFilterType::PEAKING},
            {"notch", lab::BiquadFilterType::NOTCH},
            {"allpass", lab::BiquadFilterType::ALLPASS}
        };
        
        auto it = type_map.find(type_str);
        if (it != type_map.end()) {
            return it->second;
        }
        throw nb::value_error("Invalid filter type: " + type_str);
    };
    
    auto filter_type_to_string = [](lab::BiquadFilterType type) {
        switch (type) {
            case lab::BiquadFilterType::LOWPASS: return "lowpass";
            case lab::BiquadFilterType::HIGHPASS: return "highpass";
            case lab::BiquadFilterType::BANDPASS: return "bandpass";
            case lab::BiquadFilterType::LOWSHELF: return "lowshelf";
            case lab::BiquadFilterType::HIGHSHELF: return "highshelf";
            case lab::BiquadFilterType::PEAKING: return "peaking";
            case lab::BiquadFilterType::NOTCH: return "notch";
            case lab::BiquadFilterType::ALLPASS: return "allpass";
            default: return "unknown";
        }
    };
    
    // Bind the BiquadFilterNode class
    nb::class_<lab::BiquadFilterNode, lab::AudioNode, std::shared_ptr<lab::BiquadFilterNode>>(m, "_BiquadFilterNode")
        .def("set_type", &lab::BiquadFilterNode::setType, nb::arg("type"))
        .def("type", &lab::BiquadFilterNode::type)
        .def("frequency", [](lab::BiquadFilterNode& node) {
            return node.frequency();
        })
        .def("q", [](lab::BiquadFilterNode& node) {
            return node.q();
        })
        .def("gain", [](lab::BiquadFilterNode& node) {
            return node.gain();
        })
        .def("detune", [](lab::BiquadFilterNode& node) {
            return node.detune();
        })
        // Add Pythonic interface for type
        .def("set_type_string", [filter_type_from_string](lab::BiquadFilterNode& node, const std::string& type_str) {
            node.setType(filter_type_from_string(type_str));
        }, nb::arg("type"))
        .def("type_string", [filter_type_to_string](lab::BiquadFilterNode& node) {
            return filter_type_to_string(node.type());
        });
}
