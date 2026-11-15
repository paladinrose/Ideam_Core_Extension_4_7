#include "characters_inspector.h"
#include "../ideam_characters_plugin.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void CharactersInspector::_bind_methods() {}

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
    if (!p_object) return;

    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    godot::PanelContainer *panel = memnew(godot::PanelContainer);
    godot::Ref<godot::StyleBoxFlat> style;
    style.instantiate();
    style->set_bg_color(godot::Color(0.11f, 0.13f, 0.16f, 0.95f));
    style->set_border_color(godot::Color(0.32f, 0.50f, 0.68f, 0.85f));
    style->set_border_width_all(1);
    style->set_corner_radius_all(5);
    style->set_content_margin_all(8);
    panel->add_theme_stylebox_override("panel", style);

    godot::VBoxContainer *vbox = memnew(godot::VBoxContainer);
    vbox->add_theme_constant_override("separation", 6);
    panel->add_child(vbox);

    godot::Label *header = memnew(godot::Label);
    header->set_text("IdeamCharacters — Quad-Loop Topology, Anatomical Morph Sliders & Garment Refitting");
    header->add_theme_color_override("font_color", godot::Color(0.70f, 0.90f, 1.0f));
    vbox->add_child(header);

    godot::Ref<CharacterBlueprintResource> bp;
    if (auto *direct_bp = godot::Object::cast_to<CharacterBlueprintResource>(p_object)) {
        bp = godot::Ref<CharacterBlueprintResource>(direct_bp);
    } else if (auto *char_node = godot::Object::cast_to<ParametricCharacterNode3D>(p_object)) {
        bp = char_node->get_blueprint();
    } else if (auto *dod_char = godot::Object::cast_to<DataOrientedCharacters>(p_object)) {
        bp = dod_char->get_active_blueprint();
    }

    if (bp.is_valid()) {
        CharacterMorphSliderPanel *slider_panel = memnew(CharacterMorphSliderPanel);
        slider_panel->set_undo_redo(IdeamCharactersPlugin::create_undo_redo_wrapper());
        slider_panel->set_blueprint(bp);
        vbox->add_child(slider_panel);
    }

    add_custom_control(panel);
}

} // namespace ideam::godot_ext
