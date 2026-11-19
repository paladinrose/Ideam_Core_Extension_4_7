#include "design_inspector.h"
#include "../ui/design_composer.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void DesignInspector::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("_on_open_in_design_composer", "resource"),
        &DesignInspector::_on_open_in_design_composer
    );
}

void DesignInspector::_on_open_in_design_composer(const godot::Ref<godot::Resource> &p_resource) {
    if (p_resource.is_valid()) {
        DesignComposer::open_design_resource(p_resource);
    }
}

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
    using namespace godot;
    if (!p_object) {
        return;
    }

    auto *bar = memnew(HBoxContainer);
    bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    auto *header = memnew(Label);
    header->set_text("IdeamDesign — " + p_object->get_class());
    header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bar->add_child(header);

    if (auto *res = Object::cast_to<Resource>(p_object)) {
        Ref<Resource> res_ref(res);
        auto *open_btn = memnew(Button);
        open_btn->set_text("Open in DesignComposer");
        open_btn->set_tooltip_text("Author blueprints, recipes, and 3D themes in the Tier 2 DesignComposer workspace.");
        open_btn->connect("pressed", Callable(this, "_on_open_in_design_composer").bind(res_ref));
        bar->add_child(open_btn);
    }

    add_custom_control(bar);
}

} // namespace ideam::godot_ext
