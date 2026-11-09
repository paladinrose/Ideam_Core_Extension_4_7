#include "ideam_splines_plugin.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamSplinesPlugin *IdeamSplinesPlugin::singleton = nullptr;

void IdeamSplinesPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamSplinesPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tiles_handshake_available"), &IdeamSplinesPlugin::is_tiles_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamSplinesPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamSplinesPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamSplinesPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamSplinesPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamSplinesPlugin::create_undo_redo_wrapper
    );
}

IdeamSplinesPlugin::IdeamSplinesPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamSplinesPlugin::~IdeamSplinesPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamSplinesPlugin::_enter_tree() {
    validate_script_templates();

    // 1. Ecosystem Handshake: Announce IdeamSplines is active
    set_plugin_active("IdeamSplines", true);
    register_to_ecosystem("WizardSettings", "IdeamSplines", godot::String(SPLINES_SETTINGS_PATH.data()));

    // 2. Instantiate and register all 1D-4D Spline Resource Inspector Plugins
    spline_1d_inspector.instantiate();
    add_inspector_plugin(spline_1d_inspector);

    spline_2d_inspector.instantiate();
    add_inspector_plugin(spline_2d_inspector);

    spline_3d_inspector.instantiate();
    add_inspector_plugin(spline_3d_inspector);

    spline_4d_inspector.instantiate();
    add_inspector_plugin(spline_4d_inspector);

    // [FUTURE PHASE] 3D/4D Viewport Gizmos & SplineDOD TaskDomainPack Registration:
    // - Register Spline3DGizmo and Spline4DGizmo via add_node_3d_gizmo_plugin().
    // - If is_plugin_active("IdeamTasks"), register SplinesTaskDomainPack.
}

void IdeamSplinesPlugin::_exit_tree() {
    if (spline_4d_inspector.is_valid()) {
        remove_inspector_plugin(spline_4d_inspector);
        spline_4d_inspector.unref();
    }
    if (spline_3d_inspector.is_valid()) {
        remove_inspector_plugin(spline_3d_inspector);
        spline_3d_inspector.unref();
    }
    if (spline_2d_inspector.is_valid()) {
        remove_inspector_plugin(spline_2d_inspector);
        spline_2d_inspector.unref();
    }
    if (spline_1d_inspector.is_valid()) {
        remove_inspector_plugin(spline_1d_inspector);
        spline_1d_inspector.unref();
    }

    // [FUTURE PHASE] Unregister 3D/4D gizmos and task domain packs here.

    // Ecosystem Handshake: Mark IdeamSplines inactive
    set_plugin_active("IdeamSplines", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamSplinesPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamSplinesPlugin::create_undo_redo_wrapper() {
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