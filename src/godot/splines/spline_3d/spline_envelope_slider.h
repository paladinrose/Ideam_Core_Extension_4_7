#pragma once

/**
 * spline_envelope_slider.h
 * Curved 2D Track & Non-Linear 1D Envelope Slider (`SplineEnvelopeSlider`, `godot::Range`)
 * in `src/godot/splines/ui/`.
 *
 * Implements Section 3.3 of the Ideam Core Extension Master Plan:
 *   - Slider/gauge whose visual track follows an arbitrary 2D curve (`Spline2DResource`) and
 *     maps normalized progress `t in [0, 1]` through a `Spline1DResource` response curve
 *     (`response_envelope_1d`), using `Spline1DResource::get_percents_at_value()` for inverse
 *     programmatic lookup when `set_value` is called externally.
 *   - Supports interactive dragging along the 2D curve via `Spline2DResource::get_closest_percent_on_spline()`,
 *     optional `Spline4DResource` (`track_gradient_4d`) progress coloring, and live value badge display.
 */

#include "../spline_1d/spline_1d_resource.h"
#include "../spline_2d/spline_2d_resource.h"
#include "../spline_4d/spline_4d_resource.h"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/range.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class SplineEnvelopeSlider : public godot::Range {
    GDCLASS(SplineEnvelopeSlider, godot::Range)

private:
    godot::Ref<Spline2DResource> track_spline_2d_;
    godot::Ref<Spline1DResource> response_envelope_1d_;
    godot::Ref<Spline4DResource> track_gradient_4d_;

    real_t track_percent_    = static_cast<real_t>(0.0);
    real_t track_thickness_  = static_cast<real_t>(6.0);
    real_t thumb_radius_     = static_cast<real_t>(9.0);
    bool show_value_label_   = true;
    bool is_dragging_thumb_  = false;
    bool is_syncing_value_   = false;

    godot::Color track_bg_color_   = godot::Color(0.18f, 0.21f, 0.27f, 0.92f);
    godot::Color track_fill_color_ = godot::Color(0.32f, 0.80f, 0.98f, 0.96f);
    godot::Color thumb_color_      = godot::Color(1.0f, 0.84f, 0.28f, 1.0f);

    void _ensure_default_splines();
    void _on_spline_changed();
    void _update_percent_from_mouse(const godot::Vector2 &p_local_mouse);
    [[nodiscard]] godot::Vector2 _spline_to_local(const godot::Vector2 &p_spline_pt) const;
    [[nodiscard]] godot::Vector2 _local_to_spline(const godot::Vector2 &p_local_pt) const;

protected:
    static void _bind_methods();
    void _value_changed(double p_new_value) override;

public:
    SplineEnvelopeSlider();
    ~SplineEnvelopeSlider() override;

    void _draw() override;
    void _gui_input(const godot::Ref<godot::InputEvent> &p_event) override;

    void set_track_spline_2d(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_track_spline_2d() const { return track_spline_2d_; }

    void set_response_envelope_1d(const godot::Ref<Spline1DResource> &p_envelope);
    [[nodiscard]] godot::Ref<Spline1DResource> get_response_envelope_1d() const { return response_envelope_1d_; }

    void set_track_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient);
    [[nodiscard]] godot::Ref<Spline4DResource> get_track_gradient_4d() const { return track_gradient_4d_; }

    void set_track_percent(real_t p_percent);
    [[nodiscard]] real_t get_track_percent() const noexcept { return track_percent_; }

    void set_track_thickness(real_t p_thickness);
    [[nodiscard]] real_t get_track_thickness() const noexcept { return track_thickness_; }

    void set_thumb_radius(real_t p_radius);
    [[nodiscard]] real_t get_thumb_radius() const noexcept { return thumb_radius_; }

    void set_show_value_label(bool p_show);
    [[nodiscard]] bool get_show_value_label() const noexcept { return show_value_label_; }
};

} // namespace ideam::godot_ext