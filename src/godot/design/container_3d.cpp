#include "container_3d.h"
#include <cmath>

namespace ideam::godot_ext {

void Container3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_cell_spacing", "spacing"), &Container3D::set_cell_spacing);
    ClassDB::bind_method(D_METHOD("get_cell_spacing"), &Container3D::get_cell_spacing);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "cell_spacing"), "set_cell_spacing", "get_cell_spacing");

    ClassDB::bind_method(D_METHOD("set_auto_refit_children", "auto_refit"), &Container3D::set_auto_refit_children);
    ClassDB::bind_method(D_METHOD("get_auto_refit_children"), &Container3D::get_auto_refit_children);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_refit_children"), "set_auto_refit_children", "get_auto_refit_children");

    ClassDB::bind_method(D_METHOD("refit_children_3d"), &Container3D::refit_children_3d);
}

void Container3D::set_cell_spacing(const godot::Vector3 &p_spacing) {
    cell_spacing_ = p_spacing;
    if (auto_refit_children_) {
        refit_children_3d();
    }
}

void Container3D::set_auto_refit_children(bool p_auto) {
    auto_refit_children_ = p_auto;
}

void Container3D::refit_children_3d() {
    const int n = get_child_count();
    for (int i = 0; i < n; ++i) {
        if (auto *child = godot::Object::cast_to<godot::Node3D>(get_child(i))) {
            child->set_position(cell_spacing_ * static_cast<float>(i));
        }
    }
}

void RadialContainer3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_radius", "radius"), &RadialContainer3D::set_radius);
    ClassDB::bind_method(D_METHOD("get_radius"), &RadialContainer3D::get_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "0.0,500.0,0.05"), "set_radius", "get_radius");
}

void RadialContainer3D::set_radius(real_t p_radius) {
    radius_ = p_radius;
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void RadialContainer3D::refit_children_3d() {
    const int n = get_child_count();
    if (n <= 0) {
        return;
    }
    for (int i = 0; i < n; ++i) {
        if (auto *child = godot::Object::cast_to<godot::Node3D>(get_child(i))) {
            const float angle = (Math_TAU * static_cast<float>(i)) / static_cast<float>(n);
            child->set_position(godot::Vector3(std::cos(angle) * static_cast<float>(radius_), 0.0f, std::sin(angle) * static_cast<float>(radius_)));
        }
    }
}

void SemanticStructure3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_semantic_role_id", "role_id"), &SemanticStructure3D::set_semantic_role_id);
    ClassDB::bind_method(D_METHOD("get_semantic_role_id"), &SemanticStructure3D::get_semantic_role_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "semantic_role_id"), "set_semantic_role_id", "get_semantic_role_id");

    ClassDB::bind_method(D_METHOD("set_active_theme_3d", "theme"), &SemanticStructure3D::set_active_theme_3d);
    ClassDB::bind_method(D_METHOD("get_active_theme_3d"), &SemanticStructure3D::get_active_theme_3d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "active_theme_3d", PROPERTY_HINT_RESOURCE_TYPE, "Theme3DResource"),
        "set_active_theme_3d",
        "get_active_theme_3d"
    );
}

void SemanticStructure3D::set_semantic_role_id(int p_role_id) {
    semantic_role_id_ = static_cast<uint32_t>(p_role_id);
}

void SemanticStructure3D::set_active_theme_3d(const godot::Ref<Theme3DResource> &p_theme) {
    active_theme_3d_ = p_theme;
}

} // namespace ideam::godot_ext
