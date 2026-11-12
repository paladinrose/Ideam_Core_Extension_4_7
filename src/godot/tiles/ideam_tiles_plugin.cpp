#include "ideam_tiles_plugin.h"

#include "../../core/tasks/domains/tiles/registration/tiles_task_domain_pack.h"
#include "../../core/tasks/registration/ideam_task_registry.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamTilesPlugin *IdeamTilesPlugin::singleton = nullptr;

void IdeamTilesPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamTilesPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("register_task_domain_pack"), &IdeamTilesPlugin::register_task_domain_pack);
    godot::ClassDB::bind_method(godot::D_METHOD("is_geometry_handshake_available"), &IdeamTilesPlugin::is_geometry_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_splines_handshake_available"), &IdeamTilesPlugin::is_splines_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamTilesPlugin::is_tasks_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_games_handshake_available"), &IdeamTilesPlugin::is_games_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamTilesPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamTilesPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamTilesPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamTilesPlugin::create_undo_redo_wrapper
    );
}

IdeamTilesPlugin::IdeamTilesPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamTilesPlugin::~IdeamTilesPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamTilesPlugin::_enter_tree() {
    validate_script_templates();

    // 1. Ecosystem Handshake: Announce IdeamTiles is active & register TilesTaskDomainPack
    set_plugin_active("IdeamTiles", true);
    register_to_ecosystem("WizardSettings", "IdeamTiles", godot::String(TILES_SETTINGS_PATH.data()));
    register_to_ecosystem("TaskManifests", "IdeamTiles", godot::String(core::TilesTaskDomainPack::MANIFEST_PATH));
    if (is_plugin_active("IdeamTasks")) {
        register_task_domain_pack();
    }

    // 2. Instantiate and register Tiling Resource Inspector Plugins
    wallpaper_tiling_inspector_.instantiate();
    add_inspector_plugin(wallpaper_tiling_inspector_);

    composite_tiling_inspector_.instantiate();
    add_inspector_plugin(composite_tiling_inspector_);

    hyperbolic_tiling_inspector_.instantiate();
    add_inspector_plugin(hyperbolic_tiling_inspector_);

    spherical_goldberg_inspector_.instantiate();
    add_inspector_plugin(spherical_goldberg_inspector_);

    // 3. Instantiate and register 3D Viewport Gizmos (GeodesicPlanetGizmo)
    geodesic_planet_gizmo_.instantiate();
    add_node_3d_gizmo_plugin(geodesic_planet_gizmo_);
}

void IdeamTilesPlugin::_exit_tree() {
    if (geodesic_planet_gizmo_.is_valid()) {
        remove_node_3d_gizmo_plugin(geodesic_planet_gizmo_);
        geodesic_planet_gizmo_.unref();
    }
    if (spherical_goldberg_inspector_.is_valid()) {
        remove_inspector_plugin(spherical_goldberg_inspector_);
        spherical_goldberg_inspector_.unref();
    }
    if (hyperbolic_tiling_inspector_.is_valid()) {
        remove_inspector_plugin(hyperbolic_tiling_inspector_);
        hyperbolic_tiling_inspector_.unref();
    }
    if (composite_tiling_inspector_.is_valid()) {
        remove_inspector_plugin(composite_tiling_inspector_);
        composite_tiling_inspector_.unref();
    }
    if (wallpaper_tiling_inspector_.is_valid()) {
        remove_inspector_plugin(wallpaper_tiling_inspector_);
        wallpaper_tiling_inspector_.unref();
    }

    set_plugin_active("IdeamTiles", false);
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        task_reg->set_domain_pack_active("IdeamTiles", false);
    }
}

void IdeamTilesPlugin::register_task_domain_pack() {
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        if (!task_reg->has_domain_pack("IdeamTiles")) {
            core::TilesTaskDomainPack::register_domain();
        }
        task_reg->set_domain_pack_active("IdeamTiles", true);
    }
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamTilesPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamTilesPlugin::create_undo_redo_wrapper() {
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