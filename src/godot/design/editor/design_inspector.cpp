#include "design_inspector.h"
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

bool DesignInspector::_can_handle(godot::Object *p_object) const {
    if (!p_object) {
        return false;
    }
    return p_object->is_class("Style3DResource") ||
           p_object->is_class("Theme3DResource") ||
           p_object->is_class("ParametricAssemblyRecipeResource") ||
           p_object->is_class("ParametricAssemblyNode3D") ||
           p_object->is_class("BlueprintResource") ||
           p_object->is_class("Container3D") ||
           p_object->is_class("SemanticStructure3D");
}

void DesignInspector::_parse_begin(godot::Object *p_object) {
    if (!p_object) {
        return;
    }
    godot::Label *header = memnew(godot::Label);
    header->set_text("IdeamDesign — 3D CSS Theme, Layout & Parametric Assembly");
    add_custom_control(header);
}

void DesignInspector::_bind_methods() {
}

} // namespace ideam::godot_ext
