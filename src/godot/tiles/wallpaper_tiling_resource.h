#pragma once

/**
 * wallpaper_tiling_resource.h
 * Concrete Godot Resource wrapping `ideam::core::IsohedralTiling2D<real_t>`
 * (`src/godot/tiles/`).
 *
 * Unites all three authoring layers for a single symmetry group / prototile:
 *   1. **17 Crystallographic Wallpaper Groups (`P1`..`P6M`) & 81 Isohedral Tilings (`IH01`..`IH93`)**:
 *      Supports switching either by high-level `WallpaperGroupType` (with legacy `Topology.cs`
 *      shape presets `Quad`, `Brick`, `RightTriangle`, `EquilateralTriangle`, `FlatTriangle`,
 *      `TallTriangle`, `Rhombus`) or by exact Tactile `tactile_ih_type` (`1..93`) and `0..6`
 *      topological vertex parameters.
 *   2. **Parametric Symmetry Element Handles (`AxesOfSymmetry`, `PointsOfRotation`, `Glides`)**:
 *      Preserves the explicit symmetry generators from `old_unity_code/Topology/Topology.cs`
 *      as `PackedVector2Array` properties for interactive viewport/canvas editing.
 *   3. **Lateral Handshake with `IdeamSplines` (`edge_splines`)**:
 *      Holds a `TypedArray<Spline2DResource>` (one per `edge_shape_id`) that deforms the
 *      prototile edges while automatically enforcing Tactile `J / U / S / I` symmetry rules.
 */

#include "tiling_resource.h"
#include "../../core/tiles/isohedral_tiling_2d.h"
#include "../splines/spline_2d/spline_2d_resource.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3i.hpp>

namespace ideam::godot_ext {

class WallpaperTilingResource : public TilingResource {
    GDCLASS(WallpaperTilingResource, TilingResource)

private:
    using CoreTiling = core::IsohedralTiling2D<real_t>;
    using Vec2       = typename CoreTiling::Vec2;

    CoreTiling core_tiling;
    godot::TypedArray<Spline2DResource> edge_splines;

    void _sync_edge_splines_to_core();
    void _disconnect_edge_spline_signals();
    void _connect_edge_spline_signals();
    void _on_edge_spline_changed();

    [[nodiscard]] static inline Vec2 _to_glm(const godot::Vector2 &p_v) noexcept {
        return Vec2(static_cast<real_t>(p_v.x), static_cast<real_t>(p_v.y));
    }
    [[nodiscard]] static inline godot::Vector2 _to_godot(const Vec2 &p_v) noexcept {
        return godot::Vector2(static_cast<float>(p_v.x), static_cast<float>(p_v.y));
    }

protected:
    static void _bind_methods();

public:
    WallpaperTilingResource();
    ~WallpaperTilingResource() override;

    // --- Direct C++ Core Accessors (For `CompositeTilingResource`, Controls, Nodes & `TileDOD`) ---
    [[nodiscard]] const CoreTiling &get_core_tiling() const noexcept { return core_tiling; }
    [[nodiscard]] CoreTiling &get_core_tiling_mut() noexcept { return core_tiling; }

    // ========================================================================
    // TilingResource Virtual Overrides
    // ========================================================================

    [[nodiscard]] int get_vertex_count() const override;
    [[nodiscard]] int get_aspect_count() const override;
    [[nodiscard]] int get_edge_shape_count() const override;

    [[nodiscard]] godot::PackedVector2Array get_prototile_outline(int p_subdivisions = 12) const override;
    [[nodiscard]] godot::PackedInt32Array get_prototile_triangles(int p_subdivisions = 12) const override;

    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    // ========================================================================
    // Symmetry Group, Tactile IH Type & Shape Preset Properties
    // ========================================================================

    void set_authoring_mode(TilingAuthoringModeType p_mode);
    [[nodiscard]] TilingAuthoringModeType get_authoring_mode() const;

    void set_wallpaper_group(WallpaperGroupType p_group);
    [[nodiscard]] WallpaperGroupType get_wallpaper_group() const;

    void set_tactile_ih_type(int p_ih_type);
    [[nodiscard]] int get_tactile_ih_type() const;

    void set_shape_preset(TilingShapePresetType p_preset);
    [[nodiscard]] TilingShapePresetType get_shape_preset() const;

    [[nodiscard]] TilingLatticeFamily get_lattice_family() const;
    [[nodiscard]] godot::String get_iucr_symbol() const;
    [[nodiscard]] godot::String get_orbifold_symbol() const;
    [[nodiscard]] int get_rotational_order() const;
    [[nodiscard]] bool has_reflection() const;
    [[nodiscard]] bool has_glide_reflection() const;

    // ========================================================================
    // Tactile Parameters (0..6 Vertex Parameters)
    // ========================================================================

    [[nodiscard]] int get_parameter_count() const;
    void set_tactile_parameters(const godot::PackedFloat32Array &p_params);
    [[nodiscard]] godot::PackedFloat32Array get_tactile_parameters() const;

    void set_tactile_parameter(int p_index, real_t p_value);
    [[nodiscard]] real_t get_tactile_parameter(int p_index) const;

    // ========================================================================
    // Lattice Basis Transform & Explicit Symmetry Handles (`Topology.cs`)
    // ========================================================================

    void set_lattice_scale(const godot::Vector2 &p_scale);
    [[nodiscard]] godot::Vector2 get_lattice_scale() const;

    void set_lattice_rotation_radians(real_t p_rotation_rad);
    [[nodiscard]] real_t get_lattice_rotation_radians() const;

    void set_lattice_origin(const godot::Vector2 &p_origin);
    [[nodiscard]] godot::Vector2 get_lattice_origin() const;

    [[nodiscard]] godot::Vector2 get_translation_vector_t1() const;
    [[nodiscard]] godot::Vector2 get_translation_vector_t2() const;

    void set_axes_of_symmetry(const godot::PackedVector2Array &p_axes);
    [[nodiscard]] godot::PackedVector2Array get_axes_of_symmetry() const;

    void set_points_of_rotation(const godot::PackedVector2Array &p_points);
    [[nodiscard]] godot::PackedVector2Array get_points_of_rotation() const;

    void set_glides(const godot::PackedVector2Array &p_glides);
    [[nodiscard]] godot::PackedVector2Array get_glides() const;

    // ========================================================================
    // Custom `Spline2DResource` Escher Edge Curves (`J`, `U`, `S`, `I`)
    // ========================================================================

    void set_edge_splines(const godot::TypedArray<Spline2DResource> &p_splines);
    [[nodiscard]] godot::TypedArray<Spline2DResource> get_edge_splines() const { return edge_splines; }

    void set_edge_spline(int p_edge_shape_id, const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_edge_spline(int p_edge_shape_id) const;
    [[nodiscard]] TilingEdgeShapeType get_edge_shape_constraint(int p_edge_shape_id) const;
    void clear_edge_splines();

    // ========================================================================
    // Undo/Redo Committed Mutations
    // ========================================================================

    void commit_set_wallpaper_group(WallpaperGroupType p_group);
    void commit_set_tactile_ih_type(int p_ih_type);
    void commit_set_shape_preset(TilingShapePresetType p_preset);
    void commit_set_tactile_parameter(int p_index, real_t p_value);
    void commit_set_lattice_scale(const godot::Vector2 &p_scale);
    void commit_set_lattice_rotation_radians(real_t p_rotation_rad);
    void commit_set_lattice_origin(const godot::Vector2 &p_origin);
    void commit_set_edge_spline(int p_edge_shape_id, const godot::Ref<Spline2DResource> &p_spline);

    // ========================================================================
    // Direct Lattice, Aspect & Quotient Queries
    // ========================================================================

    [[nodiscard]] godot::PackedVector2Array get_base_prototile_vertices() const;
    [[nodiscard]] godot::Transform2D get_cell_transform_2d(int p_t1, int p_t2, int p_aspect) const;
    [[nodiscard]] godot::PackedVector2Array get_cell_world_polygon(
        int p_t1,
        int p_t2,
        int p_aspect,
        bool p_use_deformed_edges = true,
        int p_subdivisions = 12
    ) const;
    [[nodiscard]] int get_cell_colour(int p_t1, int p_t2, int p_aspect) const;

    /**
     * Locates the cell `(t1, t2, aspect)` containing `p_world_point` in O(1) time.
     * Returns `Vector3i(t1, t2, aspect)` (or `Vector3i(0, 0, -1)` if invalid).
     */
    [[nodiscard]] godot::Vector3i locate_cell_at_world_point(const godot::Vector2 &p_world_point) const;

    /**
     * Steps across local edge `p_edge_slot` of `(p_t1, p_t2, p_aspect)` using the precomputed
     * quotient-graph table. Returns `Vector3i(next_t1, next_t2, next_aspect)`.
     */
    [[nodiscard]] godot::Vector3i step_to_neighbor(int p_t1, int p_t2, int p_aspect, int p_edge_slot) const;

    // --- Static Catalogue Helpers ---
    [[nodiscard]] static godot::PackedInt32Array get_all_valid_ih_types();
    [[nodiscard]] static godot::PackedInt32Array get_ih_types_for_group(WallpaperGroupType p_group);
};

} // namespace ideam::godot_ext