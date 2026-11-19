#include "characters_inspector.h"
#include "../ideam_characters_plugin.h"
#include "../ui/character_modeler.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void CharactersInspector::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("_on_open_in_character_modeler", "resource"),
        &CharactersInspector::_on_open_in_character_modeler
    );
}

void CharactersInspector::_on_open_in_character_modeler(const godot::Ref<godot::Resource> &p_resource) {
    if (p_resource.is_valid()) {
        CharacterModeler::open_character_resource(p_resource);
    }
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *CharactersInspector::get_undo_redo() const {
    return IdeamCharactersPlugin::get_static_undo_redo();
}
#endif

bool CharactersInspector::_can_handle(godot::Object *p_object) const {
    if (!p_object) return false;
    return p_object->is_class("CharacterBlueprintResource") ||
           p_object->is_class("CharacterApparelResource") ||
           p_object->is_class("ParametricCharacterNode3D") ||
           p_object->is_class("DataOrientedCharacters");
}

void CharactersInspector::_parse_begin(godot::Object *p_object) {
    using namespace godot;
    if (!p_object) return;

    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    Ref<Resource> target_res;
    if (auto *direct_bp = Object::cast_to<CharacterBlueprintResource>(p_object)) {
        target_res = Ref<Resource>(direct_bp);
    } else if (auto *direct_res = Object::cast_to<Resource>(p_object)) {
        target_res = Ref<Resource>(direct_res);
    } else if (auto *char_node = Object::cast_to<ParametricCharacterNode3D>(p_object)) {
        target_res = char_node->get_blueprint();
    } else if (auto *dod_char = Object::cast_to<DataOrientedCharacters>(p_object)) {
        target_res = dod_char->get_active_blueprint();
    }

    if (target_res.is_null()) {
        return;
    }

    auto *bar = memnew(HBoxContainer);
    bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    auto *lbl = memnew(Label);
    lbl->set_text("IdeamCharacters (" + target_res->get_class() + ")");
    lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bar->add_child(lbl);

    auto *open_btn = memnew(Button);
    open_btn->set_text("Open in CharacterModeler");
    open_btn->set_tooltip_text("Edit anatomical morph sliders and apparel in the Tier 2 CharacterModeler workspace.");
    open_btn->connect("pressed", Callable(this, "_on_open_in_character_modeler").bind(target_res));
    bar->add_child(open_btn);

    add_custom_control(bar);
}

} // namespace ideam::godot_ext
