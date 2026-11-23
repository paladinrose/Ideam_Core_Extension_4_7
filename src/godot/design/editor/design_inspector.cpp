#include "design_inspector.h"

#include "../ideam_design_plugin.h"
#include "../ui/design_composer.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_registry.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

void DesignInspector::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("_on_open_in_design_composer", "resource"),
        &DesignInspector::_on_open_in_design_composer
    );
    ClassDB::bind_method(
        D_METHOD("_on_help_pressed", "topic_id"),
        &DesignInspector::_on_help_pressed
    );
}

void DesignInspector::_on_open_in_design_composer(const godot::Ref<godot::Resource> &p_resource) {
    if (p_resource.is_valid()) {
        DesignComposer::open_design_resource(p_resource);
    }
}

void DesignInspector::_on_help_pressed(const godot::StringName &p_topic_id) {
    godot::Dictionary ctx;
    ctx["plugin"] = "IdeamDesign";
    ToolRegistry::get_singleton().dispatch_help_request(p_topic_id, "design.inspector.header", ctx);
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
           p_object->is_class("FormScaffoldResource") ||
           p_object->is_class("ProportionRulerGraphResource") ||
           p_object->is_class("FormScaffoldNode3D") ||
           p_object->is_class("Container3D") ||
           p_object->is_class("SemanticStructure3D");
}

void DesignInspector::_parse_begin(godot::Object *p_object) {
    using namespace godot;
    if (!p_object) {
        return;
    }

    // Inject IdeamUndoRedo into scaffold & ruler resources
    Ref<IdeamUndoRedo> ur = IdeamDesignPlugin::create_undo_redo_wrapper();
    if (auto *sc = Object::cast_to<FormScaffoldResource>(p_object)) {
        sc->set_undo_redo(ur);
    } else if (auto *rg = Object::cast_to<ProportionRulerGraphResource>(p_object)) {
        rg->set_undo_redo(ur);
    }

    auto *bar = memnew(HBoxContainer);
    bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    domain_tool::stamp_ui_selector(bar, "design.inspector.header");

    auto *header = memnew(Label);
    header->set_text(godot::String("IdeamDesign — ") + p_object->get_class());
    header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bar->add_child(header);

    Ref<Resource> open_target;
    if (auto *res = Object::cast_to<Resource>(p_object)) {
        open_target = Ref<Resource>(res);
    } else if (auto *node = Object::cast_to<FormScaffoldNode3D>(p_object)) {
        open_target = node->get_scaffold();
    }

    if (open_target.is_valid()) {
        auto *open_btn = memnew(Button);
        open_btn->set_text("Open in DesignComposer");
        open_btn->set_tooltip_text("Author blueprints, Loomis scaffolds, proportion rulers, and 3D themes in DesignComposer.");
        domain_tool::stamp_ui_selector(open_btn, "design.inspector.open_composer_btn");
        open_btn->connect("pressed", Callable(this, "_on_open_in_design_composer").bind(open_target));
        bar->add_child(open_btn);
    }

    auto *help_btn = memnew(Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Open IdeamTutorials help for IdeamDesign");
    domain_tool::stamp_ui_selector(help_btn, "design.inspector.help_btn");
    help_btn->connect("pressed", Callable(this, "_on_help_pressed").bind(StringName(p_object->get_class())));
    bar->add_child(help_btn);

    add_custom_control(bar);
}

} // namespace ideam::godot_ext
