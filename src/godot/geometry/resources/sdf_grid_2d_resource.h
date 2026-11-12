#pragma once

/**
 * sdf_grid_2d_resource.h
 * Concrete Godot Resource wrapping `ideam::core::SDFMath<real_t>`
 * (`src/godot/geometry/resources/`).
 *
 * Represents a contiguous row-major 2D Signed Distance Field or Chladni standing-wave
 * scalar grid (`width * height`) mapped over `world_bounds`.
 *
 * Directly bridges:
 *   - `CymaticMap` (`src/godot/cymatics/`) standing-wave nodal evaluation (`evaluate_chladni_field`).
 *   - Smooth boolean CSG (`op_smooth_union`, `op_smooth_subtraction`, `op_smooth_intersection`)
 *     across circles, boxes, segments, and `Polygon2DResource` / `Spline2DResource` contours.
 *   - Marching Squares iso-contour extraction (`extract_iso_contour_segments` &
 *     `extract_iso_contour_polygons`) into `Polygon2DResource` contours or `GeometryDOD`
 *     `GEOM_SDF_GRID_FLOAT32` buffers.
 *   - Zero-copy quantized visualization in `HeatmapGrid` (`to_heatmap_byte_array` / `sync_to_heatmap_grid`).
 */

#include "../geometry_resource.h"
#include "polygon_2d_resource.h"
#include "../../controls/heatmap_grid.h"
#include "../../../core/geometry/sdf_math.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <vector>

namespace ideam::godot_ext {

class SDFGrid2DResource : public GeometryResource {
    GDCLASS(SDFGrid2DResource, GeometryResource)

private:
    using SDF  = core::SDFMath<float>;
    using Vec2 = typename SDF::Vec2;

    godot::Vector2i resolution_ = godot::Vector2i(64, 64);
    godot::Rect2 world_bounds_  = godot::Rect2(-5.0f, -5.0f, 10.0f, 10.0f);
    float iso_value_            = 0.0f;
    std::vector<float> grid_values_;

    void _ensure_allocated();
    [[nodiscard]] Vec2 _cell_to_world(int p_x, int p_y) const noexcept;
    [[nodiscard]] Vec2 _world_to_grid_coord(const godot::Vector2 &p_world) const noexcept;
    [[nodiscard]] static float _combine_sample(
        float p_existing,
        float p_candidate,
        GeometrySDFCombineMode p_mode,
        float p_smooth_k
    ) noexcept;

protected:
    static void _bind_methods();

public:
    SDFGrid2DResource();
    ~SDFGrid2DResource() override = default;

    // --- Direct C++ Core Accessor (Zero-Copy Float32 Span for GeometryDOD) ---
    [[nodiscard]] std::span<const float> get_grid_span() const noexcept { return grid_values_; }
    [[nodiscard]] std::span<float> get_grid_span_mut() noexcept { return grid_values_; }

    // --- GeometryResource Overrides ---
    [[nodiscard]] int get_geometry_dimension() const override { return 2; }
    [[nodiscard]] godot::Rect2 get_bounds_2d() const override { return world_bounds_; }
    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    // --- Grid Configuration & Raw Buffer Serialization ---
    void set_resolution(const godot::Vector2i &p_resolution);
    [[nodiscard]] godot::Vector2i get_resolution() const noexcept { return resolution_; }

    void set_world_bounds(const godot::Rect2 &p_bounds);
    [[nodiscard]] godot::Rect2 get_world_bounds() const noexcept { return world_bounds_; }

    void set_iso_value(float p_iso_value);
    [[nodiscard]] float get_iso_value() const noexcept { return iso_value_; }

    void set_grid_values(const godot::PackedFloat32Array &p_values);
    [[nodiscard]] godot::PackedFloat32Array get_grid_values() const;

    void clear_field(float p_fill_value = 1000.0f);

    // --- 2D Analytical SDF & Cymatic Field Stampers ---
    void stamp_circle(
        const godot::Vector2 &p_center,
        float p_radius,
        GeometrySDFCombineMode p_mode = SDF_COMBINE_UNION,
        float p_smooth_k = 0.0f
    );
    void stamp_box(
        const godot::Vector2 &p_center,
        const godot::Vector2 &p_half_extents,
        GeometrySDFCombineMode p_mode = SDF_COMBINE_UNION,
        float p_smooth_k = 0.0f
    );
    void stamp_segment(
        const godot::Vector2 &p_a,
        const godot::Vector2 &p_b,
        float p_radius = 0.1f,
        GeometrySDFCombineMode p_mode = SDF_COMBINE_UNION,
        float p_smooth_k = 0.0f
    );
    void stamp_polygon(
        const godot::PackedVector2Array &p_polygon,
        GeometrySDFCombineMode p_mode = SDF_COMBINE_UNION,
        float p_smooth_k = 0.0f
    );
    void evaluate_chladni_field(
        float p_mode_n,
        float p_mode_m,
        float p_amp_a = 1.0f,
        float p_amp_b = -1.0f
    );

    // --- Sampling, Gradients & Marching Squares Iso-Contour Extraction ---
    [[nodiscard]] float sample_distance(const godot::Vector2 &p_world_pos) const;
    [[nodiscard]] godot::Vector2 sample_gradient(const godot::Vector2 &p_world_pos) const;
    [[nodiscard]] godot::Vector2 get_min_max_distance() const noexcept;

    /**
     * Returns pairs of `(start, end)` world-space points (`2 * segment_count`) extracted
     * at `p_iso_value` using `SDFMath::extract_marching_squares_contours`.
     */
    [[nodiscard]] godot::PackedVector2Array extract_iso_contour_segments(float p_iso_value = 0.0f) const;

    /**
     * Stitches Marching Squares iso-contour segments at `p_iso_value` into ordered
     * `Polygon2DResource` loops/polylines for cross-plugin bridging (`IdeamSplines` / `IdeamTiles`).
     */
    [[nodiscard]] godot::TypedArray<Polygon2DResource> extract_iso_contour_polygons(
        float p_iso_value = 0.0f,
        float p_weld_tolerance = 1e-3f
    ) const;

    // --- HeatmapGrid Companion Bridge (`src/godot/controls/heatmap_grid.h`) ---
    [[nodiscard]] godot::PackedByteArray to_heatmap_byte_array(
        float p_min_val = -2.0f,
        float p_max_val = 2.0f
    ) const;
    void sync_to_heatmap_grid(
        HeatmapGrid *p_heatmap,
        float p_min_val = -2.0f,
        float p_max_val = 2.0f
    ) const;

    // --- IdeamUndoRedo Commit Wrappers ---
    void commit_set_resolution(const godot::Vector2i &p_resolution);
    void commit_set_world_bounds(const godot::Rect2 &p_bounds);
    void commit_set_iso_value(float p_iso_value);
    void commit_clear_field(float p_fill_value = 1000.0f);
    void commit_stamp_circle(
        const godot::Vector2 &p_center,
        float p_radius,
        GeometrySDFCombineMode p_mode = SDF_COMBINE_UNION,
        float p_smooth_k = 0.0f
    );
    void commit_stamp_box(
        const godot::Vector2 &p_center,
        const godot::Vector2 &p_half_extents,
        GeometrySDFCombineMode p_mode = SDF_COMBINE_UNION,
        float p_smooth_k = 0.0f
    );
    void commit_stamp_segment(
        const godot::Vector2 &p_a,
        const godot::Vector2 &p_b,
        float p_radius = 0.1f,
        GeometrySDFCombineMode p_mode = SDF_COMBINE_UNION,
        float p_smooth_k = 0.0f
    );
    void commit_stamp_polygon(
        const godot::PackedVector2Array &p_polygon,
        GeometrySDFCombineMode p_mode = SDF_COMBINE_UNION,
        float p_smooth_k = 0.0f
    );
    void commit_evaluate_chladni_field(
        float p_mode_n,
        float p_mode_m,
        float p_amp_a = 1.0f,
        float p_amp_b = -1.0f
    );
};

} // namespace ideam::godot_ext