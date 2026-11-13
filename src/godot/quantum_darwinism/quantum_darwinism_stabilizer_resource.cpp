#include "quantum_darwinism_stabilizer_resource.h"
#include <algorithm>

namespace ideam::godot_ext {

void QuantumDarwinismStabilizerResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_max_horizon_radius", "radius"), &QuantumDarwinismStabilizerResource::set_max_horizon_radius);
    ClassDB::bind_method(D_METHOD("get_max_horizon_radius"), &QuantumDarwinismStabilizerResource::get_max_horizon_radius);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "max_horizon_radius", PROPERTY_HINT_RANGE, "1,16,1"),
        "set_max_horizon_radius",
        "get_max_horizon_radius"
    );

    ClassDB::bind_method(
        D_METHOD("set_pointer_redundancy_threshold", "threshold"),
        &QuantumDarwinismStabilizerResource::set_pointer_redundancy_threshold
    );
    ClassDB::bind_method(
        D_METHOD("get_pointer_redundancy_threshold"),
        &QuantumDarwinismStabilizerResource::get_pointer_redundancy_threshold
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "pointer_redundancy_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_pointer_redundancy_threshold",
        "get_pointer_redundancy_threshold"
    );

    ClassDB::bind_method(
        D_METHOD("set_fragile_redundancy_threshold", "threshold"),
        &QuantumDarwinismStabilizerResource::set_fragile_redundancy_threshold
    );
    ClassDB::bind_method(
        D_METHOD("get_fragile_redundancy_threshold"),
        &QuantumDarwinismStabilizerResource::get_fragile_redundancy_threshold
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "fragile_redundancy_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_fragile_redundancy_threshold",
        "get_fragile_redundancy_threshold"
    );

    ClassDB::bind_method(D_METHOD("set_pattern", "pattern"), &QuantumDarwinismStabilizerResource::set_pattern);
    ClassDB::bind_method(D_METHOD("get_pattern"), &QuantumDarwinismStabilizerResource::get_pattern);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "pattern", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralPattern"),
        "set_pattern",
        "get_pattern"
    );
}

void QuantumDarwinismStabilizerResource::set_max_horizon_radius(int p_radius) {
    max_horizon_radius_ = std::clamp(p_radius, 1, 16);
    emit_changed();
}

void QuantumDarwinismStabilizerResource::set_pointer_redundancy_threshold(float p_thresh) {
    pointer_redundancy_thresh_ = std::clamp(p_thresh, 0.0f, 1.0f);
    emit_changed();
}

void QuantumDarwinismStabilizerResource::set_fragile_redundancy_threshold(float p_thresh) {
    fragile_redundancy_thresh_ = std::clamp(p_thresh, 0.0f, 1.0f);
    emit_changed();
}

void QuantumDarwinismStabilizerNode::_bind_methods() {}

} // namespace ideam::godot_ext
