#include "style_3d_resource.h"
#include <algorithm>

namespace ideam::godot_ext {

void Style3DResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_semantic_role_id", "role_id"), &Style3DResource::set_semantic_role_id);
    ClassDB::bind_method(D_METHOD("get_semantic_role_id"), &Style3DResource::get_semantic_role_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "semantic_role_id"), "set_semantic_role_id", "get_semantic_role_id");

    ClassDB::bind_method(D_METHOD("set_scale_mode", "scale_mode"), &Style3DResource::set_scale_mode);
    ClassDB::bind_method(D_METHOD("get_scale_mode"), &Style3DResource::get_scale_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "scale_mode", PROPERTY_HINT_ENUM, "Uniform Fit,Stretch Middle,Repeat Tiles"),
        "set_scale_mode",
        "get_scale_mode"
    );

    ClassDB::bind_method(D_METHOD("set_edge_snap_policy", "policy"), &Style3DResource::set_edge_snap_policy);
    ClassDB::bind_method(D_METHOD("get_edge_snap_policy"), &Style3DResource::get_edge_snap_policy);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "edge_snap_policy", PROPERTY_HINT_ENUM, "None,Watertight,Miter Corner,Socket Anchor"),
        "set_edge_snap_policy",
        "get_edge_snap_policy"
    );

    ClassDB::bind_method(D_METHOD("set_mesh_asset", "mesh"), &Style3DResource::set_mesh_asset);
    ClassDB::bind_method(D_METHOD("get_mesh_asset"), &Style3DResource::get_mesh_asset);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "mesh_asset", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"),
        "set_mesh_asset",
        "get_mesh_asset"
    );
}

void Style3DResource::set_semantic_role_id(int p_role_id) {
    semantic_role_id_ = static_cast<uint32_t>(p_role_id);
    emit_changed();
}

void Style3DResource::set_scale_mode(int p_mode) {
    scale_mode_ = static_cast<core::DesignScaleMode>(std::clamp(p_mode, 0, 2));
    emit_changed();
}

void Style3DResource::set_edge_snap_policy(int p_policy) {
    edge_snap_policy_ = static_cast<core::EdgeSnapPolicy>(std::clamp(p_policy, 0, 3));
    emit_changed();
}

void Style3DResource::set_mesh_asset(const godot::Ref<godot::Mesh> &p_mesh) {
    mesh_asset_ = p_mesh;
    emit_changed();
}

void Theme3DResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_style_for_role", "semantic_role_id", "style"), &Theme3DResource::set_style_for_role);
    ClassDB::bind_method(D_METHOD("get_style_for_role", "semantic_role_id"), &Theme3DResource::get_style_for_role);
}

void Theme3DResource::set_style_for_role(int p_semantic_role_id, const godot::Ref<Style3DResource> &p_style) {
    role_to_style_map_[p_semantic_role_id] = p_style;
    emit_changed();
}

godot::Ref<Style3DResource> Theme3DResource::get_style_for_role(int p_semantic_role_id) const {
    if (role_to_style_map_.has(p_semantic_role_id)) {
        return role_to_style_map_[p_semantic_role_id];
    }
    return {};
}

} // namespace ideam::godot_ext
