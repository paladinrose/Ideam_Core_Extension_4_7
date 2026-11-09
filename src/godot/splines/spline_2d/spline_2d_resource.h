#pragma once

/**
 * spline_2d_resource.h
 * Concrete 2D Spline & Closed-Polygon Resource wrapping `ideam::core::Spline2D<real_t>`.
 *
 * Bridges Tier 2 `Spline2D` and Tier 1 `PolygonMath2D` / `CSGMath2D` into Godot:
 *   - Order-independent `.tres` serialization (`PackedVector2Array` for `points`, `back_handles`,
 *     and `fore_handles`, plus inherited `handle_flags`, `section_types`, `section_weights`, `wrap`).
 *   - 2D point and handle mutators paired with `commit_*` Undo/Redo counterparts.
 *   - Parametric 2D evaluation (`sample`, `sample_derivative`, `get_tangent`, `get_normal`,
 *     `get_transform_at_percent`) and proximity projection (`get_closest_point_on_spline`,
 *     `get_closest_percent_on_spline`).
 *   - Closed-polygon & computational geometry queries (`contains_point`, `get_area`, `get_abs_area`,
 *     `get_perimeter`, `get_centroid`, `get_bounding_rect`, ear-clipping `triangulate`,
 *     Sutherland-Hodgman `clip_with_convex_polygon`).
 *   - 2D spatial transforms (`rotate_spline`, `scale_spline`) and shape presets (`setup_rectangle`,
 *     `setup_regular_polygon`, `setup_star`, `setup_circle`).
 */

#include "../spline_resource.h"
#include "../../../core/splines/spline_2d.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class Spline2DResource : public SplineResource {
    GDCLASS(Spline2DResource, SplineResource)

private:
    using CoreSpline = core::Spline2D<real_t>;
    using Vec2       = typename CoreSpline::VecType;

    CoreSpline core_spline;

    // Staging caches for order-independent .tres property deserialization
    godot::PackedVector2Array pending_back_handles;
    godot::PackedVector2Array pending_fore_handles;
    godot::PackedByteArray pending_handle_flags;
    godot::PackedByteArray pending_section_types;
    godot::PackedFloat32Array pending_section_weights;

    void _apply_pending_arrays();

    [[nodiscard]] static inline Vec2 _to_glm(const godot::Vector2 &p_v) noexcept {
        return Vec2(static_cast<real_t>(p_v.x), static_cast<real_t>(p_v.y));
    }
    [[nodiscard]] static inline godot::Vector2 _to_godot(const Vec2 &p_v) noexcept {
        return godot::Vector2(static_cast<float>(p_v.x), static_cast<float>(p_v.y));
    }

protected:
    static void _bind_methods();

public:
    Spline2DResource();
    ~Spline2DResource() override;

    // --- Direct C++ Core Accessors (For Controls, Nodes, Gizmos & SplineDOD) ---
    [[nodiscard]] const CoreSpline &get_core_spline() const noexcept { return core_spline; }
    [[nodiscard]] CoreSpline &get_core_spline_mut() noexcept { return core_spline; }

    // ========================================================================
    // SplineResource Virtual Overrides
    // ========================================================================

    [[nodiscard]] int get_dimension() const override { return 2; }
    [[nodiscard]] int get_point_count() const override;
    [[nodiscard]] int get_length() const override;
    [[nodiscard]] int get_section_count() const override;

    void set_wrap(bool p_wrap) override;
    [[nodiscard]] bool get_wrap() const override;

    void set_default_curve_type(SplineCurveType p_type) override;
    [[nodiscard]] SplineCurveType get_default_curve_type() const override;

    void set_default_weight(real_t p_weight) override;
    [[nodiscard]] real_t get_default_weight() const override;

    [[nodiscard]] real_t get_total_weight() const override;
    [[nodiscard]] real_t get_total_spline_length(
        real_t p_sample_percent = static_cast<real_t>(0.05),
        bool p_force_recalculate = false
    ) const override;

    void remove_point(int p_index) override;
    void clear() override;

    void set_point_use_back_handle(int p_index, bool p_enable) override;
    [[nodiscard]] bool get_point_use_back_handle(int p_index) const override;

    void set_point_use_fore_handle(int p_index, bool p_enable) override;
    [[nodiscard]] bool get_point_use_fore_handle(int p_index) const override;

    void set_section_curve_type(int p_section_index, SplineCurveType p_type) override;
    [[nodiscard]] SplineCurveType get_section_curve_type(int p_section_index) const override;

    void set_section_weight(int p_section_index, real_t p_weight) override;
    [[nodiscard]] real_t get_section_weight(int p_section_index) const override;

    [[nodiscard]] real_t get_section_length(int p_section_index) const override;
    [[nodiscard]] real_t section_local_to_global_percent(
        int p_section_index,
        real_t p_local_t,
        bool p_weighted = true
    ) const override;
    [[nodiscard]] godot::Vector2 get_section_at_percent(
        real_t p_percent,
        bool p_weighted = true
    ) const override;

    [[nodiscard]] godot::Variant get_point_variant(real_t p_percent, bool p_weighted = true) const override;
    [[nodiscard]] godot::Variant get_derivative_variant(real_t p_percent, bool p_weighted = true) const override;
    [[nodiscard]] godot::Variant get_tangent_variant(real_t p_percent, bool p_weighted = true) const override;

    [[nodiscard]] godot::PackedByteArray get_handle_flags_array() const override;
    void set_handle_flags_array(const godot::PackedByteArray &p_flags) override;

    [[nodiscard]] godot::PackedByteArray get_section_types_array() const override;
    void set_section_types_array(const godot::PackedByteArray &p_types) override;

    [[nodiscard]] godot::PackedFloat32Array get_section_weights_array() const override;
    void set_section_weights_array(const godot::PackedFloat32Array &p_weights) override;

    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    // ========================================================================
    // 2D Point & Handle API (Direct + Undo/Redo Commit Counterparts)
    // ========================================================================

    void add_point(
        const godot::Vector2 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void insert_point(
        int p_index,
        const godot::Vector2 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );

    void set_point_position(int p_index, const godot::Vector2 &p_position);
    [[nodiscard]] godot::Vector2 get_point_position(int p_index) const;

    void set_point_back_handle(int p_index, const godot::Vector2 &p_handle, bool p_enable = true);
    [[nodiscard]] godot::Vector2 get_point_back_handle(int p_index) const;

    void set_point_fore_handle(int p_index, const godot::Vector2 &p_handle, bool p_enable = true);
    [[nodiscard]] godot::Vector2 get_point_fore_handle(int p_index) const;

    void set_point_handles(
        int p_index,
        const godot::Vector2 &p_back_handle,
        bool p_use_back,
        const godot::Vector2 &p_fore_handle,
        bool p_use_fore
    );

    // --- Undo/Redo Committed 2D Mutations ---
    void commit_add_point(
        const godot::Vector2 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void commit_insert_point(
        int p_index,
        const godot::Vector2 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void commit_set_point_position(int p_index, const godot::Vector2 &p_position);
    void commit_set_point_back_handle(int p_index, const godot::Vector2 &p_handle, bool p_enable = true);
    void commit_set_point_fore_handle(int p_index, const godot::Vector2 &p_handle, bool p_enable = true);

    // ========================================================================
    // 2D Packed Array Serialization Properties
    // ========================================================================

    void set_points_array(const godot::PackedVector2Array &p_points);
    [[nodiscard]] godot::PackedVector2Array get_points_array() const;

    void set_back_handles_array(const godot::PackedVector2Array &p_handles);
    [[nodiscard]] godot::PackedVector2Array get_back_handles_array() const;

    void set_fore_handles_array(const godot::PackedVector2Array &p_handles);
    [[nodiscard]] godot::PackedVector2Array get_fore_handles_array() const;

    // ========================================================================
    // 2D Parametric Evaluation, Frenet Frame & Proximity Solvers
    // ========================================================================

    [[nodiscard]] godot::Vector2 sample(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Vector2 get_point(real_t p_percent, bool p_weighted = true) const {
        return sample(p_percent, p_weighted);
    }
    [[nodiscard]] godot::Vector2 sample_section(int p_section_index, real_t p_local_t) const;
    [[nodiscard]] godot::Vector2 sample_derivative(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Vector2 sample_section_derivative(int p_section_index, real_t p_local_t) const;
    [[nodiscard]] godot::Vector2 sample_section_second_derivative(int p_section_index, real_t p_local_t) const;

    [[nodiscard]] godot::Vector2 get_tangent(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Vector2 get_normal(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Transform2D get_transform_at_percent(real_t p_percent, bool p_weighted = true) const;

    [[nodiscard]] godot::PackedVector2Array get_smoothed_points(int p_max_subdivisions = 10) const;

    [[nodiscard]] godot::Rect2 get_bounding_rect(bool p_use_smoothed = true) const;
    [[nodiscard]] godot::Vector2 get_closest_point_on_spline(
        const godot::Vector2 &p_target,
        int p_samples = 10
    ) const;
    [[nodiscard]] real_t get_closest_percent_on_spline(
        const godot::Vector2 &p_target,
        int p_samples = 10,
        bool p_weighted = false
    ) const;

    // ========================================================================
    // 2D Closed-Polygon & Computational Geometry Operations
    // ========================================================================

    [[nodiscard]] bool contains_point(const godot::Vector2 &p_point, int p_subdivisions = 10) const;
    [[nodiscard]] real_t get_area(int p_subdivisions = 10) const;
    [[nodiscard]] real_t get_abs_area(int p_subdivisions = 10) const;
    [[nodiscard]] real_t get_perimeter(int p_subdivisions = 10) const;
    [[nodiscard]] godot::Vector2 get_centroid(int p_subdivisions = 10) const;
    [[nodiscard]] godot::PackedInt32Array triangulate(int p_subdivisions = 10) const;
    [[nodiscard]] godot::PackedVector2Array clip_with_convex_polygon(
        const godot::PackedVector2Array &p_convex_clip,
        int p_subdivisions = 10
    ) const;

    void rotate_spline(real_t p_angle_rad);
    void scale_spline(const godot::Vector2 &p_scale);
    void commit_rotate_spline(real_t p_angle_rad);
    void commit_scale_spline(const godot::Vector2 &p_scale);

    // ========================================================================
    // 2D Shape Presets & Static Factory Builders
    // ========================================================================

    void setup_from_vertices(
        const godot::PackedVector2Array &p_vertices,
        bool p_wrap = true,
        SplineCurveType p_type = CURVE_LINEAR
    );
    void setup_rectangle(
        const godot::Vector2 &p_center = godot::Vector2(0.0f, 0.0f),
        const godot::Vector2 &p_size = godot::Vector2(100.0f, 100.0f)
    );
    void setup_regular_polygon(
        int p_sides = 6,
        real_t p_radius = static_cast<real_t>(50.0),
        real_t p_angle_offset = static_cast<real_t>(0.0)
    );
    void setup_star(
        int p_sides = 5,
        real_t p_outer_radius = static_cast<real_t>(50.0),
        real_t p_inner_radius = static_cast<real_t>(25.0),
        real_t p_angle_offset = static_cast<real_t>(0.0)
    );
    void setup_circle(
        real_t p_radius = static_cast<real_t>(50.0),
        int p_points = 8
    );

    [[nodiscard]] static godot::Ref<Spline2DResource> create_rectangle(
        const godot::Vector2 &p_center = godot::Vector2(0.0f, 0.0f),
        const godot::Vector2 &p_size = godot::Vector2(100.0f, 100.0f)
    );
    [[nodiscard]] static godot::Ref<Spline2DResource> create_regular_polygon(
        int p_sides = 6,
        real_t p_radius = static_cast<real_t>(50.0),
        real_t p_angle_offset = static_cast<real_t>(0.0)
    );
    [[nodiscard]] static godot::Ref<Spline2DResource> create_star(
        int p_sides = 5,
        real_t p_outer_radius = static_cast<real_t>(50.0),
        real_t p_inner_radius = static_cast<real_t>(25.0),
        real_t p_angle_offset = static_cast<real_t>(0.0)
    );
    [[nodiscard]] static godot::Ref<Spline2DResource> create_circle(
        real_t p_radius = static_cast<real_t>(50.0),
        int p_points = 8
    );
};

} // namespace ideam::godot_ext