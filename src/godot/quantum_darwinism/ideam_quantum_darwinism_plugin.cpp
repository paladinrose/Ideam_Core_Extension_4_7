#include "ideam_quantum_darwinism_plugin.h"
#include "../procedural_generation/ideam_procedural_generation_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamQuantumDarwinismPlugin *IdeamQuantumDarwinismPlugin::singleton = nullptr;

void IdeamQuantumDarwinismPlugin::_bind_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_plugin_domain_name"),
        &IdeamQuantumDarwinismPlugin::get_plugin_domain_name
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_procedural_generation_handshake_available"),
        &IdeamQuantumDarwinismPlugin::is_procedural_generation_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_wfc_handshake_available"),
        &IdeamQuantumDarwinismPlugin::is_wfc_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_tiles_handshake_available"),
        &IdeamQuantumDarwinismPlugin::is_tiles_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_minds_handshake_available"),
        &IdeamQuantumDarwinismPlugin::is_minds_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_narratives_handshake_available"),
        &IdeamQuantumDarwinismPlugin::is_narratives_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_games_handshake_available"),
        &IdeamQuantumDarwinismPlugin::is_games_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_tasks_handshake_available"),
        &IdeamQuantumDarwinismPlugin::is_tasks_handshake_available
    );

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamQuantumDarwinismPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamQuantumDarwinismPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamQuantumDarwinismPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper
    );
}

IdeamQuantumDarwinismPlugin::IdeamQuantumDarwinismPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamQuantumDarwinismPlugin::~IdeamQuantumDarwinismPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamQuantumDarwinismPlugin::_enter_tree() {
    validate_script_templates();

    set_plugin_active("IdeamQuantumDarwinism", true);
    set_plugin_active("IdeamQD", true);
    register_to_ecosystem(
        "WizardSettings",
        "IdeamQuantumDarwinism",
        godot::String(QD_SETTINGS_PATH.data())
    );

    qd_inspector_.instantiate();
    add_inspector_plugin(qd_inspector_);
}

void IdeamQuantumDarwinismPlugin::_exit_tree() {
    if (qd_inspector_.is_valid()) {
        remove_inspector_plugin(qd_inspector_);
        qd_inspector_.unref();
    }
    set_plugin_active("IdeamQuantumDarwinism", false);
    set_plugin_active("IdeamQD", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamQuantumDarwinismPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return IdeamProceduralGenerationPlugin::get_static_undo_redo();
}
#endif

godot::Ref<IdeamUndoRedo> IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper() {
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
