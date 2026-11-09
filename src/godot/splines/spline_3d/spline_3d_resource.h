#pragma once

/**
 * spline_3d_resource.h
 * Concrete 3D Spline Resource wrapping `ideam::core::Spline3D<real_t>`.
 *
 * Bridges Tier 2 `Spline3D` and Tier 1 `ParallelTransport3D` (Wang Double-Reflection
 * Rotation-Minimizing Bishop Frames) into Godot:
 *   - Order-independent `.tres` serialization (`PackedVector3Array` for `points`, `back_handles`,
 *     and `fore_handles`, plus inherited `handle_flags`, `section_types`, `section_weights`, `wrap`).
 *   - 3D point and handle mutators paired with `commit_*` Undo/Redo counterparts.
 *   - Parametric 3D evaluation (`sample`, `sample_derivative`, `get_tangent`, `get_transform_at_percent`)
 *     and twist-free Bishop frame sequence generation (`compute_bishop_transforms`).
 *   - 3D spatial queries (`get_aabb`, `get_closest_point_on_spline`, `get_closest_percent_on_spline`),
 *     bounding-box centered transformations (`rotate_spline`, `scale_spline`), and 3D shape presets
 *     (`setup_line`, `setup_circle_xz`, `setup_helix`).
 */

#include "../spline_resource.h"
#include "../../../core/splines/spline_3d.h"

#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class Spline3DResource : public SplineResource {
    GDCLASS(Spline3DResource, SplineResource)

private:
    using CoreSpline = core::Spline3D<real_t>;
    using Vec3       = typename CoreSpline::VecType;

    CoreSpline core_spline;

    // Staging caches for order-independent .tres property deserialization
    godot::PackedVector3Array pending_back_handles;
    godot::PackedVector3Array pending_fore_handles;
    godot::PackedByteArray pending_handle_flags;
    godot::PackedByteArray pending_section_types;
    godot::PackedFloat32Array pending_section_weights;

    void _apply_pending_arrays();

    [[nodiscard]] static inline Vec3 _to_glm(const godot::Vector3 &p_v) noexcept {
        return Vec3(
            static_cast<real_t>(p_v.x),
            static_cast<real_t>(p_v.y),
            static_cast<real_t>(p_v.z)
        );
    }
    [[nodiscard]] static inline godot::Vector3 _to_godot(const Vec3 &p_v) noexcept {
        return godot::Vector3(
            static_cast<float>(p_v.x),
            static_cast<float>(p_v.y),
            static_cast<float>(p_v.z)
        );
    }

protected:
    static void _bind_methods();

public:
    Spline3DResource();
    ~Spline3DResource() override;

    // --- Direct C++ Core Accessors (For Controls, Nodes, 3D Gizmos & SplineDOD) ---
    [[nodiscard]] const CoreSpline &get_core_spline() const noexcept { return core_spline; }
    [[nodiscard]] CoreSpline &get_core_spline_mut() noexcept { return core_spline; }

    // ========================================================================
    // SplineResource Virtual Overrides
    // ========================================================================

    [[nodiscard]] int get_dimension() const override { return 3; }
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
    // 3D Point & Handle API (Direct + Undo/Redo Commit Counterparts)
    // ========================================================================

    void add_point(
        const godot::Vector3 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void insert_point(
        int p_index,
        const godot::Vector3 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );

    void set_point_position(int p_index, const godot::Vector3 &p_position);
    [[nodiscard]] godot::Vector3 get_point_position(int p_index) const;

    void set_point_back_handle(int p_index, const godot::Vector3 &p_handle, bool p_enable = true);
    [[nodiscard]] godot::Vector3 get_point_back_handle(int p_index) const;

    void set_point_fore_handle(int p_index, const godot::Vector3 &p_handle, bool p_enable = true);
    [[nodiscard]] godot::Vector3 get_point_fore_handle(int p_index) const;

    void set_point_handles(
        int p_index,
        const godot::Vector3 &p_back_handle,
        bool p_use_back,
        const godot::Vector3 &p_fore_handle,
        bool p_use_fore
    );

    // --- Undo/Redo Committed 3D Mutations ---
    void commit_add_point(
        const godot::Vector3 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void commit_insert_point(
        int p_index,
        const godot::Vector3 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void commit_set_point_position(int p_index, const godot::Vector3 &p_position);
    void commit_set_point_back_handle(int p_index, const godot::Vector3 &p_handle, bool p_enable = true);
    void commit_set_point_fore_handle(int p_index, const godot::Vector3 &p_handle, bool p_enable = true);

    // ========================================================================
    // 3D Packed Array Serialization Properties
    // ========================================================================

    void set_points_array(const godot::PackedVector3Array &p_points);
    [[nodiscard]] godot::PackedVector3Array get_points_array() const;

    void set_back_handles_array(const godot::PackedVector3Array &p_handles);
    [[nodiscard]] godot::PackedVector3Array get_back_handles_array() const;

    void set_fore_handles_array(const godot::PackedVector3Array &p_handles);
    [[nodiscard]] godot::PackedVector3Array get_fore_handles_array() const;

    // ========================================================================
    // 3D Parametric Evaluation, Bishop RMF Frames & Proximity Solvers
    // ========================================================================

    [[nodiscard]] godot::Vector3 sample(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Vector3 get_point(real_t p_percent, bool p_weighted = true) const {
        return sample(p_percent, p_weighted);
    }
    [[nodiscard]] godot::Vector3 sample_section(int p_section_index, real_t p_local_t) const;
    [[nodiscard]] godot::Vector3 sample_derivative(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Vector3 sample_section_derivative(int p_section_index, real_t p_local_t) const;
    [[nodiscard]] godot::Vector3 sample_section_second_derivative(int p_section_index, real_t p_local_t) const;

    [[nodiscard]] godot::Vector3 get_tangent(real_t p_percent, bool p_weighted = true) const;

    /**
     * Evaluates an orthonormal 3D Transform at `p_percent` (where `-Z` points along the forward
     * tangent matching Godot's 3D convention, `Y` aligns with the normal/up hint, and `X` is binormal).
     */
    [[nodiscard]] godot::Transform3D get_transform_at_percent(
        real_t p_percent,
        bool p_weighted = true,
        const godot::Vector3 &p_up_hint = godot::Vector3(0.0f, 1.0f, 0.0f)
    ) const;

    /**
     * Generates a twist-free sequence of Rotation-Minimizing Bishop Frame Transform3Ds
     * along the spline using Wang's Double-Reflection algorithm (with closed-loop holonomy
     * correction when `wrap` is enabled).
     */
    [[nodiscard]] godot::TypedArray<godot::Transform3D> compute_bishop_transforms(
        int p_subdivisions_per_section = 10,
        const godot::Vector3 &p_initial_up = godot::Vector3(0.0f, 1.0f, 0.0f)
    ) const;

    [[nodiscard]] godot::PackedVector3Array get_smoothed_points(int p_max_subdivisions = 10) const;

    [[nodiscard]] godot::AABB get_aabb(bool p_use_smoothed = true) const;
    [[nodiscard]] godot::Vector3 get_closest_point_on_spline(
        const godot::Vector3 &p_target,
        int p_samples = 10
    ) const;
    [[nodiscard]] real_t get_closest_percent_on_spline(
        const godot::Vector3 &p_target,
        int p_samples = 10,
        bool p_weighted = false
    ) const;

    // ========================================================================
    // 3D Spatial Transformations & Shape Presets
    // ========================================================================

    void rotate_spline(const godot::Vector3 &p_euler_radians);
    void scale_spline(const godot::Vector3 &p_scale);
    void commit_rotate_spline(const godot::Vector3 &p_euler_radians);
    void commit_scale_spline(const godot::Vector3 &p_scale);

    void setup_line(
        const godot::Vector3 &p_start = godot::Vector3(0.0f, 0.0f, 0.0f),
        const godot::Vector3 &p_end = godot::Vector3(0.0f, 0.0f, -10.0f),
        int p_points = 4,
        SplineCurveType p_type = CURVE_CATMULL_ROM
    );
    void setup_circle_xz(
        real_t p_radius = static_cast<real_t>(5.0),
        int p_points = 8
    );
    void setup_helix(
        real_t p_radius = static_cast<real_t>(5.0),
        real_t p_height = static_cast<real_t>(10.0),
        real_t p_turns = static_cast<real_t>(2.0),
        int p_points_per_turn = 8
    );

    [[nodiscard]] static godot::Ref<Spline3DResource> create_line(
        const godot::Vector3 &p_start = godot::Vector3(0.0f, 0.0f, 0.0f),
        const godot::Vector3 &p_end = godot::Vector3(0.0f, 0.0f, -10.0f),
        int p_points = 4,
        SplineCurveType p_type = CURVE_CATMULL_ROM
    );
    [[nodiscard]] static godot::Ref<Spline3DResource> create_circle_xz(
        real_t p_radius = static_cast<real_t>(5.0),
        int p_points = 8
    );
    [[nodiscard]] static godot::Ref<Spline3DResource> create_helix(
        real_t p_radius = static_cast<real_t>(5.0),
        real_t p_height = static_cast<real_t>(10.0),
        real_t p_turns = static_cast<real_t>(2.0),
        int p_points_per_turn = 8
    );
};

} // namespace ideam::godot_ext