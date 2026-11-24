#pragma once

/**
 * ideam_gizmo.h
 * Trans-Environment Gizmo Controller Base Class (`IdeamGizmo`, `godot::RefCounted`)
 * in `src/godot/gizmos/`.
 *
 * Mirrors the architectural philosophy of `IdeamUndoRedo`:
 *   - Domain gizmos inherit `IdeamGizmo` instead of `godot::EditorNode3DGizmoPlugin`.
 *   - Emits semantic draw commands into `IdeamGizmoDrawList`.
 *   - Receives unified handle drag, click, and modifier-key callbacks across all 4 contexts:
 *       1. Godot Editor 3D Viewport (`IdeamEditorGizmoAdapter`)
 *       2. Custom Editor 3D SubViewport (`IdeamViewportGizmoBackend` / `IdeamViewport3DHost`)
 *       3. Runtime 3D Gameplay World (`IdeamGizmoSession`)
 *       4. Runtime Dedicated Avatar Creator / Modeler (`CharacterModeler`)
 */

#include "ideam_gizmo_draw_list.h"
#include "../tools/tool_context.h"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

class IdeamGizmo : public godot::RefCounted {
    GDCLASS(IdeamGizmo, godot::RefCounted)

private:
    godot::Ref<ToolContext> tool_context_;
    godot::StringName active_tool_mode_;
    bool enabled_ = true;
    godot::BitField<godot::KeyModifierMask> current_modifiers_ = 0;

protected:
    static void _bind_methods();

    /**
     * Helper returning the shared `IdeamUndoRedo` from `tool_context_` when available.
     */
    [[nodiscard]] godot::Ref<IdeamUndoRedo> resolve_undo_redo() const;

public:
    IdeamGizmo() = default;
    ~IdeamGizmo() override = default;

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void set_active_tool_mode(const godot::StringName &p_mode);
    [[nodiscard]] godot::StringName get_active_tool_mode() const { return active_tool_mode_; }

    void set_enabled(bool p_enabled);
    [[nodiscard]] bool is_enabled() const noexcept { return enabled_; }

    [[nodiscard]] godot::BitField<godot::KeyModifierMask> get_current_modifiers() const noexcept {
        return current_modifiers_;
    }

    // --- Virtual Domain Hooks ---
    [[nodiscard]] virtual bool can_handle(godot::Node3D *p_target) const { return false; }
    [[nodiscard]] virtual godot::String get_gizmo_name() const { return "IdeamGizmo"; }
    [[nodiscard]] virtual int32_t get_priority() const { return 0; }

    virtual void build_draw_list(
        godot::Node3D *p_target,
        IdeamGizmoDrawList &r_out,
        const GizmoViewState &p_view
    ) const {}

    [[nodiscard]] virtual godot::String get_handle_name(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        bool p_secondary
    ) const {
        (void)p_target;
        (void)p_handle_id;
        (void)p_secondary;
        return "Handle";
    }

    [[nodiscard]] virtual godot::Variant get_handle_value(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        bool p_secondary
    ) const {
        (void)p_target;
        (void)p_handle_id;
        (void)p_secondary;
        return godot::Variant();
    }

    virtual void on_handle_drag_begin(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        const GizmoRay &p_ray
    ) {
        (void)p_target;
        (void)p_handle_id;
        (void)p_ray;
    }

    virtual void on_handle_drag_update(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        const GizmoRay &p_ray,
        godot::BitField<godot::KeyModifierMask> p_modifiers
    ) {
        (void)p_target;
        (void)p_handle_id;
        (void)p_ray;
        (void)p_modifiers;
    }

    virtual void on_handle_drag_commit(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        bool p_cancel
    ) {
        (void)p_target;
        (void)p_handle_id;
        (void)p_cancel;
    }

    /**
     * Called when modifier keys (e.g., Shift for Features Focus Mode) change while the target is active.
     * Return true to request an immediate gizmo redraw.
     */
    virtual bool on_modifier_state_changed(
        godot::Node3D *p_target,
        godot::BitField<godot::KeyModifierMask> p_modifiers
    ) {
        (void)p_target;
        if (current_modifiers_ == p_modifiers) {
            return false;
        }
        current_modifiers_ = p_modifiers;
        return false;
    }

    /**
     * Called on discrete handle click (e.g., Shift+Click feature focus selection).
     * Return true if the click was consumed.
     */
    virtual bool on_handle_clicked(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        godot::BitField<godot::KeyModifierMask> p_modifiers
    ) {
        (void)p_target;
        (void)p_handle_id;
        (void)p_modifiers;
        return false;
    }

    void request_redraw();
};

} // namespace ideam::godot_ext
