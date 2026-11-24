#pragma once

/**
 * ideam_gizmo_backend.h
 * Trans-Environment Gizmo Rendering & Input Backends (`src/godot/gizmos/`):
 *   - `IdeamGizmoBackend`: Abstract backend interface.
 *   - `IdeamEditorGizmoAdapter` (`#ifdef TOOLS_ENABLED`): Thin `godot::EditorNode3DGizmoPlugin`
 *     translating `IdeamGizmoDrawList` into Editor 3D viewport gizmo calls and forwarding
 *     `_set_handle`, `_commit_handle`, and `_forward_3d_gui_input` modifier keys.
 *   - `IdeamViewportGizmoBackend`: Runtime-safe `Node3D` backend rendering `IdeamGizmoDrawList`
 *     via `RenderingServer` (`ImmediateMesh` + handle/mesh instances) into any `World3D`,
 *     with screen-space ray-to-sphere handle picking and depth-test mode control.
 *   - `IdeamGizmoSession`: High-level `Node3D` session orchestrating registered `IdeamGizmo`s,
 *     active target `Node3D`, `Camera3D`, `ToolContext`, modifier tracking, and input routing
 *     across Contexts 2, 3, and 4.
 */

#include "ideam_gizmo.h"
#include "ideam_gizmo_style_resolver.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#endif

#include <cstdint>
#include <vector>

namespace ideam::godot_ext {

/**
 * Abstract backend contract implemented by Editor and Runtime Viewport backends.
 */
class IdeamGizmoBackend {
public:
    virtual ~IdeamGizmoBackend() = default;
    virtual void set_style_resolver(const godot::Ref<IdeamGizmoStyleResolver> &p_resolver) = 0;
    virtual void request_redraw() = 0;
    [[nodiscard]] virtual bool is_editor_backend() const noexcept = 0;
};

#ifdef TOOLS_ENABLED
/**
 * Context 1 Backend — Bridges any `Ref<IdeamGizmo>` into Godot's 3D Editor Viewport.
 */
class IdeamEditorGizmoAdapter : public godot::EditorNode3DGizmoPlugin, public IdeamGizmoBackend {
    GDCLASS(IdeamEditorGizmoAdapter, godot::EditorNode3DGizmoPlugin)

private:
    godot::Ref<IdeamGizmo> gizmo_;
    godot::Ref<IdeamGizmoStyleResolver> style_resolver_;
    bool drag_active_         = false;
    int32_t active_handle_id_ = -1;
    uint64_t last_target_id_  = 0u;
    godot::BitField<godot::KeyModifierMask> last_modifiers_ = 0;

    void _on_gizmo_redraw_requested();

protected:
    static void _bind_methods();

public:
    IdeamEditorGizmoAdapter();
    ~IdeamEditorGizmoAdapter() override;

    void set_ideam_gizmo(const godot::Ref<IdeamGizmo> &p_gizmo);
    [[nodiscard]] godot::Ref<IdeamGizmo> get_ideam_gizmo() const { return gizmo_; }

    void set_style_resolver(const godot::Ref<IdeamGizmoStyleResolver> &p_resolver) override;
    void request_redraw() override;
    [[nodiscard]] bool is_editor_backend() const noexcept override { return true; }

    /**
     * Called from `IdeamEditorPlugin::_forward_3d_gui_input()` to propagate Shift/Ctrl/Alt
     * modifier transitions and Shift+Click events into the underlying `IdeamGizmo`.
     */
    int32_t forward_3d_gui_input(
        godot::Camera3D *p_camera,
        const godot::Ref<godot::InputEvent> &p_event,
        godot::Node3D *p_selected_node
    );

    // --- godot::EditorNode3DGizmoPlugin overrides ---
    [[nodiscard]] bool _has_gizmo(godot::Node3D *p_for_node_3d) const override;
    [[nodiscard]] godot::String _get_gizmo_name() const override;
    [[nodiscard]] int32_t _get_priority() const override;

    void _redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) override;
    [[nodiscard]] godot::String _get_handle_name(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary
    ) const override;
    [[nodiscard]] godot::Variant _get_handle_value(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary
    ) const override;
    void _set_handle(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary,
        godot::Camera3D *p_camera,
        const godot::Vector2 &p_screen_pos
    ) override;
    void _commit_handle(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary,
        const godot::Variant &p_restore,
        bool p_cancel
    ) override;
};
#endif // TOOLS_ENABLED

/**
 * Contexts 2, 3, and 4 Backend — Runtime-safe `RenderingServer` gizmo renderer & ray picker.
 */
class IdeamViewportGizmoBackend : public godot::Node3D, public IdeamGizmoBackend {
    GDCLASS(IdeamViewportGizmoBackend, godot::Node3D)

private:
    struct PickableHandle {
        int32_t gizmo_index = 0;
        int32_t handle_id   = 0;
        godot::Vector3 world_pos;
        real_t world_radius = static_cast<real_t>(0.045);
        bool secondary      = false;
    };

    struct RenderedInstance {
        godot::RID instance_rid;
        godot::Ref<godot::Mesh> mesh_ref;
        godot::Ref<godot::Material> material_ref;
    };

    godot::Ref<IdeamGizmoStyleResolver> style_resolver_;
    GizmoDepthTestMode depth_test_override_ = DEPTH_TEST_OVERLAY;
    bool use_depth_test_override_           = false;

    std::vector<RenderedInstance> rendered_instances_;
    std::vector<PickableHandle> pickable_handles_;

    void _clear_rendered_instances();
    void _submit_draw_list(
        int32_t p_gizmo_index,
        const godot::Transform3D &p_target_xform,
        const IdeamGizmoDrawList &p_list
    );

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    IdeamViewportGizmoBackend();
    ~IdeamViewportGizmoBackend() override;

    void set_style_resolver(const godot::Ref<IdeamGizmoStyleResolver> &p_resolver) override;
    [[nodiscard]] godot::Ref<IdeamGizmoStyleResolver> get_style_resolver() const { return style_resolver_; }

    void set_depth_test_override(bool p_enabled, GizmoDepthTestMode p_mode = DEPTH_TEST_DIEGETIC_WORLD);

    void begin_frame();
    void append_draw_list(
        int32_t p_gizmo_index,
        const godot::Transform3D &p_target_xform,
        const IdeamGizmoDrawList &p_list
    );

    /**
     * Picks the closest handle intersected by `p_ray` (screen-space / world-ray sphere test).
     * Returns true if a handle was hit, populating `r_gizmo_index` and `r_handle_id`.
     */
    [[nodiscard]] bool pick_handle(
        const GizmoRay &p_ray,
        int32_t &r_gizmo_index,
        int32_t &r_handle_id,
        real_t p_pick_radius_scale = static_cast<real_t>(1.65)
    ) const;

    void request_redraw() override {}
    [[nodiscard]] bool is_editor_backend() const noexcept override { return false; }
};

/**
 * High-level runtime & SubViewport gizmo session (`IdeamGizmoSession`) coordinating
 * multiple `IdeamGizmo` instances, target binding, camera raycasting, modifier states,
 * and Ctrl+Z / Ctrl+Y undo/redo dispatch.
 */
class IdeamGizmoSession : public godot::Node3D {
    GDCLASS(IdeamGizmoSession, godot::Node3D)

private:
    godot::Ref<ToolContext> tool_context_;
    godot::Ref<IdeamGizmoStyleResolver> style_resolver_;
    IdeamViewportGizmoBackend *backend_ = nullptr;
    std::vector<godot::Ref<IdeamGizmo>> gizmos_;

    uint64_t target_node_id_ = 0u;
    uint64_t camera_node_id_ = 0u;
    godot::StringName active_tool_mode_;

    bool redraw_dirty_        = true;
    int32_t hovered_gizmo_    = -1;
    int32_t hovered_handle_   = -1;
    bool is_dragging_         = false;
    int32_t drag_gizmo_       = -1;
    int32_t drag_handle_      = -1;
    godot::Vector2 press_screen_pos_;
    bool drag_moved_          = false;
    godot::BitField<godot::KeyModifierMask> modifiers_ = 0;

    [[nodiscard]] godot::Node3D *_resolve_target() const;
    [[nodiscard]] godot::Camera3D *_resolve_camera() const;
    void _on_gizmo_redraw_requested();
    void _on_theme_3d_changed(const godot::Ref<godot::Resource> &p_theme_3d);

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    IdeamGizmoSession();
    ~IdeamGizmoSession() override = default;

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void set_target_node(godot::Node3D *p_target);
    [[nodiscard]] godot::Node3D *get_target_node() const { return _resolve_target(); }

    void set_camera(godot::Camera3D *p_camera);
    [[nodiscard]] godot::Camera3D *get_camera() const { return _resolve_camera(); }

    void set_active_tool_mode(const godot::StringName &p_mode);
    [[nodiscard]] godot::StringName get_active_tool_mode() const { return active_tool_mode_; }

    void set_diegetic_depth_test(bool p_enabled);

    void add_gizmo(const godot::Ref<IdeamGizmo> &p_gizmo);
    void remove_gizmo(const godot::Ref<IdeamGizmo> &p_gizmo);
    void clear_gizmos();

    void mark_dirty() noexcept { redraw_dirty_ = true; }
    void rebuild_gizmos_now();

    /**
     * Routes an `InputEvent` (mouse press/release/motion or key modifier/undo) into the session.
     * Returns true if the event was consumed by a gizmo handle or shortcut.
     */
    bool handle_input_event(const godot::Ref<godot::InputEvent> &p_event);
};

} // namespace ideam::godot_ext
