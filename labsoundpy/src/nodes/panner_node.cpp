/**
 * Implementation file for PannerNode bindings.
 */

#include "nodes/panner_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/map.h>

void register_panner_node(nb::module_ &m) {
    // First, register the PanningModel enum
    nb::enum_<lab::PanningModel>(m, "PanningModel")
        .value("EQUALPOWER", lab::PanningModel::EQUALPOWER)
        .value("HRTF", lab::PanningModel::HRTF);
    
    // Register the DistanceModel enum
    nb::enum_<lab::DistanceModel>(m, "DistanceModel")
        .value("LINEAR", lab::DistanceModel::LINEAR)
        .value("INVERSE", lab::DistanceModel::INVERSE)
        .value("EXPONENTIAL", lab::DistanceModel::EXPONENTIAL);
    
    // Create a mapping from string to PanningModel for Pythonic interface
    auto panning_model_from_string = [](const std::string& model_str) {
        static const std::map<std::string, lab::PanningModel> model_map = {
            {"equalpower", lab::PanningModel::EQUALPOWER},
            {"HRTF", lab::PanningModel::HRTF}
        };
        
        auto it = model_map.find(model_str);
        if (it != model_map.end()) {
            return it->second;
        }
        throw nb::value_error("Invalid panning model: " + model_str);
    };
    
    auto panning_model_to_string = [](lab::PanningModel model) {
        switch (model) {
            case lab::PanningModel::EQUALPOWER: return "equalpower";
            case lab::PanningModel::HRTF: return "HRTF";
            default: return "unknown";
        }
    };
    
    // Create a mapping from string to DistanceModel for Pythonic interface
    auto distance_model_from_string = [](const std::string& model_str) {
        static const std::map<std::string, lab::DistanceModel> model_map = {
            {"linear", lab::DistanceModel::LINEAR},
            {"inverse", lab::DistanceModel::INVERSE},
            {"exponential", lab::DistanceModel::EXPONENTIAL}
        };
        
        auto it = model_map.find(model_str);
        if (it != model_map.end()) {
            return it->second;
        }
        throw nb::value_error("Invalid distance model: " + model_str);
    };
    
    auto distance_model_to_string = [](lab::DistanceModel model) {
        switch (model) {
            case lab::DistanceModel::LINEAR: return "linear";
            case lab::DistanceModel::INVERSE: return "inverse";
            case lab::DistanceModel::EXPONENTIAL: return "exponential";
            default: return "unknown";
        }
    };
    
    // Bind the PannerNode class
    nb::class_<lab::PannerNode, lab::AudioNode, std::shared_ptr<lab::PannerNode>>(m, "_PannerNode")
        .def("set_position", &lab::PannerNode::setPosition, nb::arg("x"), nb::arg("y"), nb::arg("z"))
        .def("position_x", [](lab::PannerNode& node) { return node.positionX(); })
        .def("position_y", [](lab::PannerNode& node) { return node.positionY(); })
        .def("position_z", [](lab::PannerNode& node) { return node.positionZ(); })
        .def("set_orientation", &lab::PannerNode::setOrientation, nb::arg("x"), nb::arg("y"), nb::arg("z"))
        .def("orientation_x", [](lab::PannerNode& node) { return node.orientationX(); })
        .def("orientation_y", [](lab::PannerNode& node) { return node.orientationY(); })
        .def("orientation_z", [](lab::PannerNode& node) { return node.orientationZ(); })
        .def("set_velocity", &lab::PannerNode::setVelocity, nb::arg("x"), nb::arg("y"), nb::arg("z"))
        .def("velocity_x", [](lab::PannerNode& node) { return node.velocityX(); })
        .def("velocity_y", [](lab::PannerNode& node) { return node.velocityY(); })
        .def("velocity_z", [](lab::PannerNode& node) { return node.velocityZ(); })
        .def("distance_gain", [](lab::PannerNode& node) { return node.distanceGain(); })
        .def("cone_gain", [](lab::PannerNode& node) { return node.coneGain(); })
        .def("set_panning_model", &lab::PannerNode::setPanningModel, nb::arg("model"))
        .def("panning_model", &lab::PannerNode::panningModel)
        .def("set_distance_model", &lab::PannerNode::setDistanceModel, nb::arg("model"))
        .def("distance_model", &lab::PannerNode::distanceModel)
        .def("set_ref_distance", &lab::PannerNode::setRefDistance, nb::arg("distance"))
        .def("ref_distance", &lab::PannerNode::refDistance)
        .def("set_max_distance", &lab::PannerNode::setMaxDistance, nb::arg("distance"))
        .def("max_distance", &lab::PannerNode::maxDistance)
        .def("set_rolloff_factor", &lab::PannerNode::setRolloffFactor, nb::arg("factor"))
        .def("rolloff_factor", &lab::PannerNode::rolloffFactor)
        .def("set_cone_inner_angle", &lab::PannerNode::setConeInnerAngle, nb::arg("angle"))
        .def("cone_inner_angle", &lab::PannerNode::coneInnerAngle)
        .def("set_cone_outer_angle", &lab::PannerNode::setConeOuterAngle, nb::arg("angle"))
        .def("cone_outer_angle", &lab::PannerNode::coneOuterAngle)
        .def("set_cone_outer_gain", &lab::PannerNode::setConeOuterGain, nb::arg("gain"))
        .def("cone_outer_gain", &lab::PannerNode::coneOuterGain)
        // Add Pythonic interface for models
        .def("set_panning_model_string", [panning_model_from_string](lab::PannerNode& node, const std::string& model_str) {
            node.setPanningModel(panning_model_from_string(model_str));
        }, nb::arg("model"))
        .def("panning_model_string", [panning_model_to_string](lab::PannerNode& node) {
            return panning_model_to_string(node.panningModel());
        })
        .def("set_distance_model_string", [distance_model_from_string](lab::PannerNode& node, const std::string& model_str) {
            node.setDistanceModel(distance_model_from_string(model_str));
        }, nb::arg("model"))
        .def("distance_model_string", [distance_model_to_string](lab::PannerNode& node) {
            return distance_model_to_string(node.distanceModel());
        });
}
