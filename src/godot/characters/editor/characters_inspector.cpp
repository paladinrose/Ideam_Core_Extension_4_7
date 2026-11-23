#include "characters_inspector.h"

#include "../character_gallery_bank_resource.h"
#include "../ideam_characters_plugin.h"
#include "../ui/character_modeler.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_registry.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

void CharactersInspector::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("_on_open_in_character_modeler", "resource"),
        &CharactersInspector::_on_open_in_character_modeler
    );
    ClassDB::bind_method(
        D_METHOD("_on_help_pressed", "topic_id"),
        &CharactersInspector::_on_help_pressed
    );
}

void CharactersInspector::_on_open_in_character_modeler(const godot::Ref<godot::Resource> &p_resource) {
    if (p_resource.is_valid()) {
        CharacterModeler::open_character_resource(p_resource);
    }
}

void CharactersInspector::_on_help_pressed(const godot::StringName &p_topic_id) {
    godot::Dictionary ctx;
    ctx["plugin"] = "IdeamCharacters";
    ToolRegistry::get_singleton().dispatch_help_request(p_topic_id, "characters.inspector.header", ctx);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *CharactersInspector::get_undo_redo() const {
    return IdeamCharactersPlugin::get_static_undo_redo();
}
#endif

bool CharactersInspector::_can_handle(godot::Object *p_object) const {
    if (!p_object) return false;
    return p_object->is_class("BodyPlanResource") ||
           p_object->is_class("CharacterInstanceResource") ||
           p_object->is_class("CharacterGalleryBankResource") ||
           p_object->is_class("LayeredCharacterNode3D") ||
           p_object->is_class("CharacterBlueprintResource") ||
           p_object->is_class("CharacterApparelResource") ||
           p_object->is_class("ParametricCharacterNode3D") ||
           p_object->is_class("DataOrientedCharacters");
}

void CharactersInspector::_parse_begin(godot::Object *p_object) {
    using namespace godot;
    if (!p_object) return;

    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    Ref<IdeamUndoRedo> ur = IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (auto *bp = Object::cast_to<BodyPlanResource>(p_object)) {
        bp->set_undo_redo(ur);
    } else if (auto *ci = Object::cast_to<CharacterInstanceResource>(p_object)) {
        ci->set_undo_redo(ur);
    }

    Ref<Resource> target_res;
    if (auto *direct_res = Object::cast_to<Resource>(p_object)) {
        target_res = Ref<Resource>(direct_res);
    } else if (auto *layered_node = Object::cast_to<LayeredCharacterNode3D>(p_object)) {
        target_res = layered_node->get_character_instance();
    } else if (auto *char_node = Object::cast_to<ParametricCharacterNode3D>(p_object)) {
        target_res = char_node->get_blueprint();
    } else if (auto *dod_char = Object::cast_to<DataOrientedCharacters>(p_object)) {
        target_res = dod_char->get_active_blueprint();
    }

    auto *bar = memnew(HBoxContainer);
    bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    domain_tool::stamp_ui_selector(bar, "characters.inspector.header");

    auto *lbl = memnew(Label);
    lbl->set_text(godot::String("IdeamCharacters (") + p_object->get_class() + godot::String(")"));
    lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bar->add_child(lbl);

    if (target_res.is_valid()) {
        auto *open_btn = memnew(Button);
        open_btn->set_text("Open in CharacterModeler");
        open_btn->set_tooltip_text("Open in the dual-mode CharacterModeler (Plan Authoring + Character Generator + Gallery Banks).");
        domain_tool::stamp_ui_selector(open_btn, "characters.inspector.open_modeler_btn");
        open_btn->connect("pressed", Callable(this, "_on_open_in_character_modeler").bind(target_res));
        bar->add_child(open_btn);
    }

    auto *help_btn = memnew(Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Open IdeamTutorials help for IdeamCharacters");
    domain_tool::stamp_ui_selector(help_btn, "characters.inspector.help_btn");
    help_btn->connect("pressed", Callable(this, "_on_help_pressed").bind(StringName(p_object->get_class())));
    bar->add_child(help_btn);

    add_custom_control(bar);
}

} // namespace ideam::godot_ext
