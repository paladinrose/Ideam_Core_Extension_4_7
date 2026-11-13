#include "ideam_communications_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamCommunicationsPlugin *IdeamCommunicationsPlugin::singleton = nullptr;

void IdeamCommunicationsPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamCommunicationsPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_linguistics_handshake_available"), &IdeamCommunicationsPlugin::is_linguistics_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_minds_handshake_available"), &IdeamCommunicationsPlugin::is_minds_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_splines_handshake_available"), &IdeamCommunicationsPlugin::is_splines_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_narratives_handshake_available"), &IdeamCommunicationsPlugin::is_narratives_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_games_handshake_available"), &IdeamCommunicationsPlugin::is_games_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamCommunicationsPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamCommunicationsPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamCommunicationsPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamCommunicationsPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamCommunicationsPlugin::create_undo_redo_wrapper
    );
}

IdeamCommunicationsPlugin::IdeamCommunicationsPlugin() {
    if (singleton == nullptr) singleton = this;
}

IdeamCommunicationsPlugin::~IdeamCommunicationsPlugin() {
    if (singleton == this) singleton = nullptr;
}

void IdeamCommunicationsPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamCommunications", true);
    register_to_ecosystem("WizardSettings", "IdeamCommunications", godot::String(COMMUNICATIONS_SETTINGS_PATH.data()));

    communications_inspector_.instantiate();
    add_inspector_plugin(communications_inspector_);
}

void IdeamCommunicationsPlugin::_exit_tree() {
    if (communications_inspector_.is_valid()) {
        remove_inspector_plugin(communications_inspector_);
        communications_inspector_.unref();
    }
    set_plugin_active("IdeamCommunications", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamCommunicationsPlugin::get_static_undo_redo() {
    return singleton ? singleton->get_undo_redo() : nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamCommunicationsPlugin::create_undo_redo_wrapper() {
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
