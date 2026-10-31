#include "difficulty_profile_radar.h"

#include <cmath>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

namespace ideam::godot_ext {

void DifficultyProfileRadar::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_difficulty", "difficulty"), &DifficultyProfileRadar::set_difficulty);
    godot::ClassDB::bind_method(godot::D_METHOD("get_difficulty"), &DifficultyProfileRadar::get_difficulty);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "difficulty", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayDifficulty"), "set_difficulty", "get_difficulty");

    godot::ClassDB::bind_method(godot::D_METHOD("set_accessibility_overlay", "accessibility"), &DifficultyProfileRadar::set_accessibility_overlay);
    godot::ClassDB::bind_method(godot::D_METHOD("get_accessibility_overlay"), &DifficultyProfileRadar::get_accessibility_overlay);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "accessibility_overlay", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayAccessibilityOption"), "set_accessibility_overlay", "get_accessibility_overlay");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_scalar_radius", "max_scalar"), &DifficultyProfileRadar::set_max_scalar_radius);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_scalar_radius"), &DifficultyProfileRadar::get_max_scalar_radius);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "max_scalar_radius", godot::PROPERTY_HINT_RANGE, "1.2,4.0,0.1"), "set_max_scalar_radius", "get_max_scalar_radius");

    godot::ClassDB::bind_method(godot::D_METHOD("set_concentric_rings", "rings"), &DifficultyProfileRadar::set_concentric_rings);
    godot::ClassDB::bind_method(godot::D_METHOD("get_concentric_rings"), &DifficultyProfileRadar::get_concentric_rings);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "concentric_rings", godot::PROPERTY_HINT_RANGE, "2,8,1"), "set_concentric_rings", "get_concentric_rings");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_baseline_ring", "show"), &DifficultyProfileRadar::set_show_baseline_ring);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_baseline_ring"), &DifficultyProfileRadar::get_show_baseline_ring);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_baseline_ring"), "set_show_baseline_ring", "get_show_baseline_ring");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_accessibility_overlay", "show"), &DifficultyProfileRadar::set_show_accessibility_overlay);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_accessibility_overlay"), &DifficultyProfileRadar::get_show_accessibility_overlay);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_accessibility_overlay"), "set_show_accessibility_overlay", "get_show_accessibility_overlay");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_axis_labels", "show"), &DifficultyProfileRadar::set_show_axis_labels);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_axis_labels"), &DifficultyProfileRadar::get_show_axis_labels);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_axis_labels"), "set_show_axis_labels", "get_show_axis_labels");

    ADD_GROUP("Custom Colors", "custom_");
    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_bg_color", "color"), &DifficultyProfileRadar::set_custom_bg_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_bg_color"), &DifficultyProfileRadar::get_custom_bg_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_bg_color"), "set_custom_bg_color", "get_custom_bg_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_fill_color", "color"), &DifficultyProfileRadar::set_custom_fill_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_fill_color"), &DifficultyProfileRadar::get_custom_fill_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_fill_color"), "set_custom_fill_color", "get_custom_fill_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_assist_color", "color"), &DifficultyProfileRadar::set_custom_assist_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_assist_color"), &DifficultyProfileRadar::get_custom_assist_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_assist_color"), "set_custom_assist_color", "get_custom_assist_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_baseline_color", "color"), &DifficultyProfileRadar::set_custom_baseline_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_baseline_color"), &DifficultyProfileRadar::get_custom_baseline_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_baseline_color"), "set_custom_baseline_color", "get_custom_baseline_color");

    godot::ClassDB::bind_method(godot::D_METHOD("_on_resource_changed"), &DifficultyProfileRadar::_on_resource_changed);
}

DifficultyProfileRadar::DifficultyProfileRadar() {
    set_clip_contents(true);
}

DifficultyProfileRadar::~DifficultyProfileRadar() {
}

godot::Vector2 DifficultyProfileRadar::_get_minimum_size() const {
    return godot::Vector2(220.0f, 220.0f);
}

void DifficultyProfileRadar::set_difficulty(const godot::Ref<GameplayDifficulty> &p_difficulty) {
    if (difficulty == p_difficulty) return;

    if (difficulty.is_valid() && difficulty->is_connected("changed", godot::Callable(this, "_on_resource_changed"))) {
        difficulty->disconnect("changed", godot::Callable(this, "_on_resource_changed"));
    }
    difficulty = p_difficulty;
    if (difficulty.is_valid() && !difficulty->is_connected("changed", godot::Callable(this, "_on_resource_changed"))) {
        difficulty->connect("changed", godot::Callable(this, "_on_resource_changed"));
    }
    queue_redraw();
}
godot::Ref<GameplayDifficulty> DifficultyProfileRadar::get_difficulty() const { return difficulty; }

void DifficultyProfileRadar::set_accessibility_overlay(const godot::Ref<GameplayAccessibilityOption> &p_accessibility) {
    if (accessibility_overlay == p_accessibility) return;

    if (accessibility_overlay.is_valid() && accessibility_overlay->is_connected("changed", godot::Callable(this, "_on_resource_changed"))) {
        accessibility_overlay->disconnect("changed", godot::Callable(this, "_on_resource_changed"));
    }
    accessibility_overlay = p_accessibility;
    if (accessibility_overlay.is_valid() && !accessibility_overlay->is_connected("changed", godot::Callable(this, "_on_resource_changed"))) {
        accessibility_overlay->connect("changed", godot::Callable(this, "_on_resource_changed"));
    }
    queue_redraw();
}
godot::Ref<GameplayAccessibilityOption> DifficultyProfileRadar::get_accessibility_overlay() const { return accessibility_overlay; }

void DifficultyProfileRadar::set_max_scalar_radius(float p_max) {
    max_scalar_radius = godot::Math::clamp(p_max, 1.2f, 4.0f);
    queue_redraw();
}
float DifficultyProfileRadar::get_max_scalar_radius() const { return max_scalar_radius; }

void DifficultyProfileRadar::set_concentric_rings(int p_rings) {
    concentric_rings = godot::Math::clamp(p_rings, 2, 8);
    queue_redraw();
}
int DifficultyProfileRadar::get_concentric_rings() const { return concentric_rings; }

void DifficultyProfileRadar::set_show_baseline_ring(bool p_show) {
    show_baseline_ring = p_show;
    queue_redraw();
}
bool DifficultyProfileRadar::get_show_baseline_ring() const { return show_baseline_ring; }

void DifficultyProfileRadar::set_show_accessibility_overlay(bool p_show) {
    show_accessibility_overlay = p_show;
    queue_redraw();
}
bool DifficultyProfileRadar::get_show_accessibility_overlay() const { return show_accessibility_overlay; }

void DifficultyProfileRadar::set_show_axis_labels(bool p_show) {
    show_axis_labels = p_show;
    queue_redraw();
}
bool DifficultyProfileRadar::get_show_axis_labels() const { return show_axis_labels; }

void DifficultyProfileRadar::set_custom_bg_color(const godot::Color &p_color) {
    custom_bg_color = p_color;
    queue_redraw();
}
godot::Color DifficultyProfileRadar::get_custom_bg_color() const { return custom_bg_color; }

void DifficultyProfileRadar::set_custom_fill_color(const godot::Color &p_color) {
    custom_fill_color = p_color;
    queue_redraw();
}
godot::Color DifficultyProfileRadar::get_custom_fill_color() const { return custom_fill_color; }

void DifficultyProfileRadar::set_custom_assist_color(const godot::Color &p_color) {
    custom_assist_color = p_color;
    queue_redraw();
}
godot::Color DifficultyProfileRadar::get_custom_assist_color() const { return custom_assist_color; }

void DifficultyProfileRadar::set_custom_baseline_color(const godot::Color &p_color) {
    custom_baseline_color = p_color;
    queue_redraw();
}
godot::Color DifficultyProfileRadar::get_custom_baseline_color() const { return custom_baseline_color; }

void DifficultyProfileRadar::_on_resource_changed() {
    queue_redraw();
}

void DifficultyProfileRadar::_gather_axes(
    godot::PackedStringArray &r_labels,
    godot::PackedFloat32Array &r_raw_scalars,
    godot::PackedFloat32Array &r_effective_scalars
) const {
    r_labels.clear();
    r_raw_scalars.clear();
    r_effective_scalars.clear();

    float combat = 1.0f;
    float puzzle = 1.0f;
    float stealth = 1.0f;
    float survival = 1.0f;
    float aggression = 1.0f;

    if (difficulty.is_valid()) {
        combat = (difficulty->get_enemy_health_multiplier() + difficulty->get_enemy_damage_multiplier()) * 0.5f;
        puzzle = GameplayDifficulty::level_to_scalar(difficulty->get_puzzle_level());
        stealth = (difficulty->get_ai_vision_range_multiplier() + difficulty->get_ai_hearing_range_multiplier() + difficulty->get_ai_detection_speed_multiplier()) / 3.0f;
        float inv_loot = difficulty->get_loot_abundance_multiplier() > 0.05f ? (1.0f / difficulty->get_loot_abundance_multiplier()) : 2.0f;
        survival = (inv_loot + difficulty->get_stamina_drain_multiplier() + difficulty->get_hazard_damage_multiplier()) / 3.0f;
        aggression = (difficulty->get_enemy_speed_multiplier() + difficulty->get_enemy_aggression_multiplier()) * 0.5f;
    }

    r_labels.push_back("Combat");
    r_raw_scalars.push_back(combat);

    r_labels.push_back("Puzzle / Riddle");
    r_raw_scalars.push_back(puzzle);

    r_labels.push_back("Stealth / AI");
    r_raw_scalars.push_back(stealth);

    r_labels.push_back("Survival");
    r_raw_scalars.push_back(survival);

    r_labels.push_back("Pace / Speed");
    r_raw_scalars.push_back(aggression);

    // Append any custom aspect levels stored in GameplayDifficulty
    if (difficulty.is_valid()) {
        godot::Dictionary custom = difficulty->get_custom_aspect_levels();
        godot::Array keys = custom.keys();
        for (int i = 0; i < keys.size(); ++i) {
            godot::String k = keys[i];
            float val = static_cast<float>(custom[keys[i]]);
            r_labels.push_back(k.capitalize());
            r_raw_scalars.push_back(val);
        }
    }

    // Compute effective post-Accessibility cascade scalars
    float dmg_assist = 1.0f;
    float speed_assist = 1.0f;
    float timeout_assist = 1.0f;
    if (accessibility_overlay.is_valid() && accessibility_overlay->get_enabled()) {
        dmg_assist = accessibility_overlay->get_player_damage_taken_multiplier();
        speed_assist = accessibility_overlay->get_enemy_time_scale() * accessibility_overlay->get_game_speed_scale();
        timeout_assist = accessibility_overlay->get_interaction_timeout_multiplier() > 0.1f
            ? (1.0f / accessibility_overlay->get_interaction_timeout_multiplier())
            : 1.0f;
    }

    for (int i = 0; i < r_raw_scalars.size(); ++i) {
        float eff = r_raw_scalars[i];
        if (i == 0) eff *= dmg_assist;
        else if (i == 1) eff *= timeout_assist;
        else if (i == 4) eff *= speed_assist;
        r_effective_scalars.push_back(eff);
    }
}

void DifficultyProfileRadar::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED || p_what == NOTIFICATION_RESIZED) {
        queue_redraw();
        return;
    }
    if (p_what != NOTIFICATION_DRAW) {
        return;
    }

    godot::Color bg_col = (custom_bg_color.a > 0.0f)
        ? custom_bg_color
        : godot::Color(0.11f, 0.12f, 0.15f, 0.88f);
    godot::Color fill_col = (custom_fill_color.a > 0.0f)
        ? custom_fill_color
        : godot::Color(0.95f, 0.42f, 0.22f, 0.42f);
    godot::Color stroke_col(fill_col.r, fill_col.g, fill_col.b, 0.95f);
    godot::Color assist_col = (custom_assist_color.a > 0.0f)
        ? custom_assist_color
        : godot::Color(0.18f, 0.84f, 0.96f, 0.85f);
    godot::Color base_col = (custom_baseline_color.a > 0.0f)
        ? custom_baseline_color
        : godot::Color(0.55f, 0.88f, 0.55f, 0.55f);
    godot::Color grid_col(1.0f, 1.0f, 1.0f, 0.15f);
    godot::Color text_col = has_theme_color("font_color", "Label")
        ? get_theme_color("font_color", "Label")
        : godot::Color(0.92f, 0.93f, 0.95f, 1.0f);

    godot::PackedStringArray labels;
    godot::PackedFloat32Array raw_scalars;
    godot::PackedFloat32Array eff_scalars;
    _gather_axes(labels, raw_scalars, eff_scalars);

    int n = labels.size();
    if (n < 3) return;

    godot::Vector2 sz = get_size();
    godot::Vector2 center = sz * 0.5f;
    float margin = show_axis_labels ? 36.0f : 14.0f;
    float max_r = godot::Math::max(godot::Math::min(sz.x, sz.y) * 0.5f - margin, 16.0f);

    // 1. Draw Concentric Polygon Rings & Spokes
    auto build_ring = [&](float p_radius) {
        godot::PackedVector2Array ring;
        ring.resize(n + 1);
        for (int i = 0; i <= n; ++i) {
            int idx = i % n;
            float angle = -Math_PI * 0.5f + (Math_TAU * static_cast<float>(idx)) / static_cast<float>(n);
            ring.set(i, center + godot::Vector2(std::cos(angle), std::sin(angle)) * p_radius);
        }
        return ring;
    };

    godot::PackedVector2Array outer_poly = build_ring(max_r);
    godot::PackedVector2Array outer_closed;
    outer_closed.resize(n);
    for (int i = 0; i < n; ++i) outer_closed.set(i, outer_poly[i]);
    draw_colored_polygon(outer_closed, bg_col);

    for (int r = 1; r <= concentric_rings; ++r) {
        float ring_r = max_r * (static_cast<float>(r) / static_cast<float>(concentric_rings));
        draw_polyline(build_ring(ring_r), grid_col, 1.0f);
    }

    // 1.0x Normal Baseline Ring
    if (show_baseline_ring && max_scalar_radius > 0.1f) {
        float norm_r = max_r * godot::Math::clamp(1.0f / max_scalar_radius, 0.0f, 1.0f);
        draw_polyline(build_ring(norm_r), base_col, 1.5f);
    }

    // Axis spokes + Labels
    godot::Ref<godot::Font> font = get_theme_default_font();
    int font_size = get_theme_default_font_size() > 0 ? get_theme_default_font_size() - 2 : 12;

    for (int i = 0; i < n; ++i) {
        float angle = -Math_PI * 0.5f + (Math_TAU * static_cast<float>(i)) / static_cast<float>(n);
        godot::Vector2 dir(std::cos(angle), std::sin(angle));
        draw_line(center, center + dir * max_r, grid_col, 1.0f);

        if (show_axis_labels && font.is_valid()) {
            godot::Vector2 lbl_pos = center + dir * (max_r + 14.0f);
            draw_string(
                font,
                godot::Vector2(lbl_pos.x - 45.0f, lbl_pos.y + 4.0f),
                labels[i],
                godot::HORIZONTAL_ALIGNMENT_CENTER,
                90.0f,
                font_size,
                text_col
            );
        }
    }

    // 2. Draw Raw Difficulty Polygon
    godot::PackedVector2Array raw_poly;
    godot::PackedVector2Array raw_outline;
    raw_poly.resize(n);
    raw_outline.resize(n + 1);
    for (int i = 0; i < n; ++i) {
        float angle = -Math_PI * 0.5f + (Math_TAU * static_cast<float>(i)) / static_cast<float>(n);
        float norm = godot::Math::clamp(raw_scalars[i] / max_scalar_radius, 0.05f, 1.0f);
        godot::Vector2 pt = center + godot::Vector2(std::cos(angle), std::sin(angle)) * (max_r * norm);
        raw_poly.set(i, pt);
        raw_outline.set(i, pt);
    }
    raw_outline.set(n, raw_poly[0]);

    draw_colored_polygon(raw_poly, fill_col);
    draw_polyline(raw_outline, stroke_col, 2.25f);
    for (int i = 0; i < n; ++i) {
        draw_circle(raw_poly[i], 4.0f, stroke_col);
    }

    // 3. Draw Effective Post-Accessibility (!important) Overlay Polygon
    if (show_accessibility_overlay && accessibility_overlay.is_valid()) {
        godot::PackedVector2Array eff_outline;
        eff_outline.resize(n + 1);
        for (int i = 0; i < n; ++i) {
            float angle = -Math_PI * 0.5f + (Math_TAU * static_cast<float>(i)) / static_cast<float>(n);
            float norm = godot::Math::clamp(eff_scalars[i] / max_scalar_radius, 0.05f, 1.0f);
            godot::Vector2 pt = center + godot::Vector2(std::cos(angle), std::sin(angle)) * (max_r * norm);
            eff_outline.set(i, pt);
            draw_circle(pt, 3.5f, assist_col);
        }
        eff_outline.set(n, eff_outline[0]);
        draw_polyline(eff_outline, assist_col, 1.75f);
    }
}

} // namespace ideam::godot_ext