#pragma once

/**
 * spline_1d_resource.h
 * Concrete 1D Spline Resource wrapping `ideam::core::Spline1D<real_t>`.
 *
 * Establishes the canonical pattern for all dimensional SplineResources (1D-4D):
 *   - Order-independent packed-array `.tres` serialization (`points`, `back_handles`, `fore_handles`,
 *     plus inherited `handle_flags`, `section_types`, `section_weights`, `wrap`).
 *   - Atomic point, handle, and section mutators paired with `commit_*` Undo/Redo counterparts.
 *   - Direct 1D evaluation (`sample`, `sample_section`, `sample_derivative`, `sample_second_derivative`),
 *     multi-modal inverse root-finding (`get_percents_at_value`), and preset builders (`create_zero_to_one`,
 *     `create_one_to_zero`, `create_equidistant`).
 */

#include "../spline_resource.h"
#include "../../../core/splines/spline_1d.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class Spline1DResource : public SplineResource {
    GDCLASS(Spline1DResource, SplineResource)

private:
    using CoreSpline = core::Spline1D<real_t>;
    using Vec1       = typename CoreSpline::VecType;

    CoreSpline core_spline;

    // Staging caches to ensure order-independent property deserialization from .tres files
    godot::PackedFloat32Array pending_back_handles;
    godot::PackedFloat32Array pending_fore_handles;
    godot::PackedByteArray pending_handle_flags;
    godot::PackedByteArray pending_section_types;
    godot::PackedFloat32Array pending_section_weights;

    void _apply_pending_arrays();

protected:
    static void _bind_methods();

public:
    Spline1DResource();
    ~Spline1DResource() override;

    // --- Direct C++ Core Accessors (For Controls, Nodes & SplineDOD) ---
    [[nodiscard]] const CoreSpline &get_core_spline() const noexcept { return core_spline; }
    [[nodiscard]] CoreSpline &get_core_spline_mut() noexcept { return core_spline; }

    // ========================================================================
    // SplineResource Virtual Overrides
    // ========================================================================

    [[nodiscard]] int get_dimension() const override { return 1; }
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
    // 1D Point & Handle API (Direct + Undo/Redo Commit Counterparts)
    // ========================================================================

    void add_point(
        real_t p_value,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void insert_point(
        int p_index,
        real_t p_value,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );

    void set_point_position(int p_index, real_t p_value);
    [[nodiscard]] real_t get_point_position(int p_index) const;

    void set_point_back_handle(int p_index, real_t p_handle, bool p_enable = true);
    [[nodiscard]] real_t get_point_back_handle(int p_index) const;

    void set_point_fore_handle(int p_index, real_t p_handle, bool p_enable = true);
    [[nodiscard]] real_t get_point_fore_handle(int p_index) const;

    void set_point_handles(
        int p_index,
        real_t p_back_handle,
        bool p_use_back,
        real_t p_fore_handle,
        bool p_use_fore
    );

    // --- Undo/Redo Committed 1D Mutations ---
    void commit_add_point(
        real_t p_value,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void commit_insert_point(
        int p_index,
        real_t p_value,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void commit_set_point_position(int p_index, real_t p_value);
    void commit_set_point_back_handle(int p_index, real_t p_handle, bool p_enable = true);
    void commit_set_point_fore_handle(int p_index, real_t p_handle, bool p_enable = true);

    // ========================================================================
    // 1D Packed Array Serialization Properties
    // ========================================================================

    void set_points_array(const godot::PackedFloat32Array &p_points);
    [[nodiscard]] godot::PackedFloat32Array get_points_array() const;

    void set_back_handles_array(const godot::PackedFloat32Array &p_handles);
    [[nodiscard]] godot::PackedFloat32Array get_back_handles_array() const;

    void set_fore_handles_array(const godot::PackedFloat32Array &p_handles);
    [[nodiscard]] godot::PackedFloat32Array get_fore_handles_array() const;

    // ========================================================================
    // 1D Evaluation, Root-Finding & Solvers
    // ========================================================================

    [[nodiscard]] real_t sample(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] real_t get_point(real_t p_percent, bool p_weighted = true) const {
        return sample(p_percent, p_weighted);
    }
    [[nodiscard]] real_t sample_section(int p_section_index, real_t p_local_t) const;
    [[nodiscard]] real_t sample_derivative(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] real_t sample_section_derivative(int p_section_index, real_t p_local_t) const;
    [[nodiscard]] real_t sample_section_second_derivative(int p_section_index, real_t p_local_t) const;

    [[nodiscard]] godot::PackedFloat32Array get_smoothed_points(int p_max_subdivisions = 10) const;

    /**
     * Multi-modal inverse solver: returns all normalized percentages `t` in [0, 1]
     * where `f(t) == p_value`.
     */
    [[nodiscard]] godot::PackedFloat32Array get_percents_at_value(
        real_t p_value,
        int p_samples = 10,
        bool p_weighted = true
    ) const;

    [[nodiscard]] real_t get_min_value(bool p_use_smoothed = false) const;
    [[nodiscard]] real_t get_max_value(bool p_use_smoothed = false) const;
    [[nodiscard]] real_t get_closest_value_on_spline(real_t p_target_value, int p_samples = 10) const;

    // ========================================================================
    // 1D Presets & Static Factory Builders
    // ========================================================================

    void setup_zero_to_one();
    void setup_one_to_zero();
    void setup_equidistant(int p_count = 4, SplineCurveType p_type = CURVE_LINEAR);

    [[nodiscard]] static godot::Ref<Spline1DResource> create_zero_to_one();
    [[nodiscard]] static godot::Ref<Spline1DResource> create_one_to_zero();
    [[nodiscard]] static godot::Ref<Spline1DResource> create_equidistant(
        int p_count = 4,
        SplineCurveType p_type = CURVE_LINEAR
    );
};

} // namespace ideam::godot_ext