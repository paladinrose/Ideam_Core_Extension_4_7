#pragma once

/**
 * surface_network_resource.h
 * Multi-patch watertight Curve-Network Resource (`src/godot/surfaces/`).
 *
 * Manages an ordered list of `SurfacePatchResource` patches (`NurbsPatch3DResource` and/or
 * `CoonsPatch3DResource`), plus serializable `quad_bridges` and `pole_stitches` descriptors.
 * Provides:
 *   - `sample_frame(patch_id, u, v)` — single-call curvilinear anchoring across any patch.
 *   - `build_array_mesh(lod_level)` — watertight, welded, smooth-normal `ArrayMesh`.
 *   - `export_quad_mesh_data(lod_level)` — raw quad-grid + patch/ring metadata dictionary.
 *   - `get_manifold_stats(lod_level)` — edge-valence diagnostics (watertight / manifold check).
 *   - Object-owned `IdeamUndoRedo` & `IdeamTutorials` help topic hooks.
 */

#include "coons_patch_3d_resource.h"
#include "nurbs_patch_3d_resource.h"
#include "surface_patch_resource.h"
#include "../../core/surfaces/surface_network_3d.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class SurfaceNetworkResource : public godot::Resource {
    GDCLASS(SurfaceNetworkResource, godot::Resource)

private:
    godot::String network_name_;
    godot::TypedArray<SurfacePatchResource> patches_;
    godot::TypedArray<godot::Dictionary> quad_bridges_;
    godot::TypedArray<godot::Dictionary> pole_stitches_;

    real_t weld_epsilon_ = static_cast<real_t>(1e-4);
    bool recompute_smooth_normals_ = true;
    godot::Vector2 uv_scale_ = godot::Vector2(1.0f, 1.0f);
    godot::Color base_color_ = godot::Color(0.84f, 0.90f, 0.96f, 1.0f);

    godot::Ref<IdeamUndoRedo> undo_redo_;
    godot::StringName help_topic_id_;
    godot::PackedStringArray semantic_tags_;

    mutable core::SurfaceNetwork3D<real_t> core_network_;
    mutable bool core_dirty_ = true;

    void _sync_core() const;
    void _connect_patches();
    void _disconnect_patches();
    void _on_patch_changed();

protected:
    static void _bind_methods();

public:
    SurfaceNetworkResource() = default;
    ~SurfaceNetworkResource() override;

    [[nodiscard]] const core::SurfaceNetwork3D<real_t> &get_core_network() const;

    // --- Properties ---
    void set_network_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_network_name() const { return network_name_; }

    void set_patches(const godot::TypedArray<SurfacePatchResource> &p_patches);
    [[nodiscard]] godot::TypedArray<SurfacePatchResource> get_patches() const { return patches_; }

    void set_quad_bridges(const godot::TypedArray<godot::Dictionary> &p_bridges);
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_quad_bridges() const { return quad_bridges_; }

    void set_pole_stitches(const godot::TypedArray<godot::Dictionary> &p_poles);
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_pole_stitches() const { return pole_stitches_; }

    void set_weld_epsilon(real_t p_eps);
    [[nodiscard]] real_t get_weld_epsilon() const noexcept { return weld_epsilon_; }

    void set_recompute_smooth_normals(bool p_smooth);
    [[nodiscard]] bool get_recompute_smooth_normals() const noexcept { return recompute_smooth_normals_; }

    void set_uv_scale(const godot::Vector2 &p_scale);
    [[nodiscard]] godot::Vector2 get_uv_scale() const noexcept { return uv_scale_; }

    void set_base_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_base_color() const noexcept { return base_color_; }

    // --- Undo/Redo & Tutorials ---
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const;

    void set_semantic_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    // --- Ergonomic Network Construction API ---
    int add_patch(const godot::Ref<SurfacePatchResource> &p_patch);
    void remove_patch(int p_index);
    [[nodiscard]] int get_patch_count() const { return static_cast<int>(patches_.size()); }
    [[nodiscard]] godot::Ref<SurfacePatchResource> get_patch(int p_index) const;

    void add_quad_bridge(
        int p_patch_a,
        SurfacePatchBoundaryEdge p_edge_a,
        int p_patch_b,
        SurfacePatchBoundaryEdge p_edge_b,
        bool p_reverse_b = false,
        int p_bridge_rings = 2,
        real_t p_tangent_strength = static_cast<real_t>(0.35)
    );
    void add_pole_stitch(const godot::TypedArray<godot::Dictionary> &p_edges, real_t p_center_normal_bias = static_cast<real_t>(0.0));

    // --- Unified Frame Queries, LOD Mesh Output & Diagnostics ---
    [[nodiscard]] godot::Transform3D sample_frame(int p_patch_id, real_t p_u, real_t p_v) const;
    [[nodiscard]] godot::Vector3 sample_position(int p_patch_id, real_t p_u, real_t p_v) const;
    [[nodiscard]] godot::Vector3 sample_normal(int p_patch_id, real_t p_u, real_t p_v) const;

    [[nodiscard]] godot::Ref<godot::ArrayMesh> build_array_mesh(int p_lod_level = 0) const;
    [[nodiscard]] godot::Dictionary export_quad_mesh_data(int p_lod_level = 0) const;
    [[nodiscard]] godot::Dictionary get_manifold_stats(int p_lod_level = 0) const;
};

} // namespace ideam::godot_ext
