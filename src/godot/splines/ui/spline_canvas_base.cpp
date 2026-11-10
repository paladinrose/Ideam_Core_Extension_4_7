#include "spline_canvas_base.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void SplineCanvasBase::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(DRAG_AXIS_FREE_2D);
    BIND_ENUM_CONSTANT(DRAG_AXIS_X_ONLY);
    BIND_ENUM_CONSTANT(DRAG_AXIS_Y_ONLY);

    BIND_ENUM_CONSTANT(POINT_STYLE_CIRCLE);
    BIND_ENUM_CONSTANT(POINT_STYLE_SQUARE);
    BIND_ENUM_CONSTANT(POINT_STYLE_FADER_CAP);

    ClassDB::bind_method(D_METHOD("set_spline_resource", "resource"), &SplineCanvasBase::set_spline_resource);
    ClassDB::bind_method(D_METHOD("get_spline_resource"), &SplineCanvasBase::get_spline_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline_resource", PROPERTY_HINT_RESOURCE_TYPE, "SplineResource"),
        "set_spline_resource",
        "get_spline_resource"
    );
}

void SplineCanvasBase::set_spline_resource(const godot::Ref<SplineResource> &p_resource) {
    spline_resource = p_resource;
    set_geometry_resource(p_resource);
}

int SplineCanvasBase::_get_canvas_point_count() const {
    return spline_resource.is_valid() ? spline_resource->get_point_count() : 0;
}

int SplineCanvasBase::_get_canvas_section_count() const {
    return spline_resource.is_valid() ? spline_resource->get_section_count() : 0;
}

bool SplineCanvasBase::_is_canvas_wrapped() const {
    return spline_resource.is_valid() && spline_resource->get_wrap();
}

bool SplineCanvasBase::_has_back_handle(int p_index) const {
    return spline_resource.is_valid() && spline_resource->get_point_use_back_handle(p_index);
}

bool SplineCanvasBase::_has_fore_handle(int p_index) const {
    return spline_resource.is_valid() && spline_resource->get_point_use_fore_handle(p_index);
}

void SplineCanvasBase::_on_right_click_point(int p_index) {
    if (spline_resource.is_valid()) {
        spline_resource->commit_remove_point(p_index);
    }
}

} // namespace ideam::godot_ext