#pragma once

/**
 * ideam_gizmo_style_resolver.h
 * Diegetic 3D Gizmo Style Resolver (`IdeamGizmoStyleResolver`, `src/godot/gizmos/`).
 *
 * Queries `ToolContext::get_active_theme_3d()` (`Theme3DResource::resolve_cascaded_style()`)
 * using semantic role IDs and state tags (`hover`, `dragging`, `locked`, `asymmetric_detail`,
 * `limit_reached`), enabling zero-C++ reskinning of gizmos across Editor and Runtime contexts
 * (e.g., brass calipers, sci-fi holograms, chalk architectural lines, painterly strokes).
 */

#include "ideam_gizmo_draw_list.h"
#include "../tools/tool_context.h"

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/color.hpp>

#include <unordered_map>

namespace ideam::godot_ext {

struct ResolvedGizmoStyle {
    godot::Color color = godot::Color(0.95f, 0.85f, 0.30f, 0.95f);
    godot::Ref<godot::Material> material;
    godot::Ref<godot::Mesh> handle_mesh;
    real_t handle_scale = static_cast<real_t>(1.0);
    GizmoDepthTestMode depth_test = DEPTH_TEST_OVERLAY;
};

class IdeamGizmoStyleResolver : public godot::RefCounted {
    GDCLASS(IdeamGizmoStyleResolver, godot::RefCounted)

private:
    godot::Ref<ToolContext> tool_context_;
    mutable std::unordered_map<uint64_t, godot::Ref<godot::StandardMaterial3D>> material_cache_;
    mutable std::unordered_map<int32_t, godot::Ref<godot::Mesh>> default_shape_cache_;

    static int _semantic_role_to_design_role_id(GizmoSemanticRole p_role) noexcept;
    static godot::Color _default_color_for_role(GizmoSemanticRole p_role) noexcept;
    static godot::Color _apply_state_tint(const godot::Color &p_base, uint32_t p_state_flags) noexcept;
    godot::Ref<godot::Mesh> _get_default_mesh_for_shape(GizmoHandleShapeHint p_shape) const;

protected:
    static void _bind_methods();

public:
    IdeamGizmoStyleResolver() = default;
    ~IdeamGizmoStyleResolver() override = default;

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void clear_cache();

    [[nodiscard]] ResolvedGizmoStyle resolve_style(
        GizmoSemanticRole p_role,
        uint32_t p_state_flags = GIZMO_STATE_NONE,
        GizmoHandleShapeHint p_shape = HANDLE_SHAPE_SPHERE,
        GizmoDepthTestMode p_depth_test = DEPTH_TEST_OVERLAY
    ) const;

    [[nodiscard]] godot::Color resolve_color(
        GizmoSemanticRole p_role,
        uint32_t p_state_flags = GIZMO_STATE_NONE
    ) const;
};

} // namespace ideam::godot_ext
