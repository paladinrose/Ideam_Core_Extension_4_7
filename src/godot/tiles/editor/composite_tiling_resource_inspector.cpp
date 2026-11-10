#include "composite_tiling_resource_inspector.h"

#include "../composite_tiling_resource.h"
#include "../ui/composite_tiling_control.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace ideam::godot_ext {

void CompositeTilingResourceInspector::_bind_methods() {
}

CompositeTilingResourceInspector::CompositeTilingResourceInspector() = default;

CompositeTilingResourceInspector::~CompositeTilingResourceInspector() = default;

bool CompositeTilingResourceInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) return false;
    return godot::Object::cast_to<CompositeTilingResource>(p_object) != nullptr;
}

godot::Control *CompositeTilingResourceInspector::_create_inspector_control(godot::Object *p_object) {
    CompositeTilingResource *comp = godot::Object::cast_to<CompositeTilingResource>(p_object);
    if (!comp) return nullptr;

    CompositeTilingControl *ctrl = memnew(CompositeTilingControl);
    ctrl->set_composite_tiling(godot::Ref<CompositeTilingResource>(comp));
    return ctrl;
}

} // namespace ideam::godot_ext
