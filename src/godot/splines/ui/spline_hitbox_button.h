#pragma once

/**
 * spline_hitbox_button.h
 * Exact Curved-Polygon Hitbox UI Controls (`SplineRegionControl` & `SplineHitboxButton`)
 * in `src/godot/splines/ui/`.
 *
 * Implements Section 3.3 of the Ideam Core Extension Master Plan:
 *   - Overrides `_has_point(const Vector2 &p_point) const` via `Spline2DResource::contains_point()`
 *     so non-rectangular, organic, or Escher/Voronoi-bounded 2D UI regions and buttons respond
 *     with exact closed-curve hit-testing.
 *   - Renders ear-clipped triangulated `Spline2DResource` interiors with state colors (`Normal`,
 *     `Hover`, `Pressed`, `Disabled`) driven either by explicit `Color` properties OR sampled
 *     from a bound `Spline4DResource` (`ColorSplineControl` gradient).
 */

#include "../spline_2d/spline_2d_resource.h"
#include "../spline_4d/spline_4d_resource.h"

#include <godot_cpp/classes/base_button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

/**
 * SplineHitboxButton (`godot::BaseButton`)
 * Interactive button whose clickable area and visual shape are defined by a closed `Spline2DResource`.
 */
class SplineHitboxButton : public godot::BaseButton {
    GDCLASS(SplineHitboxButton, godot::BaseButton)

private:
    godot::Ref<Spline2DResource> hitbox_spline_2d_;
    godot::Ref<Spline4DResource> state_gradient_4d_;

    int curve_subdivisions_         = 12;
    bool fit_spline_to_control_     = true;
    real_t border_width_            = static_cast<real_t>(2.0);
    godot::String label_text_       = "";
    int font_size_                  = 14;

    godot::Color normal_color_      = godot::Color(0.20f, 0.42f, 0.68f, 0.88f);
    godot::Color hover_color_       = godot::Color(0.32f, 0.64f, 0.94f, 0.95f);
    godot::Color pressed_color_     = godot::Color(0.96f, 0.74f, 0.24f, 0.98f);
    godot::Color disabled_color_    = godot::Color(0.28f, 0.30f, 0.34f, 0.55f);
    godot::Color border_color_      = godot::Color(0.75f, 0.88f, 1.0f, 0.92f);
    godot::Color font_color_        = godot::Color(0.96f, 0.98f, 1.0f, 1.0f);

    void _on_spline_changed();
    void _ensure_default_spline();
    [[nodiscard]] godot::Vector2 _local_to_spline_space(const godot::Vector2 &p_local) const;
    [[nodiscard]] godot::Vector2 _spline_to_local_space(const godot::Vector2 &p_spline_pt) const;
    [[nodiscard]] godot::Color _resolve_current_fill_color() const;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    SplineHitboxButton();
    ~SplineHitboxButton() override;

    [[nodiscard]] bool _has_point(const godot::Vector2 &p_point) const override;
    void _draw() override;

    void set_hitbox_spline_2d(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_hitbox_spline_2d() const { return hitbox_spline_2d_; }

    void set_state_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient);
    [[nodiscard]] godot::Ref<Spline4DResource> get_state_gradient_4d() const { return state_gradient_4d_; }

    void set_curve_subdivisions(int p_subdivs);
    [[nodiscard]] int get_curve_subdivisions() const noexcept { return curve_subdivisions_; }

    void set_fit_spline_to_control(bool p_fit);
    [[nodiscard]] bool get_fit_spline_to_control() const noexcept { return fit_spline_to_control_; }

    void set_border_width(real_t p_width);
    [[nodiscard]] real_t get_border_width() const noexcept { return border_width_; }

    void set_label_text(const godot::String &p_text);
    [[nodiscard]] godot::String get_label_text() const { return label_text_; }

    void set_normal_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_normal_color() const noexcept { return normal_color_; }

    void set_hover_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_hover_color() const noexcept { return hover_color_; }

    void set_pressed_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_pressed_color() const noexcept { return pressed_color_; }

    void set_disabled_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_disabled_color() const noexcept { return disabled_color_; }

    void set_border_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_border_color() const noexcept { return border_color_; }
};

/**
 * SplineRegionControl
 * Lightweight alias/subclass of `SplineHitboxButton` for interactive 2D map territories,
 * board spaces, and narrative location zones.
 */
class SplineRegionControl : public SplineHitboxButton {
    GDCLASS(SplineRegionControl, SplineHitboxButton)

private:
    int region_id_ = 0;

protected:
    static void _bind_methods();

public:
    SplineRegionControl() = default;
    ~SplineRegionControl() override = default;

    void set_region_id(int p_id);
    [[nodiscard]] int get_region_id() const noexcept { return region_id_; }
};

} // namespace ideam::godot_ext