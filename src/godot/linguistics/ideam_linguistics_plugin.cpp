#include "ideam_linguistics_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamLinguisticsPlugin *IdeamLinguisticsPlugin::singleton = nullptr;

void IdeamLinguisticsPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamLinguisticsPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_communications_handshake_available"), &IdeamLinguisticsPlugin::is_communications_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_minds_handshake_available"), &IdeamLinguisticsPlugin::is_minds_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_narratives_handshake_available"), &IdeamLinguisticsPlugin::is_narratives_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_procedural_generation_handshake_available"), &IdeamLinguisticsPlugin::is_procedural_generation_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_games_handshake_available"), &IdeamLinguisticsPlugin::is_games_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamLinguisticsPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamLinguisticsPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamLinguisticsPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamLinguisticsPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamLinguisticsPlugin::create_undo_redo_wrapper
    );
}

IdeamLinguisticsPlugin::IdeamLinguisticsPlugin() {
    if (singleton == nullptr) singleton = this;
}

IdeamLinguisticsPlugin::~IdeamLinguisticsPlugin() {
    if (singleton == this) singleton = nullptr;
}

void IdeamLinguisticsPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamLinguistics", true);
    register_to_ecosystem("WizardSettings", "IdeamLinguistics", godot::String(LINGUISTICS_SETTINGS_PATH.data()));

    linguistics_inspector_.instantiate();
    add_inspector_plugin(linguistics_inspector_);
}

void IdeamLinguisticsPlugin::_exit_tree() {
    if (linguistics_inspector_.is_valid()) {
        remove_inspector_plugin(linguistics_inspector_);
        linguistics_inspector_.unref();
    }
    set_plugin_active("IdeamLinguistics", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamLinguisticsPlugin::get_static_undo_redo() {
    return singleton ? singleton->get_undo_redo() : nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamLinguisticsPlugin::create_undo_redo_wrapper() {
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
