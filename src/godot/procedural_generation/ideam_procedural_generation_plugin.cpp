#include "ideam_procedural_generation_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamProceduralGenerationPlugin *IdeamProceduralGenerationPlugin::singleton = nullptr;

void IdeamProceduralGenerationPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamProceduralGenerationPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_geometry_handshake_available"), &IdeamProceduralGenerationPlugin::is_geometry_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_splines_handshake_available"), &IdeamProceduralGenerationPlugin::is_splines_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tiles_handshake_available"), &IdeamProceduralGenerationPlugin::is_tiles_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_design_handshake_available"), &IdeamProceduralGenerationPlugin::is_design_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_wfc_handshake_available"), &IdeamProceduralGenerationPlugin::is_wfc_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamProceduralGenerationPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamProceduralGenerationPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamProceduralGenerationPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamProceduralGenerationPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamProceduralGenerationPlugin::create_undo_redo_wrapper
    );
}

IdeamProceduralGenerationPlugin::IdeamProceduralGenerationPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamProceduralGenerationPlugin::~IdeamProceduralGenerationPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamProceduralGenerationPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamProceduralGeneration", true);
    register_to_ecosystem("WizardSettings", "IdeamProceduralGeneration", godot::String(PROCGEN_SETTINGS_PATH.data()));
}

void IdeamProceduralGenerationPlugin::_exit_tree() {
    set_plugin_active("IdeamProceduralGeneration", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamProceduralGenerationPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamProceduralGenerationPlugin::create_undo_redo_wrapper() {
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
