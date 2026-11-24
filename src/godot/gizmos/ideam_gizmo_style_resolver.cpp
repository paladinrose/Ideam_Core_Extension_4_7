#include "ideam_gizmo_style_resolver.h"

#include "../design/style_3d_resource.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/torus_mesh.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void IdeamGizmoStyleResolver::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &IdeamGizmoStyleResolver::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &IdeamGizmoStyleResolver::get_tool_context);
    ClassDB::bind_method(D_METHOD("clear_cache"), &IdeamGizmoStyleResolver::clear_cache);
}

void IdeamGizmoStyleResolver::set_tool_context(const godot::Ref<ToolContext> &p_context) {
    if (tool_context_ == p_context) {
        return;
    }
    tool_context_ = p_context;
    clear_cache();
}

void IdeamGizmoStyleResolver::clear_cache() {
    material_cache_.clear();
}

int IdeamGizmoStyleResolver::_semantic_role_to_design_role_id(GizmoSemanticRole p_role) noexcept {
    // Map gizmo semantic roles onto hierarchical `SemanticRoleCategory` bitpaths so
    // `Theme3DResource::resolve_cascaded_style()` can reskin gizmos via standard CSS cascade rules.
    constexpr uint32_t kGizmoRoleBase = 0x07000000u; // Dedicated 0x07xxxxxx Gizmo family
    return static_cast<int>(kGizmoRoleBase | (static_cast<uint32_t>(p_role) << 8));
}

godot::Color IdeamGizmoStyleResolver::_default_color_for_role(GizmoSemanticRole p_role) noexcept {
    switch (p_role) {
        case GIZMO_HANDLE_SCAFFOLD:       return godot::Color(0.98f, 0.84f, 0.28f, 0.96f);
        case GIZMO_HANDLE_SPLINE_ANCHOR:  return godot::Color(0.28f, 0.86f, 0.98f, 0.96f);
        case GIZMO_HANDLE_SPLINE_TANGENT: return godot::Color(0.62f, 0.94f, 0.68f, 0.92f);
        case GIZMO_HANDLE_FEATURE_MAJOR:  return godot::Color(0.98f, 0.56f, 0.22f, 0.96f);
        case GIZMO_HANDLE_FEATURE_DETAIL: return godot::Color(0.95f, 0.36f, 0.72f, 0.96f);
        case GIZMO_HANDLE_SOCKET_MALE:    return godot::Color(0.26f, 0.78f, 0.98f, 0.95f);
        case GIZMO_HANDLE_SOCKET_FEMALE:  return godot::Color(0.98f, 0.52f, 0.26f, 0.95f);
        case GIZMO_LINE_BONE_LINK:        return godot::Color(0.98f, 0.80f, 0.28f, 0.90f);
        case GIZMO_LINE_RULER:            return godot::Color(0.32f, 0.85f, 0.98f, 0.92f);
        case GIZMO_LINE_SEAM_HOLE:        return godot::Color(0.92f, 0.38f, 0.96f, 0.95f);
        case GIZMO_LINE_CLEARANCE:        return godot::Color(0.70f, 0.82f, 0.94f, 0.58f);
        case GIZMO_LINE_AXIS_NORMAL:      return godot::Color(0.34f, 0.92f, 0.46f, 0.95f);
        case GIZMO_LINE_AXIS_TANGENT:     return godot::Color(0.96f, 0.34f, 0.34f, 0.92f);
        case GIZMO_PLUMB_BALANCED:        return godot::Color(0.34f, 0.94f, 0.48f, 0.92f);
        case GIZMO_PLUMB_UNBALANCED:      return godot::Color(0.98f, 0.42f, 0.25f, 0.95f);
        case GIZMO_SNAP_PREVIEW:          return godot::Color(0.30f, 1.00f, 0.74f, 0.98f);
        case GIZMO_LIMIT_REACHED:         return godot::Color(1.00f, 0.28f, 0.22f, 0.98f);
        case GIZMO_LABEL_ANNOTATION:      return godot::Color(0.96f, 0.96f, 0.98f, 0.95f);
        case GIZMO_ROLE_DEFAULT:
        default:                          return godot::Color(0.88f, 0.90f, 0.94f, 0.90f);
    }
}

godot::Color IdeamGizmoStyleResolver::_apply_state_tint(const godot::Color &p_base, uint32_t p_state_flags) noexcept {
    godot::Color c = p_base;
    if ((p_state_flags & GIZMO_STATE_LIMIT_REACHED) != 0u) {
        c = c.lerp(godot::Color(1.0f, 0.24f, 0.18f, c.a), 0.72f);
    } else if ((p_state_flags & GIZMO_STATE_ASYMMETRIC_DETAIL) != 0u) {
        c = c.lerp(godot::Color(0.96f, 0.32f, 0.86f, c.a), 0.55f);
    }
    if ((p_state_flags & GIZMO_STATE_DRAGGING) != 0u) {
        c = c.lerp(godot::Color(1.0f, 1.0f, 1.0f, 1.0f), 0.45f);
    } else if ((p_state_flags & GIZMO_STATE_HOVER) != 0u) {
        c = c.lerp(godot::Color(1.0f, 1.0f, 1.0f, 1.0f), 0.25f);
    }
    if ((p_state_flags & GIZMO_STATE_LOCKED) != 0u) {
        const float lum = c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
        c = godot::Color(lum * 0.65f, lum * 0.65f, lum * 0.65f, c.a * 0.65f);
    }
    return c;
}

godot::Ref<godot::Mesh> IdeamGizmoStyleResolver::_get_default_mesh_for_shape(GizmoHandleShapeHint p_shape) const {
    const int32_t key = static_cast<int32_t>(p_shape);
    auto it = default_shape_cache_.find(key);
    if (it != default_shape_cache_.end() && it->second.is_valid()) {
        return it->second;
    }

    godot::Ref<godot::Mesh> mesh;
    switch (p_shape) {
        case HANDLE_SHAPE_CUBE: {
            godot::Ref<godot::BoxMesh> bm;
            bm.instantiate();
            bm->set_size(godot::Vector3(1.6f, 1.6f, 1.6f));
            mesh = bm;
            break;
        }
        case HANDLE_SHAPE_DIAMOND: {
            godot::Ref<godot::SphereMesh> sm;
            sm.instantiate();
            sm->set_radius(1.0f);
            sm->set_height(2.0f);
            sm->set_radial_segments(4);
            sm->set_rings(2);
            mesh = sm;
            break;
        }
        case HANDLE_SHAPE_RING: {
            godot::Ref<godot::TorusMesh> tm;
            tm.instantiate();
            tm->set_inner_radius(0.65f);
            tm->set_outer_radius(1.05f);
            tm->set_rings(16);
            tm->set_ring_segments(8);
            mesh = tm;
            break;
        }
        case HANDLE_SHAPE_CONE: {
            godot::Ref<godot::CylinderMesh> cm;
            cm.instantiate();
            cm->set_top_radius(0.0f);
            cm->set_bottom_radius(0.85f);
            cm->set_height(1.8f);
            cm->set_radial_segments(10);
            mesh = cm;
            break;
        }
        case HANDLE_SHAPE_DISC: {
            godot::Ref<godot::CylinderMesh> dm;
            dm.instantiate();
            dm->set_top_radius(1.0f);
            dm->set_bottom_radius(1.0f);
            dm->set_height(0.25f);
            dm->set_radial_segments(14);
            mesh = dm;
            break;
        }
        case HANDLE_SHAPE_SPHERE:
        default: {
            godot::Ref<godot::SphereMesh> sm;
            sm.instantiate();
            sm->set_radius(1.0f);
            sm->set_height(2.0f);
            sm->set_radial_segments(12);
            sm->set_rings(6);
            mesh = sm;
            break;
        }
    }
    default_shape_cache_[key] = mesh;
    return mesh;
}

godot::Color IdeamGizmoStyleResolver::resolve_color(
    GizmoSemanticRole p_role,
    uint32_t p_state_flags
) const {
    return _apply_state_tint(_default_color_for_role(p_role), p_state_flags);
}

ResolvedGizmoStyle IdeamGizmoStyleResolver::resolve_style(
    GizmoSemanticRole p_role,
    uint32_t p_state_flags,
    GizmoHandleShapeHint p_shape,
    GizmoDepthTestMode p_depth_test
) const {
    ResolvedGizmoStyle out{};
    out.color       = resolve_color(p_role, p_state_flags);
    out.depth_test  = p_depth_test;
    out.handle_mesh = _get_default_mesh_for_shape(p_shape);

    if ((p_state_flags & GIZMO_STATE_DRAGGING) != 0u) {
        out.handle_scale = static_cast<real_t>(1.28);
    } else if ((p_state_flags & GIZMO_STATE_HOVER) != 0u) {
        out.handle_scale = static_cast<real_t>(1.15);
    }

    // 1. Check active `Theme3DResource` on `ToolContext` for diegetic reskinning overrides.
    if (tool_context_.is_valid() && tool_context_->get_active_theme_3d().is_valid()) {
        if (auto *theme_3d = godot::Object::cast_to<Theme3DResource>(tool_context_->get_active_theme_3d().ptr())) {
            const int role_id = _semantic_role_to_design_role_id(p_role);
            godot::Ref<Style3DResource> style = theme_3d->resolve_cascaded_style(role_id, 0);
            if (style.is_valid()) {
                if (style->get_mesh_asset().is_valid()) {
                    out.handle_mesh = style->get_mesh_asset();
                }
                if (style->get_material_override().is_valid()) {
                    out.material = style->get_material_override();
                }
                const godot::Vector3 s = style->get_target_extents_scale();
                out.handle_scale *= std::max(static_cast<real_t>(0.1), (s.x + s.y + s.z) / static_cast<real_t>(3.0));
            }
        }
    }

    // 2. Fallback to cached unshaded StandardMaterial3D if no diegetic material override was provided.
    if (out.material.is_null()) {
        const uint64_t cache_key =
            (static_cast<uint64_t>(p_role) & 0xFFFFULL) |
            ((static_cast<uint64_t>(p_state_flags) & 0xFFFFULL) << 16) |
            ((static_cast<uint64_t>(p_depth_test) & 0xFFULL) << 32);
        auto it = material_cache_.find(cache_key);
        if (it != material_cache_.end() && it->second.is_valid()) {
            out.material = it->second;
        } else {
            godot::Ref<godot::StandardMaterial3D> mat;
            mat.instantiate();
            mat->set_shading_mode(godot::BaseMaterial3D::SHADING_MODE_UNSHADED);
            mat->set_albedo(out.color);
            mat->set_transparency(out.color.a < 0.99f
                ? godot::BaseMaterial3D::TRANSPARENCY_ALPHA
                : godot::BaseMaterial3D::TRANSPARENCY_DISABLED);
            mat->set_flag(godot::BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, p_depth_test == DEPTH_TEST_OVERLAY);
            mat->set_flag(godot::BaseMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
            mat->set_render_priority(p_depth_test == DEPTH_TEST_OVERLAY ? 10 : 0);
            material_cache_[cache_key] = mat;
            out.material = mat;
        }
    }

    return out;
}

} // namespace ideam::godot_ext
