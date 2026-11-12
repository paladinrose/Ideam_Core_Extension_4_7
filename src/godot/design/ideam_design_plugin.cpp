#include "ideam_design_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamDesignPlugin *IdeamDesignPlugin::singleton = nullptr;

void IdeamDesignPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamDesignPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_geometry_handshake_available"), &IdeamDesignPlugin::is_geometry_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_splines_handshake_available"), &IdeamDesignPlugin::is_splines_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tiles_handshake_available"), &IdeamDesignPlugin::is_tiles_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_characters_handshake_available"), &IdeamDesignPlugin::is_characters_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_wfc_handshake_available"), &IdeamDesignPlugin::is_wfc_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamDesignPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamDesignPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamDesignPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamDesignPlugin::create_undo_redo_wrapper
    );
}

IdeamDesignPlugin::IdeamDesignPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamDesignPlugin::~IdeamDesignPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamDesignPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamDesign", true);
    register_to_ecosystem("WizardSettings", "IdeamDesign", godot::String(DESIGN_SETTINGS_PATH.data()));
}

void IdeamDesignPlugin::_exit_tree() {
    set_plugin_active("IdeamDesign", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamDesignPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamDesignPlugin::create_undo_redo_wrapper() {
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
