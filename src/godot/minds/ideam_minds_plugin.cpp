#include "ideam_minds_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamMindsPlugin *IdeamMindsPlugin::singleton = nullptr;

void IdeamMindsPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamMindsPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_narratives_handshake_available"), &IdeamMindsPlugin::is_narratives_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_games_handshake_available"), &IdeamMindsPlugin::is_games_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_geometry_handshake_available"), &IdeamMindsPlugin::is_geometry_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tiles_handshake_available"), &IdeamMindsPlugin::is_tiles_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamMindsPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamMindsPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamMindsPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamMindsPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamMindsPlugin::create_undo_redo_wrapper
    );
}

IdeamMindsPlugin::IdeamMindsPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamMindsPlugin::~IdeamMindsPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamMindsPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamMinds", true);
    register_to_ecosystem("WizardSettings", "IdeamMinds", godot::String(MINDS_SETTINGS_PATH.data()));
}

void IdeamMindsPlugin::_exit_tree() {
    set_plugin_active("IdeamMinds", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamMindsPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamMindsPlugin::create_undo_redo_wrapper() {
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
