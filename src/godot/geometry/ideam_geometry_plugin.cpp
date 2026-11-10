#include "ideam_geometry_plugin.h"

#include "../../core/tasks/domains/geometry/registration/geometry_task_domain_pack.h"
#include "../../core/tasks/registration/ideam_task_registry.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamGeometryPlugin *IdeamGeometryPlugin::singleton = nullptr;

void IdeamGeometryPlugin::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("get_plugin_domain_name"), &IdeamGeometryPlugin::get_plugin_domain_name);
    ClassDB::bind_method(D_METHOD("register_task_domain_pack"), &IdeamGeometryPlugin::register_task_domain_pack);
    ClassDB::bind_method(D_METHOD("is_splines_handshake_available"), &IdeamGeometryPlugin::is_splines_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tiles_handshake_available"), &IdeamGeometryPlugin::is_tiles_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tasks_handshake_available"), &IdeamGeometryPlugin::is_tasks_handshake_available);
    ClassDB::bind_method(D_METHOD("is_cymatics_handshake_available"), &IdeamGeometryPlugin::is_cymatics_handshake_available);

#ifdef TOOLS_ENABLED
    ClassDB::bind_static_method(
        "IdeamGeometryPlugin",
        D_METHOD("undo_redo"),
        &IdeamGeometryPlugin::get_static_undo_redo
    );
#endif
    ClassDB::bind_static_method(
        "IdeamGeometryPlugin",
        D_METHOD("create_undo_redo_wrapper"),
        &IdeamGeometryPlugin::create_undo_redo_wrapper
    );
}

IdeamGeometryPlugin::IdeamGeometryPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamGeometryPlugin::~IdeamGeometryPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamGeometryPlugin::_enter_tree() {
    validate_script_templates();

    // 1. Ecosystem Handshake: Announce IdeamGeometry is active and register GeometryTaskDomainPack
    set_plugin_active("IdeamGeometry", true);
    register_to_ecosystem("WizardSettings", "IdeamGeometry", godot::String(GEOMETRY_SETTINGS_PATH.data()));
    register_to_ecosystem("TaskManifests", "IdeamGeometry", godot::String(core::GeometryTaskDomainPack::MANIFEST_PATH));
    if (is_plugin_active("IdeamTasks")) {
        register_task_domain_pack();
    }

    // 2. Register shared Geometry Resource Inspectors
    polygon_2d_inspector.instantiate();
    add_inspector_plugin(polygon_2d_inspector);
}

void IdeamGeometryPlugin::_exit_tree() {
    if (polygon_2d_inspector.is_valid()) {
        remove_inspector_plugin(polygon_2d_inspector);
        polygon_2d_inspector.unref();
    }

    set_plugin_active("IdeamGeometry", false);
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        task_reg->set_domain_pack_active("IdeamGeometry", false);
    }
}

void IdeamGeometryPlugin::register_task_domain_pack() {
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        if (!task_reg->has_domain_pack("IdeamGeometry")) {
            core::GeometryTaskDomainPack::register_domain();
        }
        task_reg->set_domain_pack_active("IdeamGeometry", true);
    }
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamGeometryPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamGeometryPlugin::create_undo_redo_wrapper() {
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
