#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../gameplay/gameplay_accessibility_option.h"
#include "../gameplay/gameplay_difficulty.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: DifficultyProfileRadar renders a multi-axis spider/radar polygon chart
 * visualizing `GameplayDifficulty` across its core domains (Combat/Action, Riddle/Puzzle,
 * Stealth/Perception, Survival/Economy, plus any entries in `custom_aspect_levels`).
 *
 * When a `GameplayAccessibilityOption` resource is also bound, the radar overlays a
 * secondary `!important` effective polygon illustrating how motor/cognitive assists
 * (such as `enemy_time_scale` or `player_damage_taken_multiplier`) temper the raw
 * difficulty profile inside the `GameplayStyle` cascade.
 */
class DifficultyProfileRadar : public godot::Control {
    GDCLASS(DifficultyProfileRadar, godot::Control)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::Ref<GameplayDifficulty> difficulty;
    godot::Ref<GameplayAccessibilityOption> accessibility_overlay;

    float max_scalar_radius = 2.0f; // 1.0x Normal sits at 50% radius, 2.0x Nightmare at outer ring
    int concentric_rings = 4;
    bool show_baseline_ring = true;
    bool show_accessibility_overlay = true;
    bool show_axis_labels = true;

    // Custom Theme Color Overrides (alpha == 0 uses Theme fallback)
    godot::Color custom_bg_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_fill_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_assist_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_baseline_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);

    void _on_resource_changed();
    void _gather_axes(
        godot::PackedStringArray &r_labels,
        godot::PackedFloat32Array &r_raw_scalars,
        godot::PackedFloat32Array &r_effective_scalars
    ) const;

public:
    DifficultyProfileRadar();
    ~DifficultyProfileRadar() override;

    virtual godot::Vector2 _get_minimum_size() const override;

    // --- Resource Bindings ---
    void set_difficulty(const godot::Ref<GameplayDifficulty> &p_difficulty);
    godot::Ref<GameplayDifficulty> get_difficulty() const;

    void set_accessibility_overlay(const godot::Ref<GameplayAccessibilityOption> &p_accessibility);
    godot::Ref<GameplayAccessibilityOption> get_accessibility_overlay() const;

    // --- Display Configuration ---
    void set_max_scalar_radius(float p_max);
    float get_max_scalar_radius() const;

    void set_concentric_rings(int p_rings);
    int get_concentric_rings() const;

    void set_show_baseline_ring(bool p_show);
    bool get_show_baseline_ring() const;

    void set_show_accessibility_overlay(bool p_show);
    bool get_show_accessibility_overlay() const;

    void set_show_axis_labels(bool p_show);
    bool get_show_axis_labels() const;

    // --- Custom Color Overrides ---
    void set_custom_bg_color(const godot::Color &p_color);
    godot::Color get_custom_bg_color() const;

    void set_custom_fill_color(const godot::Color &p_color);
    godot::Color get_custom_fill_color() const;

    void set_custom_assist_color(const godot::Color &p_color);
    godot::Color get_custom_assist_color() const;

    void set_custom_baseline_color(const godot::Color &p_color);
    godot::Color get_custom_baseline_color() const;
};

} // namespace ideam::godot_ext