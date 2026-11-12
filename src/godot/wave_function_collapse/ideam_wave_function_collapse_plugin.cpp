#include "ideam_wave_function_collapse_plugin.h"
#include "../procedural_generation/ideam_procedural_generation_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamWaveFunctionCollapsePlugin *IdeamWaveFunctionCollapsePlugin::singleton = nullptr;

void IdeamWaveFunctionCollapsePlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamWaveFunctionCollapsePlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_procedural_generation_handshake_available"),
        &IdeamWaveFunctionCollapsePlugin::is_procedural_generation_handshake_available
    );
    godot::ClassDB::bind_method(godot::D_METHOD("is_tiles_handshake_available"), &IdeamWaveFunctionCollapsePlugin::is_tiles_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_design_handshake_available"), &IdeamWaveFunctionCollapsePlugin::is_design_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_narratives_handshake_available"), &IdeamWaveFunctionCollapsePlugin::is_narratives_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_games_handshake_available"), &IdeamWaveFunctionCollapsePlugin::is_games_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamWaveFunctionCollapsePlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamWaveFunctionCollapsePlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamWaveFunctionCollapsePlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamWaveFunctionCollapsePlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamWaveFunctionCollapsePlugin::create_undo_redo_wrapper
    );
}

IdeamWaveFunctionCollapsePlugin::IdeamWaveFunctionCollapsePlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamWaveFunctionCollapsePlugin::~IdeamWaveFunctionCollapsePlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamWaveFunctionCollapsePlugin::_enter_tree() {
    validate_script_templates();

    // Announce both canonical "IdeamWaveFunctionCollapse" and shorthand "IdeamWFC"
    set_plugin_active("IdeamWaveFunctionCollapse", true);
    set_plugin_active("IdeamWFC", true);
    register_to_ecosystem("WizardSettings", "IdeamWaveFunctionCollapse", godot::String(WFC_SETTINGS_PATH.data()));

    wfc_rule_tensor_inspector_.instantiate();
    add_inspector_plugin(wfc_rule_tensor_inspector_);
}

void IdeamWaveFunctionCollapsePlugin::_exit_tree() {
    if (wfc_rule_tensor_inspector_.is_valid()) {
        remove_inspector_plugin(wfc_rule_tensor_inspector_);
        wfc_rule_tensor_inspector_.unref();
    }
    set_plugin_active("IdeamWaveFunctionCollapse", false);
    set_plugin_active("IdeamWFC", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamWaveFunctionCollapsePlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    // Lean on IdeamProceduralGenerationPlugin if WFC's own EditorPlugin undo_redo is queried while delegated
    return IdeamProceduralGenerationPlugin::get_static_undo_redo();
}
#endif

godot::Ref<IdeamUndoRedo> IdeamWaveFunctionCollapsePlugin::create_undo_redo_wrapper() {
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
