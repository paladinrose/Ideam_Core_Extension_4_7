#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../gameplay/gameplay_perspective.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: PerspectiveFrustumVisualizer renders a dual-panel 2D architectural schematic
 * (Top-Down XZ Plan View + Side-Profile YZ Elevation View) of a `GameplayPerspective`
 * resource using immediate-mode `CanvasItem` drawing.
 *
 * It illustrates:
 * 1. Top-Down Plan View: Player origin, lateral shoulder offset (`SHOULDER_LEFT`,
 *    `SHOULDER_CENTER`, `SHOULDER_RIGHT`), spring-arm distance (`arm_length` vs.
 *    `aim_arm_length`), and horizontal `fov` vs. `aim_fov` frustum cones (or parallel
 *    orthogonal bounds when `PROJECTION_ORTHOGONAL` is selected).
 * 2. Side-Profile Elevation View: Vertical `pivot_offset.y`, default pitch angle
 *    (`default_pitch_degrees`), and pitch constraint arc (`min_pitch_degrees` ..
 *    `max_pitch_degrees`).
 */
class PerspectiveFrustumVisualizer : public godot::Control {
    GDCLASS(PerspectiveFrustumVisualizer, godot::Control)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::Ref<GameplayPerspective> perspective;

    bool preview_aim_state = false;
    bool show_side_elevation = true;
    bool show_labels = true;

    // Custom Theme Color Overrides (alpha == 0 uses Theme fallback)
    godot::Color custom_bg_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_frustum_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_aim_frustum_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_rig_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);

    void _on_perspective_changed();
    void _draw_top_down_view(
        const godot::Rect2 &p_rect,
        const godot::Color &p_bg_col,
        const godot::Color &p_frustum_col,
        const godot::Color &p_aim_col,
        const godot::Color &p_rig_col,
        const godot::Color &p_grid_col,
        const godot::Color &p_text_col
    );
    void _draw_side_elevation_view(
        const godot::Rect2 &p_rect,
        const godot::Color &p_bg_col,
        const godot::Color &p_frustum_col,
        const godot::Color &p_rig_col,
        const godot::Color &p_grid_col,
        const godot::Color &p_text_col
    );

public:
    PerspectiveFrustumVisualizer();
    ~PerspectiveFrustumVisualizer() override;

    virtual godot::Vector2 _get_minimum_size() const override;

    // --- Resource & Display Configuration ---
    void set_perspective(const godot::Ref<GameplayPerspective> &p_perspective);
    godot::Ref<GameplayPerspective> get_perspective() const;

    void set_preview_aim_state(bool p_aiming);
    bool get_preview_aim_state() const;

    void set_show_side_elevation(bool p_show);
    bool get_show_side_elevation() const;

    void set_show_labels(bool p_show);
    bool get_show_labels() const;

    // --- Custom Color Overrides ---
    void set_custom_bg_color(const godot::Color &p_color);
    godot::Color get_custom_bg_color() const;

    void set_custom_frustum_color(const godot::Color &p_color);
    godot::Color get_custom_frustum_color() const;

    void set_custom_aim_frustum_color(const godot::Color &p_color);
    godot::Color get_custom_aim_frustum_color() const;

    void set_custom_rig_color(const godot::Color &p_color);
    godot::Color get_custom_rig_color() const;
};

} // namespace ideam::godot_ext