#include "spline_4d_resource_inspector.h"

#include "../spline_4d/color_spline_control.h"
#include "../spline_4d/spline_4d_control.h"
#include "../spline_4d/spline_4d_resource.h"

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace ideam::godot_ext {

void Spline4DResourceInspector::_bind_methods() {
}

Spline4DResourceInspector::Spline4DResourceInspector() = default;

Spline4DResourceInspector::~Spline4DResourceInspector() = default;

bool Spline4DResourceInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<Spline4DResource>(p_object) != nullptr;
}

godot::Control *Spline4DResourceInspector::_create_inspector_control(godot::Object *p_object) {
    Spline4DResource *spline_4d = godot::Object::cast_to<Spline4DResource>(p_object);
    if (!spline_4d) {
        return nullptr;
    }

    godot::Ref<Spline4DResource> ref_4d(spline_4d);

    // When `color_gradient_mode` is active on the resource, mount `ColorSplineControl`
    // directly above `Spline4DControl` (or as the primary interactive gradient editor).
    if (spline_4d->get_color_gradient_mode()) {
        godot::VBoxContainer *wrapper = memnew(godot::VBoxContainer);
        wrapper->add_theme_constant_override("separation", 8);

        ColorSplineControl *color_ctrl = memnew(ColorSplineControl);
        color_ctrl->set_spline_4d(ref_4d);
        wrapper->add_child(color_ctrl);

        Spline4DControl *spatial_ctrl = memnew(Spline4DControl);
        spatial_ctrl->set_spline_4d(ref_4d);
        wrapper->add_child(spatial_ctrl);

        return wrapper;
    }

    Spline4DControl *ctrl = memnew(Spline4DControl);
    ctrl->set_spline_4d(ref_4d);
    return ctrl;
}

} // namespace ideam::godot_ext