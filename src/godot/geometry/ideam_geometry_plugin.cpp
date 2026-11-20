#include "ideam_geometry_plugin.h"

#include "../controls/ideam_tier1_control_registry.h"
#include "ui/geometry_composer.h"
#include "ui/polygon_2d_control.h"
#include "ui/sdf_grid_2d_canvas.h"
#include "ui/tensor_field_2d_canvas.h"
#include "ui/voronoi_2d_canvas.h"

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

    // 2. Register shared Geometry Resource Inspectors (Section 3.2)
    polygon_2d_inspector.instantiate();
    add_inspector_plugin(polygon_2d_inspector);

    voronoi_2d_inspector.instantiate();
    add_inspector_plugin(voronoi_2d_inspector);

    sdf_grid_2d_inspector.instantiate();
    add_inspector_plugin(sdf_grid_2d_inspector);

    tensor_field_2d_inspector.instantiate();
    add_inspector_plugin(tensor_field_2d_inspector);

    // 3. Register Geometry Tier 1 Controls with GeometryComposer ("General" tab)
    auto &reg = IdeamTier1ControlRegistry::get_singleton();
    const godot::StringName ws_id(GeometryComposer::WORKSPACE_ID);

    reg.register_workspace_launcher({
        ws_id,
        "Geometry Composer",
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<IdeamToolContext> &p_ctx) {
            if (GeometryComposer *composer = GeometryComposer::get_or_create_shared_composer()) {
                if (p_ctx.is_valid()) composer->set_tool_context(p_ctx);
                composer->open_asset(p_asset);
            }
        }
    });

    reg.register_control({
        ws_id, "Polygon2DResource", "General", "2D Polygon", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<IdeamToolContext> &p_ctx) -> godot::Control * {
            godot::Ref<Polygon2DResource> poly = p_asset;
            if (poly.is_null()) return nullptr;
            if (p_ctx.is_valid() && p_ctx->get_undo_redo().is_valid()) poly->set_undo_redo(p_ctx->get_undo_redo());
            auto *ctrl = memnew(Polygon2DControl);
            ctrl->set_polygon_resource(poly);
            return ctrl;
        },
        nullptr
    });

    reg.register_control({
        ws_id, "VoronoiDiagram2DResource", "General", "Voronoi 2D", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<IdeamToolContext> &p_ctx) -> godot::Control * {
            godot::Ref<VoronoiDiagram2DResource> vor = p_asset;
            if (vor.is_null()) return nullptr;
            if (p_ctx.is_valid() && p_ctx->get_undo_redo().is_valid()) vor->set_undo_redo(p_ctx->get_undo_redo());
            auto *ctrl = memnew(Voronoi2DControl);
            ctrl->set_voronoi_resource(vor);
            return ctrl;
        },
        nullptr
    });

    reg.register_control({
        ws_id, "SDFGrid2DResource", "General", "SDF Grid 2D", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<IdeamToolContext> &p_ctx) -> godot::Control * {
            godot::Ref<SDFGrid2DResource> sdf = p_asset;
            if (sdf.is_null()) return nullptr;
            if (p_ctx.is_valid() && p_ctx->get_undo_redo().is_valid()) sdf->set_undo_redo(p_ctx->get_undo_redo());
            auto *ctrl = memnew(SDFGrid2DControl);
            ctrl->set_sdf_grid_resource(sdf);
            return ctrl;
        },
        nullptr
    });

    reg.register_control({
        ws_id, "TensorField2DResource", "General", "Tensor Field 2D", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<IdeamToolContext> &p_ctx) -> godot::Control * {
            godot::Ref<TensorField2DResource> tf = p_asset;
            if (tf.is_null()) return nullptr;
            if (p_ctx.is_valid() && p_ctx->get_undo_redo().is_valid()) tf->set_undo_redo(p_ctx->get_undo_redo());
            auto *ctrl = memnew(TensorField2DControl);
            ctrl->set_custom_minimum_size(godot::Vector2(0.0f, 360.0f));
            ctrl->set_geometry_resource(tf);
            return ctrl;
        },
        nullptr
    });
}

void IdeamGeometryPlugin::_exit_tree() {
    auto &reg = IdeamTier1ControlRegistry::get_singleton();
    const godot::StringName ws_id(GeometryComposer::WORKSPACE_ID);
    reg.unregister_class(ws_id, "Polygon2DResource");
    reg.unregister_class(ws_id, "VoronoiDiagram2DResource");
    reg.unregister_class(ws_id, "SDFGrid2DResource");
    reg.unregister_class(ws_id, "TensorField2DResource");
    reg.unregister_workspace_launcher(ws_id);
    if (tensor_field_2d_inspector.is_valid()) {
        remove_inspector_plugin(tensor_field_2d_inspector);
        tensor_field_2d_inspector.unref();
    }
    if (sdf_grid_2d_inspector.is_valid()) {
        remove_inspector_plugin(sdf_grid_2d_inspector);
        sdf_grid_2d_inspector.unref();
    }
    if (voronoi_2d_inspector.is_valid()) {
        remove_inspector_plugin(voronoi_2d_inspector);
        voronoi_2d_inspector.unref();
    }
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