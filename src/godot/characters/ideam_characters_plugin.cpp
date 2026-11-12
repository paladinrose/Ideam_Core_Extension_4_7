#include "ideam_characters_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamCharactersPlugin *IdeamCharactersPlugin::singleton = nullptr;

void IdeamCharactersPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamCharactersPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_splines_handshake_available"), &IdeamCharactersPlugin::is_splines_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_design_handshake_available"), &IdeamCharactersPlugin::is_design_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_geometry_handshake_available"), &IdeamCharactersPlugin::is_geometry_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamCharactersPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamCharactersPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamCharactersPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamCharactersPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamCharactersPlugin::create_undo_redo_wrapper
    );
}

IdeamCharactersPlugin::IdeamCharactersPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamCharactersPlugin::~IdeamCharactersPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamCharactersPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamCharacters", true);
    register_to_ecosystem("WizardSettings", "IdeamCharacters", godot::String(CHARACTERS_SETTINGS_PATH.data()));
}

void IdeamCharactersPlugin::_exit_tree() {
    set_plugin_active("IdeamCharacters", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamCharactersPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamCharactersPlugin::create_undo_redo_wrapper() {
    godot::Ref<IdeamUndoRedo> ur;
    ur.instantiate();
#ifdef TOOLS_ENABLED
    if (godot::EditorUndoRedoManager *ed_ur = get_static_undo_redo()) {
        ur->set_editor_undo_redo(ed_ur);
    }
#endif
    return ur;
}

} // namespace ideam::godot_ext
