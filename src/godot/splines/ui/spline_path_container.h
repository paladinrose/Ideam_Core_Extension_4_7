#pragma once

/**
 * spline_path_container.h
 * Curved 2D Layout Container (`SplinePathContainer`, `godot::Container`) in `src/godot/splines/ui/`.
 *
 * Implements Section 3.3 of the Ideam Core Extension Master Plan:
 *   - Positions and rotates child `Control` nodes along a `Spline2DResource` with optional
 *     `Spline1DResource` scale (`scale_falloff_1d`) and opacity (`opacity_falloff_1d`) envelopes.
 *   - Ideal for curved card hands (`IdeamGames`), radial/arc menus, carousel inventories,
 *     and narrative choice wheels (`IdeamNarratives`).
 *   - Supports uniform span distribution, center-clustered card-hand fan spacing, continuous
 *     carousel `scroll_offset` (`wrap` aware), tangent rotation alignment, and interactive
 *     hover elevation/scale boost.
 */

#include "../spline_1d/spline_1d_resource.h"
#include "../spline_2d/spline_2d_resource.h"

#include <godot_cpp/classes/container.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

enum SplinePathDistributionMode : int32_t {
    SPLINE_PATH_DISTRIBUTE_UNIFORM         = 0, // Spread evenly across [margin_start, 1 - margin_end]
    SPLINE_PATH_DISTRIBUTE_CENTER_CLUSTER  = 1, // Cluster around t = 0.5 with `cluster_step_percent`
    SPLINE_PATH_DISTRIBUTE_ARC_LENGTH      = 2  // Sample unweighted (uniform arc-length approximation)
};

class SplinePathContainer : public godot::Container {
    GDCLASS(SplinePathContainer, godot::Container)

private:
    godot::Ref<Spline2DResource> path_spline_2d_;
    godot::Ref<Spline1DResource> scale_falloff_1d_;
    godot::Ref<Spline1DResource> opacity_falloff_1d_;

    SplinePathDistributionMode distribution_mode_ = SPLINE_PATH_DISTRIBUTE_UNIFORM;
    bool normalize_to_container_rect_             = true;
    bool align_rotation_to_tangent_               = true;
    real_t rotation_offset_degrees_               = static_cast<real_t>(0.0);

    real_t margin_start_percent_ = static_cast<real_t>(0.08);
    real_t margin_end_percent_   = static_cast<real_t>(0.08);
    real_t cluster_step_percent_ = static_cast<real_t>(0.12);
    real_t scroll_offset_        = static_cast<real_t>(0.0);

    bool show_debug_curve_     = false;
    godot::Color debug_color_  = godot::Color(0.38f, 0.82f, 1.0f, 0.55f);

    void _on_spline_changed();
    void _reflow_children();
    [[nodiscard]] godot::Vector2 _map_spline_point_to_local(
        const godot::Vector2 &p_spline_pt,
        const godot::Rect2 &p_spline_bounds
    ) const;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    SplinePathContainer();
    ~SplinePathContainer() override;

    void _draw() override;

    void set_path_spline_2d(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_path_spline_2d() const { return path_spline_2d_; }

    void set_scale_falloff_1d(const godot::Ref<Spline1DResource> &p_curve);
    [[nodiscard]] godot::Ref<Spline1DResource> get_scale_falloff_1d() const { return scale_falloff_1d_; }

    void set_opacity_falloff_1d(const godot::Ref<Spline1DResource> &p_curve);
    [[nodiscard]] godot::Ref<Spline1DResource> get_opacity_falloff_1d() const { return opacity_falloff_1d_; }

    void set_distribution_mode(SplinePathDistributionMode p_mode);
    [[nodiscard]] SplinePathDistributionMode get_distribution_mode() const noexcept { return distribution_mode_; }

    void set_normalize_to_container_rect(bool p_normalize);
    [[nodiscard]] bool get_normalize_to_container_rect() const noexcept { return normalize_to_container_rect_; }

    void set_align_rotation_to_tangent(bool p_align);
    [[nodiscard]] bool get_align_rotation_to_tangent() const noexcept { return align_rotation_to_tangent_; }

    void set_rotation_offset_degrees(real_t p_deg);
    [[nodiscard]] real_t get_rotation_offset_degrees() const noexcept { return rotation_offset_degrees_; }

    void set_margin_start_percent(real_t p_margin);
    [[nodiscard]] real_t get_margin_start_percent() const noexcept { return margin_start_percent_; }

    void set_margin_end_percent(real_t p_margin);
    [[nodiscard]] real_t get_margin_end_percent() const noexcept { return margin_end_percent_; }

    void set_cluster_step_percent(real_t p_step);
    [[nodiscard]] real_t get_cluster_step_percent() const noexcept { return cluster_step_percent_; }

    void set_scroll_offset(real_t p_offset);
    [[nodiscard]] real_t get_scroll_offset() const noexcept { return scroll_offset_; }

    void set_show_debug_curve(bool p_show);
    [[nodiscard]] bool get_show_debug_curve() const noexcept { return show_debug_curve_; }

    [[nodiscard]] real_t get_child_spline_percent(int p_visible_child_index, int p_visible_count) const noexcept;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplinePathDistributionMode);