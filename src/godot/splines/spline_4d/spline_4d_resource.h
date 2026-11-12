#pragma once

/**
 * spline_4d_resource.h
 * Concrete 4D Spline Resource wrapping `ideam::core::Spline4D<real_t>`.
 *
 * Supports both primary 4D workflows in the Ideam ecosystem:
 *   1. **3D Spatial Path + 4th-Dimension Attribute Track (`W`)**:
 *      Where `(X, Y, Z)` is the 3D curve position and `W` encodes cross-section scale,
 *      radius, speed, or banking roll in radians (`compute_bishop_transforms` with `p_w_is_roll`).
 *   2. **4-Channel Color / Perceptual Gradient Curve (`RGBA` / `HSVA`)**:
 *      Where `(X, Y, Z, W)` encodes 4-channel color stops (`sample_color`, `setup_color_gradient`).
 */

#include "../spline_resource.h"
#include "../../../core/splines/spline_4d.h"

#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector4_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

namespace ideam::godot_ext {

class Spline4DResource : public SplineResource {
    GDCLASS(Spline4DResource, SplineResource)

private:
    using CoreSpline = core::Spline4D<real_t>;
    using Vec4       = typename CoreSpline::VecType;
    using Vec3       = typename CoreSpline::Vec3Type;

    CoreSpline core_spline;
    bool color_gradient_mode = false;

    // Staging caches for order-independent .tres property deserialization
    godot::PackedVector4Array pending_back_handles;
    godot::PackedVector4Array pending_fore_handles;
    godot::PackedByteArray pending_handle_flags;
    godot::PackedByteArray pending_section_types;
    godot::PackedFloat32Array pending_section_weights;

    void _apply_pending_arrays();

    [[nodiscard]] static inline Vec4 _to_glm(const godot::Vector4 &p_v) noexcept {
        return Vec4(
            static_cast<real_t>(p_v.x),
            static_cast<real_t>(p_v.y),
            static_cast<real_t>(p_v.z),
            static_cast<real_t>(p_v.w)
        );
    }
    [[nodiscard]] static inline godot::Vector4 _to_godot(const Vec4 &p_v) noexcept {
        return godot::Vector4(
            static_cast<float>(p_v.x),
            static_cast<float>(p_v.y),
            static_cast<float>(p_v.z),
            static_cast<float>(p_v.w)
        );
    }
    [[nodiscard]] static inline Vec3 _to_glm3(const godot::Vector3 &p_v) noexcept {
        return Vec3(
            static_cast<real_t>(p_v.x),
            static_cast<real_t>(p_v.y),
            static_cast<real_t>(p_v.z)
        );
    }
    [[nodiscard]] static inline godot::Vector3 _to_godot3(const Vec3 &p_v) noexcept {
        return godot::Vector3(
            static_cast<float>(p_v.x),
            static_cast<float>(p_v.y),
            static_cast<float>(p_v.z)
        );
    }

protected:
    static void _bind_methods();

public:
    Spline4DResource();
    ~Spline4DResource() override;

    // --- Direct C++ Core Accessors (For Controls, Nodes, 4D Gizmos & SplineDOD) ---
    [[nodiscard]] const CoreSpline &get_core_spline() const noexcept { return core_spline; }
    [[nodiscard]] CoreSpline &get_core_spline_mut() noexcept { return core_spline; }

    // ========================================================================
    // SplineResource Virtual Overrides
    // ========================================================================

    [[nodiscard]] int get_dimension() const override { return 4; }
    [[nodiscard]] int get_point_count() const override;
    [[nodiscard]] int get_length() const override;
    [[nodiscard]] int get_section_count() const override;

    void set_color_gradient_mode(bool p_enabled);
    [[nodiscard]] bool get_color_gradient_mode() const noexcept { return color_gradient_mode; }

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
    // 4D Point & Handle API (Direct + Undo/Redo Commit Counterparts)
    // ========================================================================

    void add_point(
        const godot::Vector4 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void insert_point(
        int p_index,
        const godot::Vector4 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );

    void set_point_position(int p_index, const godot::Vector4 &p_position);
    [[nodiscard]] godot::Vector4 get_point_position(int p_index) const;

    void set_point_xyz(int p_index, const godot::Vector3 &p_xyz);
    [[nodiscard]] godot::Vector3 get_point_xyz(int p_index) const;

    void set_point_w(int p_index, real_t p_w);
    [[nodiscard]] real_t get_point_w(int p_index) const;

    void set_point_back_handle(int p_index, const godot::Vector4 &p_handle, bool p_enable = true);
    [[nodiscard]] godot::Vector4 get_point_back_handle(int p_index) const;

    void set_point_fore_handle(int p_index, const godot::Vector4 &p_handle, bool p_enable = true);
    [[nodiscard]] godot::Vector4 get_point_fore_handle(int p_index) const;

    void set_point_handles(
        int p_index,
        const godot::Vector4 &p_back_handle,
        bool p_use_back,
        const godot::Vector4 &p_fore_handle,
        bool p_use_fore
    );

    // --- Undo/Redo Committed 4D Mutations ---
    void commit_add_point(
        const godot::Vector4 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void commit_insert_point(
        int p_index,
        const godot::Vector4 &p_position,
        SplineCurveType p_type = CURVE_NONE,
        real_t p_weight = static_cast<real_t>(-1.0)
    );
    void commit_set_point_position(int p_index, const godot::Vector4 &p_position);
    void commit_set_point_w(int p_index, real_t p_w);
    void commit_set_point_back_handle(int p_index, const godot::Vector4 &p_handle, bool p_enable = true);
    void commit_set_point_fore_handle(int p_index, const godot::Vector4 &p_handle, bool p_enable = true);

    // ========================================================================
    // 4D Packed Array Serialization Properties
    // ========================================================================

    void set_points_array(const godot::PackedVector4Array &p_points);
    [[nodiscard]] godot::PackedVector4Array get_points_array() const;

    void set_back_handles_array(const godot::PackedVector4Array &p_handles);
    [[nodiscard]] godot::PackedVector4Array get_back_handles_array() const;

    void set_fore_handles_array(const godot::PackedVector4Array &p_handles);
    [[nodiscard]] godot::PackedVector4Array get_fore_handles_array() const;

    // ========================================================================
    // 4D Parametric Evaluation, Bishop RMF (XYZ + W) & Color Sampling
    // ========================================================================

    [[nodiscard]] godot::Vector4 sample(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Vector4 get_point(real_t p_percent, bool p_weighted = true) const {
        return sample(p_percent, p_weighted);
    }
    [[nodiscard]] godot::Vector4 sample_section(int p_section_index, real_t p_local_t) const;
    [[nodiscard]] godot::Vector4 sample_derivative(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Vector4 sample_section_derivative(int p_section_index, real_t p_local_t) const;
    [[nodiscard]] godot::Vector4 sample_section_second_derivative(int p_section_index, real_t p_local_t) const;

    [[nodiscard]] godot::Vector4 get_tangent(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Vector3 get_tangent_3d(real_t p_percent, bool p_weighted = true) const;

    /**
     * Evaluates the 4D spline as a `godot::Color`.
     * When `p_hsva_to_rgba` is true (matching `core::Spline4D::create_gradient`), treats
     * `(X=Hue[0..360], Y=Sat[0..1], Z=Val[0..1], W=Alpha[0..1])` and converts to RGBA;
     * otherwise treats `(X, Y, Z, W)` directly as `(R, G, B, A)`.
     */
    [[nodiscard]] godot::Color sample_color(
        real_t p_percent,
        bool p_weighted = true,
        bool p_hsva_to_rgba = false
    ) const;

    /**
     * Evaluates a 3D Transform at `p_percent` from the `XYZ` spatial track and applies `W`
     * as either uniform cross-section scale (`p_w_is_roll == false`) or axial banking roll
     * in radians around the forward tangent (`p_w_is_roll == true`).
     */
    [[nodiscard]] godot::Transform3D get_transform_3d_at_percent(
        real_t p_percent,
        bool p_weighted = true,
        bool p_w_is_roll = false,
        const godot::Vector3 &p_up_hint = godot::Vector3(0.0f, 1.0f, 0.0f)
    ) const;

    /**
     * Generates a twist-free sequence of 3D Rotation-Minimizing Bishop Frame Transform3Ds
     * along `XYZ`, applying `W` as either uniform basis scale (`p_w_is_roll == false`) or
     * banking roll in radians (`p_w_is_roll == true`).
     */
    [[nodiscard]] godot::TypedArray<godot::Transform3D> compute_bishop_transforms(
        int p_subdivisions_per_section = 10,
        const godot::Vector3 &p_initial_up = godot::Vector3(0.0f, 1.0f, 0.0f),
        bool p_w_is_roll = false
    ) const;

    [[nodiscard]] godot::PackedVector4Array get_smoothed_points(int p_max_subdivisions = 10) const;

    [[nodiscard]] godot::AABB get_aabb_3d(bool p_use_smoothed = true) const;
    [[nodiscard]] godot::Vector2 get_w_range(bool p_use_smoothed = true) const;

    [[nodiscard]] godot::Vector4 get_closest_point_on_spline(
        const godot::Vector4 &p_target,
        int p_samples = 10
    ) const;
    [[nodiscard]] real_t get_closest_percent_on_spline(
        const godot::Vector4 &p_target,
        int p_samples = 10,
        bool p_weighted = false
    ) const;

    // ========================================================================
    // Semi-Spatial 3D Transformations (XYZ Transformed, W Preserved) & Presets
    // ========================================================================

    void rotate_spline(const godot::Vector3 &p_euler_radians);
    void scale_spline(const godot::Vector3 &p_scale);
    void commit_rotate_spline(const godot::Vector3 &p_euler_radians);
    void commit_scale_spline(const godot::Vector3 &p_scale);

    void setup_tapered_line(
        const godot::Vector3 &p_start = godot::Vector3(0.0f, 0.0f, 0.0f),
        real_t p_start_w = static_cast<real_t>(1.0),
        const godot::Vector3 &p_end = godot::Vector3(0.0f, 0.0f, -10.0f),
        real_t p_end_w = static_cast<real_t>(0.25),
        int p_points = 4,
        SplineCurveType p_type = CURVE_CATMULL_ROM
    );
    void setup_banked_circle_xz(
        real_t p_radius = static_cast<real_t>(5.0),
        real_t p_bank_roll_rad = static_cast<real_t>(0.35),
        int p_points = 8
    );
    void setup_color_gradient(
        const godot::PackedColorArray &p_colors,
        bool p_convert_to_hsva = false
    );

    [[nodiscard]] static godot::Ref<Spline4DResource> create_tapered_line(
        const godot::Vector3 &p_start = godot::Vector3(0.0f, 0.0f, 0.0f),
        real_t p_start_w = static_cast<real_t>(1.0),
        const godot::Vector3 &p_end = godot::Vector3(0.0f, 0.0f, -10.0f),
        real_t p_end_w = static_cast<real_t>(0.25),
        int p_points = 4,
        SplineCurveType p_type = CURVE_CATMULL_ROM
    );
    [[nodiscard]] static godot::Ref<Spline4DResource> create_banked_circle_xz(
        real_t p_radius = static_cast<real_t>(5.0),
        real_t p_bank_roll_rad = static_cast<real_t>(0.35),
        int p_points = 8
    );
    [[nodiscard]] static godot::Ref<Spline4DResource> create_color_gradient(
        const godot::PackedColorArray &p_colors,
        bool p_convert_to_hsva = false
    );
};

} // namespace ideam::godot_ext