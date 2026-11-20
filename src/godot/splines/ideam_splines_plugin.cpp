#include "ideam_splines_plugin.h"

#include "../tools/tool_registry.h"
#include "../geometry/ui/geometry_composer.h"
#include "spline_1d/spline_1d_control.h"
#include "spline_2d/spline_2d_control.h"
#include "spline_3d/spline_3d_control.h"
#include "spline_4d/color_spline_control.h"
#include "spline_4d/spline_4d_control.h"
#include <godot_cpp/classes/v_box_container.hpp>

#include "../../core/tasks/domains/splines/registration/splines_task_domain_pack.h"
#include "../../core/tasks/registration/ideam_task_registry.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamSplinesPlugin *IdeamSplinesPlugin::singleton = nullptr;

void IdeamSplinesPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamSplinesPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("register_task_domain_pack"), &IdeamSplinesPlugin::register_task_domain_pack);
    godot::ClassDB::bind_method(godot::D_METHOD("is_geometry_handshake_available"), &IdeamSplinesPlugin::is_geometry_handshake_available);
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

    // 1. Ecosystem Handshake: Announce IdeamSplines is active & register SplinesTaskDomainPack
    set_plugin_active("IdeamSplines", true);
    register_to_ecosystem("WizardSettings", "IdeamSplines", godot::String(SPLINES_SETTINGS_PATH.data()));
    register_to_ecosystem("TaskManifests", "IdeamSplines", godot::String(core::SplinesTaskDomainPack::MANIFEST_PATH));
    if (is_plugin_active("IdeamTasks")) {
        register_task_domain_pack();
    }

    // 2. Instantiate and register all 1D-4D & Tween Spline Resource Inspector Plugins
    spline_1d_inspector.instantiate();
    add_inspector_plugin(spline_1d_inspector);

    spline_2d_inspector.instantiate();
    add_inspector_plugin(spline_2d_inspector);

    spline_3d_inspector.instantiate();
    add_inspector_plugin(spline_3d_inspector);

    spline_4d_inspector.instantiate();
    add_inspector_plugin(spline_4d_inspector);

    spline_tween_inspector.instantiate();
    add_inspector_plugin(spline_tween_inspector);

    // 3. Instantiate and register 3D, 4D, Extrude, Scatter & Corridor Viewport Gizmo Plugins
    spline_3d_gizmo.instantiate();
    add_node_3d_gizmo_plugin(spline_3d_gizmo);

    spline_4d_gizmo.instantiate();
    add_node_3d_gizmo_plugin(spline_4d_gizmo);

    spline_extrude_gizmo.instantiate();
    add_node_3d_gizmo_plugin(spline_extrude_gizmo);

    spline_scatter_gizmo.instantiate();
    add_node_3d_gizmo_plugin(spline_scatter_gizmo);

    spline_corridor_gizmo.instantiate();
    add_node_3d_gizmo_plugin(spline_corridor_gizmo);

    // 4. Mount 2D Scene-Space Viewport Editor Toolbar for SplineNode2D & SplinePolygon2D
    spline_2d_viewport_editor = memnew(Spline2DViewportEditor);
    spline_2d_viewport_editor->set_owner_plugin(this);
    add_control_to_container(EditorPlugin::CONTAINER_CANVAS_EDITOR_MENU, spline_2d_viewport_editor);

    // 5. Register Splines Tier 1 Controls with GeometryComposer ("Splines" tab)
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(GeometryComposer::WORKSPACE_ID);

    reg.register_control({
        ws_id, "Spline1DResource", "Splines", "1D Spline", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            godot::Ref<Spline1DResource> s1 = p_asset;
            if (s1.is_null()) return nullptr;
            if (p_ctx.is_valid() && p_ctx->get_undo_redo().is_valid()) s1->set_undo_redo(p_ctx->get_undo_redo());
            auto *ctrl = memnew(Spline1DControl);
            ctrl->set_spline_1d(s1);
            return ctrl;
        },
        nullptr
    });

    reg.register_control({
        ws_id, "Spline2DResource", "Splines", "2D Spline", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            godot::Ref<Spline2DResource> s2 = p_asset;
            if (s2.is_null()) return nullptr;
            if (p_ctx.is_valid() && p_ctx->get_undo_redo().is_valid()) s2->set_undo_redo(p_ctx->get_undo_redo());
            auto *ctrl = memnew(Spline2DControl);
            ctrl->set_spline_2d(s2);
            return ctrl;
        },
        nullptr
    });

    reg.register_control({
        ws_id, "Spline3DResource", "Splines", "3D Spline", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            godot::Ref<Spline3DResource> s3 = p_asset;
            if (s3.is_null()) return nullptr;
            if (p_ctx.is_valid() && p_ctx->get_undo_redo().is_valid()) s3->set_undo_redo(p_ctx->get_undo_redo());
            auto *ctrl = memnew(Spline3DControl);
            ctrl->set_spline_3d(s3);
            return ctrl;
        },
        nullptr
    });

    reg.register_control({
        ws_id, "Spline4DResource", "Splines", "4D / Color Spline", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            godot::Ref<Spline4DResource> s4 = p_asset;
            if (s4.is_null()) return nullptr;
            if (p_ctx.is_valid() && p_ctx->get_undo_redo().is_valid()) s4->set_undo_redo(p_ctx->get_undo_redo());
            if (s4->get_color_gradient_mode()) {
                auto *wrapper = memnew(godot::VBoxContainer);
                wrapper->add_theme_constant_override("separation", 8);
                auto *color_ctrl = memnew(ColorSplineControl);
                color_ctrl->set_spline_4d(s4);
                wrapper->add_child(color_ctrl);
                auto *spatial_ctrl = memnew(Spline4DControl);
                spatial_ctrl->set_spline_4d(s4);
                wrapper->add_child(spatial_ctrl);
                return wrapper;
            }
            auto *ctrl = memnew(Spline4DControl);
            ctrl->set_spline_4d(s4);
            return ctrl;
        },
        nullptr
    });

    reg.register_control({
        ws_id, "SplineTweenResource", "Splines", "Spline Tween", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            (void)p_ctx;
            godot::Ref<SplineTweenResource> tw = p_asset;
            if (tw.is_null()) return nullptr;
            auto *ctrl = memnew(SplineTweenerControl);
            ctrl->set_tween_resource(tw);
            return ctrl;
        },
        nullptr
    });
}

void IdeamSplinesPlugin::_exit_tree() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(GeometryComposer::WORKSPACE_ID);
    reg.unregister_class(ws_id, "Spline1DResource");
    reg.unregister_class(ws_id, "Spline2DResource");
    reg.unregister_class(ws_id, "Spline3DResource");
    reg.unregister_class(ws_id, "Spline4DResource");
    reg.unregister_class(ws_id, "SplineTweenResource");
    if (spline_2d_viewport_editor) {
        remove_control_from_container(EditorPlugin::CONTAINER_CANVAS_EDITOR_MENU, spline_2d_viewport_editor);
        memdelete(spline_2d_viewport_editor);
        spline_2d_viewport_editor = nullptr;
    }
    if (spline_corridor_gizmo.is_valid()) {
        remove_node_3d_gizmo_plugin(spline_corridor_gizmo);
        spline_corridor_gizmo.unref();
    }
    if (spline_scatter_gizmo.is_valid()) {
        remove_node_3d_gizmo_plugin(spline_scatter_gizmo);
        spline_scatter_gizmo.unref();
    }
    if (spline_extrude_gizmo.is_valid()) {
        remove_node_3d_gizmo_plugin(spline_extrude_gizmo);
        spline_extrude_gizmo.unref();
    }
    if (spline_4d_gizmo.is_valid()) {
        remove_node_3d_gizmo_plugin(spline_4d_gizmo);
        spline_4d_gizmo.unref();
    }
    if (spline_3d_gizmo.is_valid()) {
        remove_node_3d_gizmo_plugin(spline_3d_gizmo);
        spline_3d_gizmo.unref();
    }

    if (spline_tween_inspector.is_valid()) {
        remove_inspector_plugin(spline_tween_inspector);
        spline_tween_inspector.unref();
    }
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

    set_plugin_active("IdeamSplines", false);
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        task_reg->set_domain_pack_active("IdeamSplines", false);
    }
}

bool IdeamSplinesPlugin::_handles(godot::Object *p_object) const {
    return spline_2d_viewport_editor && spline_2d_viewport_editor->handles_object(p_object);
}

void IdeamSplinesPlugin::_edit(godot::Object *p_object) {
    if (spline_2d_viewport_editor) {
        spline_2d_viewport_editor->edit_object(p_object);
    }
}

void IdeamSplinesPlugin::_make_visible(bool p_visible) {
    if (spline_2d_viewport_editor) {
        spline_2d_viewport_editor->set_editor_visible(p_visible);
    }
}

bool IdeamSplinesPlugin::_forward_canvas_gui_input(const godot::Ref<godot::InputEvent> &p_event) {
    if (spline_2d_viewport_editor) {
        return spline_2d_viewport_editor->forward_canvas_gui_input(p_event);
    }
    return false;
}

void IdeamSplinesPlugin::_forward_canvas_draw_over_viewport(godot::Control *p_viewport_control) {
    if (spline_2d_viewport_editor) {
        spline_2d_viewport_editor->forward_canvas_draw_over_viewport(p_viewport_control);
    }
}

void IdeamSplinesPlugin::register_task_domain_pack() {
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        if (!task_reg->has_domain_pack("IdeamSplines")) {
            core::SplinesTaskDomainPack::register_domain();
        }
        task_reg->set_domain_pack_active("IdeamSplines", true);
    }
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