#pragma once

/**
 * surface_patch_resource.h
 * Abstract base Godot `Resource` for all parametric 3D surface patches (`src/godot/surfaces/`).
 *
 * Shared contract implemented by `NurbsPatch3DResource` and `CoonsPatch3DResource`:
 *   - Object-owned `IdeamUndoRedo` integration (matching `Narreme` and `SplineResource`).
 *   - `IdeamTutorials` help topic override (`help_topic_id`) and `semantic_tags` hooks.
 *   - Unified `(u, v)` surface coordinate queries: `sample_position(u, v)`, `sample_normal(u, v)`,
 *     `sample_frame(u, v)` (returning an orthonormal `Transform3D` where X = tangent_u, Y = normal,
 *     Z = tangent_v, origin = position), and `offset_from_frame(u, v, d_normal, d_u, d_v)`.
 *   - Exact vertex-budget LOD quad-mesh generation (`build_array_mesh(lod_level)` and
 *     `export_quad_mesh_data(lod_level)`).
 */

#include "../../core/surfaces/surface_common.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

enum SurfacePatchBoundaryEdge : int32_t {
    SURFACE_EDGE_NORTH_V0 = 0,
    SURFACE_EDGE_SOUTH_V1 = 1,
    SURFACE_EDGE_WEST_U0  = 2,
    SURFACE_EDGE_EAST_U1  = 3
};

class SurfacePatchResource : public godot::Resource {
    GDCLASS(SurfacePatchResource, godot::Resource)

protected:
    godot::String patch_name_;
    int base_u_divs_ = 12;
    int base_v_divs_ = 12;
    godot::Vector2 uv_scale_ = godot::Vector2(1.0f, 1.0f);
    godot::Color base_color_ = godot::Color(0.84f, 0.90f, 0.96f, 1.0f);

    // Foundational Undo/Redo & IdeamTutorials hooks (matching IdeamNarratives)
    godot::Ref<IdeamUndoRedo> undo_redo_;
    godot::StringName help_topic_id_;
    godot::PackedStringArray semantic_tags_;

    static void _bind_methods();

    [[nodiscard]] static inline glm::vec<3, real_t> _to_glm(const godot::Vector3 &p_v) noexcept {
        return glm::vec<3, real_t>(static_cast<real_t>(p_v.x), static_cast<real_t>(p_v.y), static_cast<real_t>(p_v.z));
    }

    [[nodiscard]] static inline godot::Vector3 _to_godot(const glm::vec<3, real_t> &p_v) noexcept {
        return godot::Vector3(static_cast<float>(p_v.x), static_cast<float>(p_v.y), static_cast<float>(p_v.z));
    }

    [[nodiscard]] static godot::Transform3D _frame_to_transform(const core::SurfaceFrame3DPOD<real_t> &p_frame) noexcept;

    [[nodiscard]] static godot::Ref<godot::ArrayMesh> _quad_mesh_to_array_mesh(
        const core::SurfaceQuadMesh3D<real_t> &p_mesh,
        const godot::Vector2 &p_uv_scale,
        const godot::Color &p_color
    );

    [[nodiscard]] static godot::Dictionary _quad_mesh_to_dictionary(
        const core::SurfaceQuadMesh3D<real_t> &p_mesh
    );

public:
    SurfacePatchResource() = default;
    ~SurfacePatchResource() override = default;

    // --- Identity, Subdivision & Visual Defaults ---
    void set_patch_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_patch_name() const { return patch_name_; }

    void set_base_u_divs(int p_divs);
    [[nodiscard]] int get_base_u_divs() const noexcept { return base_u_divs_; }

    void set_base_v_divs(int p_divs);
    [[nodiscard]] int get_base_v_divs() const noexcept { return base_v_divs_; }

    void set_uv_scale(const godot::Vector2 &p_scale);
    [[nodiscard]] godot::Vector2 get_uv_scale() const noexcept { return uv_scale_; }

    void set_base_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_base_color() const noexcept { return base_color_; }

    // --- Undo/Redo & Tutorials Hooks ---
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) { undo_redo_ = p_ur; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const;

    void set_semantic_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    // --- Virtual Core Evaluation & Mesh Contract ---
    [[nodiscard]] virtual bool is_patch_valid() const { return false; }
    [[nodiscard]] virtual godot::String get_patch_type_label() const { return "SurfacePatch"; }

    [[nodiscard]] virtual core::SurfaceFrame3DPOD<real_t> sample_core_frame(
        real_t p_u,
        real_t p_v,
        uint32_t p_patch_id = 0u
    ) const {
        (void)p_u;
        (void)p_v;
        (void)p_patch_id;
        return core::SurfaceFrame3DPOD<real_t>{};
    }

    [[nodiscard]] virtual core::SurfaceQuadMesh3D<real_t> tessellate_core_quad_mesh(
        int p_lod_level = 0,
        uint32_t p_patch_id = 0u
    ) const {
        (void)p_lod_level;
        (void)p_patch_id;
        return core::SurfaceQuadMesh3D<real_t>{};
    }

    [[nodiscard]] virtual godot::Dictionary get_snapshot_state() const { return godot::Dictionary(); }
    virtual void apply_snapshot_state(const godot::Dictionary &p_state) { (void)p_state; }

    // --- Exposed Godot Surface Coordinate Queries & Mesh Builders ---
    [[nodiscard]] godot::Vector3 sample_position(real_t p_u, real_t p_v) const;
    [[nodiscard]] godot::Vector3 sample_normal(real_t p_u, real_t p_v) const;
    [[nodiscard]] godot::Transform3D sample_frame(real_t p_u, real_t p_v) const;
    [[nodiscard]] godot::Vector3 offset_from_frame(
        real_t p_u,
        real_t p_v,
        real_t p_d_normal,
        real_t p_d_tangent_u = static_cast<real_t>(0.0),
        real_t p_d_tangent_v = static_cast<real_t>(0.0)
    ) const;

    [[nodiscard]] godot::Ref<godot::ArrayMesh> build_array_mesh(int p_lod_level = 0) const;
    [[nodiscard]] godot::Dictionary export_quad_mesh_data(int p_lod_level = 0) const;
    [[nodiscard]] godot::Vector2i get_resolved_lod_divs(int p_lod_level = 0) const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SurfacePatchBoundaryEdge);
