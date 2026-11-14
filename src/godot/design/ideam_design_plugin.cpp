#include "ideam_design_plugin.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamDesignPlugin *IdeamDesignPlugin::singleton = nullptr;

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
    inspector_plugin_.instantiate();
    add_inspector_plugin(inspector_plugin_);
}

void IdeamDesignPlugin::_exit_tree() {
    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
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

void IdeamDesignPlugin::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("get_plugin_domain_name"), &IdeamDesignPlugin::get_plugin_domain_name);
    ClassDB::bind_method(D_METHOD("is_geometry_handshake_available"), &IdeamDesignPlugin::is_geometry_handshake_available);
    ClassDB::bind_method(D_METHOD("is_splines_handshake_available"), &IdeamDesignPlugin::is_splines_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tiles_handshake_available"), &IdeamDesignPlugin::is_tiles_handshake_available);
    ClassDB::bind_method(D_METHOD("is_linguistics_handshake_available"), &IdeamDesignPlugin::is_linguistics_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tasks_handshake_available"), &IdeamDesignPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    ClassDB::bind_static_method("IdeamDesignPlugin", D_METHOD("undo_redo"), &IdeamDesignPlugin::get_static_undo_redo);
#endif
    ClassDB::bind_static_method("IdeamDesignPlugin", D_METHOD("create_undo_redo_wrapper"), &IdeamDesignPlugin::create_undo_redo_wrapper);
}

} // namespace ideam::godot_ext
