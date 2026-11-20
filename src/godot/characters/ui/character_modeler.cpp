#include "character_modeler.h"

#include "../ideam_characters_plugin.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

CharacterModeler *CharacterModeler::shared_instance_ = nullptr;
Window *CharacterModeler::shared_window_             = nullptr;

void CharacterModeler::_bind_methods() {
    ClassDB::bind_static_method(
        "CharacterModeler",
        D_METHOD("open_character_resource", "resource", "owner"),
        &CharacterModeler::open_character_resource,
        DEFVAL(nullptr)
    );
}

CharacterModeler::CharacterModeler() {
    set_workspace_id(StringName(WORKSPACE_ID));

    ensure_sidebar_tab("Blueprints");
    ensure_sidebar_tab("Apparel");

    configure_theme_registry("res://addons/ideam_characters/resources/character_modeler_theme_registry.tres");

#ifdef TOOLS_ENABLED
    if (tool_context_.is_valid()) {
        if (EditorUndoRedoManager *ed_ur = IdeamCharactersPlugin::get_static_undo_redo()) {
            tool_context_->set_editor_undo_redo(ed_ur);
        }
    }
#endif
}

CharacterModeler::~CharacterModeler() {
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
    }
}

CharacterModeler *CharacterModeler::get_or_create_shared_modeler(Control *p_owner) {
    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            if (auto *existing = Object::cast_to<CharacterModeler>(p_owner->get_child(i))) {
                return existing;
            }
        }
        auto *embedded = memnew(CharacterModeler);
        p_owner->add_child(embedded);
        return embedded;
    }

    if (shared_instance_ && shared_window_) {
        shared_window_->show();
        shared_window_->grab_focus();
        return shared_instance_;
    }

    shared_instance_ = memnew(CharacterModeler);
    shared_window_   = ToolWorkspace::create_workspace_window(
        shared_instance_,
        "Ideam CharacterModeler",
        Vector2i(1080, 700)
    );
    return shared_instance_;
}

void CharacterModeler::open_character_resource(const Ref<Resource> &p_resource, Control *p_owner) {
    if (p_resource.is_null()) {
        return;
    }
    if (auto *modeler = get_or_create_shared_modeler(p_owner)) {
        modeler->open_asset(p_resource);
    }
}

} // namespace ideam::godot_ext
