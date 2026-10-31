#include "perspective_frustum_visualizer.h"

#include <cmath>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

void PerspectiveFrustumVisualizer::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_perspective", "perspective"), &PerspectiveFrustumVisualizer::set_perspective);
    godot::ClassDB::bind_method(godot::D_METHOD("get_perspective"), &PerspectiveFrustumVisualizer::get_perspective);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "perspective", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayPerspective"), "set_perspective", "get_perspective");

    godot::ClassDB::bind_method(godot::D_METHOD("set_preview_aim_state", "aiming"), &PerspectiveFrustumVisualizer::set_preview_aim_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_preview_aim_state"), &PerspectiveFrustumVisualizer::get_preview_aim_state);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "preview_aim_state"), "set_preview_aim_state", "get_preview_aim_state");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_side_elevation", "show"), &PerspectiveFrustumVisualizer::set_show_side_elevation);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_side_elevation"), &PerspectiveFrustumVisualizer::get_show_side_elevation);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_side_elevation"), "set_show_side_elevation", "get_show_side_elevation");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_labels", "show"), &PerspectiveFrustumVisualizer::set_show_labels);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_labels"), &PerspectiveFrustumVisualizer::get_show_labels);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_labels"), "set_show_labels", "get_show_labels");

    ADD_GROUP("Custom Colors", "custom_");
    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_bg_color", "color"), &PerspectiveFrustumVisualizer::set_custom_bg_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_bg_color"), &PerspectiveFrustumVisualizer::get_custom_bg_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_bg_color"), "set_custom_bg_color", "get_custom_bg_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_frustum_color", "color"), &PerspectiveFrustumVisualizer::set_custom_frustum_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_frustum_color"), &PerspectiveFrustumVisualizer::get_custom_frustum_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_frustum_color"), "set_custom_frustum_color", "get_custom_frustum_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_aim_frustum_color", "color"), &PerspectiveFrustumVisualizer::set_custom_aim_frustum_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_aim_frustum_color"), &PerspectiveFrustumVisualizer::get_custom_aim_frustum_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_aim_frustum_color"), "set_custom_aim_frustum_color", "get_custom_aim_frustum_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_rig_color", "color"), &PerspectiveFrustumVisualizer::set_custom_rig_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_rig_color"), &PerspectiveFrustumVisualizer::get_custom_rig_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_rig_color"), "set_custom_rig_color", "get_custom_rig_color");

    godot::ClassDB::bind_method(godot::D_METHOD("_on_perspective_changed"), &PerspectiveFrustumVisualizer::_on_perspective_changed);
}

PerspectiveFrustumVisualizer::PerspectiveFrustumVisualizer() {
    set_clip_contents(true);
}

PerspectiveFrustumVisualizer::~PerspectiveFrustumVisualizer() {
}

godot::Vector2 PerspectiveFrustumVisualizer::_get_minimum_size() const {
    return godot::Vector2(show_side_elevation ? 360.0f : 190.0f, 175.0f);
}

void PerspectiveFrustumVisualizer::set_perspective(const godot::Ref<GameplayPerspective> &p_perspective) {
    if (perspective == p_perspective) return;

    if (perspective.is_valid() && perspective->is_connected("changed", godot::Callable(this, "_on_perspective_changed"))) {
        perspective->disconnect("changed", godot::Callable(this, "_on_perspective_changed"));
    }
    perspective = p_perspective;
    if (perspective.is_valid() && !perspective->is_connected("changed", godot::Callable(this, "_on_perspective_changed"))) {
        perspective->connect("changed", godot::Callable(this, "_on_perspective_changed"));
    }
    queue_redraw();
}
godot::Ref<GameplayPerspective> PerspectiveFrustumVisualizer::get_perspective() const { return perspective; }

void PerspectiveFrustumVisualizer::set_preview_aim_state(bool p_aiming) {
    preview_aim_state = p_aiming;
    queue_redraw();
}
bool PerspectiveFrustumVisualizer::get_preview_aim_state() const { return preview_aim_state; }

void PerspectiveFrustumVisualizer::set_show_side_elevation(bool p_show) {
    show_side_elevation = p_show;
    update_minimum_size();
    queue_redraw();
}
bool PerspectiveFrustumVisualizer::get_show_side_elevation() const { return show_side_elevation; }

void PerspectiveFrustumVisualizer::set_show_labels(bool p_show) {
    show_labels = p_show;
    queue_redraw();
}
bool PerspectiveFrustumVisualizer::get_show_labels() const { return show_labels; }

void PerspectiveFrustumVisualizer::set_custom_bg_color(const godot::Color &p_color) {
    custom_bg_color = p_color;
    queue_redraw();
}
godot::Color PerspectiveFrustumVisualizer::get_custom_bg_color() const { return custom_bg_color; }

void PerspectiveFrustumVisualizer::set_custom_frustum_color(const godot::Color &p_color) {
    custom_frustum_color = p_color;
    queue_redraw();
}
godot::Color PerspectiveFrustumVisualizer::get_custom_frustum_color() const { return custom_frustum_color; }

void PerspectiveFrustumVisualizer::set_custom_aim_frustum_color(const godot::Color &p_color) {
    custom_aim_frustum_color = p_color;
    queue_redraw();
}
godot::Color PerspectiveFrustumVisualizer::get_custom_aim_frustum_color() const { return custom_aim_frustum_color; }

void PerspectiveFrustumVisualizer::set_custom_rig_color(const godot::Color &p_color) {
    custom_rig_color = p_color;
    queue_redraw();
}
godot::Color PerspectiveFrustumVisualizer::get_custom_rig_color() const { return custom_rig_color; }

void PerspectiveFrustumVisualizer::_on_perspective_changed() {
    queue_redraw();
}

void PerspectiveFrustumVisualizer::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED || p_what == NOTIFICATION_RESIZED) {
        queue_redraw();
        return;
    }
    if (p_what != NOTIFICATION_DRAW) {
        return;
    }

    godot::Color bg_col = (custom_bg_color.a > 0.0f)
        ? custom_bg_color
        : godot::Color(0.11f, 0.12f, 0.15f, 0.90f);
    godot::Color frustum_col = (custom_frustum_color.a > 0.0f)
        ? custom_frustum_color
        : godot::Color(0.20f, 0.78f, 0.96f, 0.28f);
    godot::Color aim_col = (custom_aim_frustum_color.a > 0.0f)
        ? custom_aim_frustum_color
        : godot::Color(0.98f, 0.74f, 0.18f, 0.35f);
    godot::Color rig_col = (custom_rig_color.a > 0.0f)
        ? custom_rig_color
        : godot::Color(0.92f, 0.42f, 0.32f, 1.0f);
    godot::Color grid_col(1.0f, 1.0f, 1.0f, 0.14f);
    godot::Color text_col = has_theme_color("font_color", "Label")
        ? get_theme_color("font_color", "Label")
        : godot::Color(0.92f, 0.93f, 0.95f, 1.0f);

    godot::Vector2 sz = get_size();
    float pad = 8.0f;

    if (show_side_elevation) {
        float panel_w = (sz.x - pad * 3.0f) * 0.5f;
        float panel_h = sz.y - pad * 2.0f;
        _draw_top_down_view(godot::Rect2(pad, pad, panel_w, panel_h), bg_col, frustum_col, aim_col, rig_col, grid_col, text_col);
        _draw_side_elevation_view(godot::Rect2(pad * 2.0f + panel_w, pad, panel_w, panel_h), bg_col, frustum_col, rig_col, grid_col, text_col);
    } else {
        _draw_top_down_view(godot::Rect2(pad, pad, sz.x - pad * 2.0f, sz.y - pad * 2.0f), bg_col, frustum_col, aim_col, rig_col, grid_col, text_col);
    }
}

void PerspectiveFrustumVisualizer::_draw_top_down_view(
    const godot::Rect2 &p_rect,
    const godot::Color &p_bg_col,
    const godot::Color &p_frustum_col,
    const godot::Color &p_aim_col,
    const godot::Color &p_rig_col,
    const godot::Color &p_grid_col,
    const godot::Color &p_text_col
) {
    draw_rect(p_rect, p_bg_col, true);
    draw_rect(p_rect, p_grid_col, false, 1.25f);

    float fov_deg = perspective.is_valid() ? perspective->get_fov() : 75.0f;
    float aim_fov_deg = perspective.is_valid() ? perspective->get_aim_fov() : 50.0f;
    float arm_len = perspective.is_valid() ? perspective->get_arm_length() : 2.5f;
    float aim_arm_len = perspective.is_valid() ? perspective->get_aim_arm_length() : 1.35f;
    godot::Vector3 cam_off = perspective.is_valid() ? perspective->get_effective_camera_offset(preview_aim_state) : godot::Vector3(0.45f, 0.15f, 0.0f);
    PerspectiveProjectionMode proj = perspective.is_valid() ? perspective->get_projection_mode() : PerspectiveProjectionMode::PROJECTION_PERSPECTIVE;

    // Place player target near lower-middle of the Top-Down box (looking upward -Y on screen)
    godot::Vector2 player_pt(p_rect.position.x + p_rect.size.x * 0.5f, p_rect.position.y + p_rect.size.y * 0.62f);
    float meters_to_px = godot::Math::min(p_rect.size.x, p_rect.size.y) / 9.5f;

    float active_arm = preview_aim_state ? aim_arm_len : arm_len;
    godot::Vector2 cam_pt = player_pt + godot::Vector2(cam_off.x * meters_to_px * 1.8f, godot::Math::min(active_arm, 4.5f) * meters_to_px);

    // Draw Normal Frustum Cone (looking upward along -Y)
    float cone_len = p_rect.size.y * 0.52f;
    auto draw_cone = [&](float p_fov, const godot::Color &p_fill, float p_stroke_w) {
        godot::Color edge(p_fill.r, p_fill.g, p_fill.b, 0.88f);
        if (proj == PerspectiveProjectionMode::PROJECTION_ORTHOGONAL) {
            float half_w = p_rect.size.x * 0.30f;
            godot::Rect2 ortho_box(cam_pt.x - half_w, cam_pt.y - cone_len, half_w * 2.0f, cone_len);
            draw_rect(ortho_box, p_fill, true);
            draw_rect(ortho_box, edge, false, p_stroke_w);
        } else {
            float half_rad = godot::Math::deg_to_rad(godot::Math::clamp(p_fov, 15.0f, 150.0f) * 0.5f);
            float spread_x = std::tan(half_rad) * cone_len * 0.65f;
            spread_x = godot::Math::min(spread_x, p_rect.size.x * 0.46f);
            godot::PackedVector2Array tri;
            tri.resize(3);
            tri.set(0, cam_pt);
            tri.set(1, godot::Vector2(cam_pt.x - spread_x, cam_pt.y - cone_len));
            tri.set(2, godot::Vector2(cam_pt.x + spread_x, cam_pt.y - cone_len));
            draw_colored_polygon(tri, p_fill);
            draw_line(tri[0], tri[1], edge, p_stroke_w);
            draw_line(tri[0], tri[2], edge, p_stroke_w);
            draw_line(tri[1], tri[2], edge, 1.0f);
        }
    };

    draw_cone(fov_deg, p_frustum_col, 1.5f);
    draw_cone(aim_fov_deg, p_aim_col, 1.5f);

    // Draw SpringArm & Camera Rig
    draw_line(player_pt, cam_pt, p_rig_col, 2.0f);
    draw_circle(player_pt, 6.0f, godot::Color(0.35f, 0.90f, 0.52f, 1.0f)); // Player Pivot
    draw_circle(cam_pt, 5.0f, p_rig_col);                                   // Camera Node

    if (show_labels) {
        godot::Ref<godot::Font> font = get_theme_default_font();
        int font_size = get_theme_default_font_size() > 0 ? get_theme_default_font_size() - 2 : 12;
        if (font.is_valid()) {
            draw_string(
                font,
                godot::Vector2(p_rect.position.x + 6.0f, p_rect.position.y + 15.0f),
                "Top-Down (FOV " + godot::String::num(fov_deg, 0) + "° / Aim " + godot::String::num(aim_fov_deg, 0) + "°)",
                godot::HORIZONTAL_ALIGNMENT_LEFT,
                p_rect.size.x - 12.0f,
                font_size,
                p_text_col
            );
        }
    }
}

void PerspectiveFrustumVisualizer::_draw_side_elevation_view(
    const godot::Rect2 &p_rect,
    const godot::Color &p_bg_col,
    const godot::Color &p_frustum_col,
    const godot::Color &p_rig_col,
    const godot::Color &p_grid_col,
    const godot::Color &p_text_col
) {
    draw_rect(p_rect, p_bg_col, true);
    draw_rect(p_rect, p_grid_col, false, 1.25f);

    float pitch_deg = perspective.is_valid() ? perspective->get_default_pitch_degrees() : -10.0f;
    float min_pitch = perspective.is_valid() ? perspective->get_min_pitch_degrees() : -80.0f;
    float max_pitch = perspective.is_valid() ? perspective->get_max_pitch_degrees() : 75.0f;
    float arm_len = perspective.is_valid() ? perspective->get_effective_arm_length(preview_aim_state) : 2.5f;

    // Ground plane line
    float ground_y = p_rect.position.y + p_rect.size.y - 22.0f;
    draw_line(godot::Vector2(p_rect.position.x + 10.0f, ground_y), godot::Vector2(p_rect.position.x + p_rect.size.x - 10.0f, ground_y), p_grid_col, 1.5f);

    // Player capsule & head pivot in side view
    godot::Vector2 feet_pt(p_rect.position.x + p_rect.size.x * 0.52f, ground_y);
    godot::Vector2 pivot_pt(feet_pt.x, ground_y - p_rect.size.y * 0.36f);
    draw_line(feet_pt, pivot_pt, godot::Color(0.35f, 0.90f, 0.52f, 0.9f), 4.0f);
    draw_circle(pivot_pt, 5.0f, godot::Color(0.35f, 0.90f, 0.52f, 1.0f));

    // Pitch constraint arc around pivot
    float arc_r = godot::Math::min(p_rect.size.x, p_rect.size.y) * 0.34f;
    float min_rad = Math_PI + godot::Math::deg_to_rad(min_pitch);
    float max_rad = Math_PI + godot::Math::deg_to_rad(max_pitch);
    draw_arc(pivot_pt, arc_r, min_rad, max_rad, 32, godot::Color(p_frustum_col.r, p_frustum_col.g, p_frustum_col.b, 0.65f), 2.0f);

    // Active camera boom at default_pitch_degrees
    float boom_r = godot::Math::clamp(arm_len / 6.0f, 0.15f, 1.0f) * arc_r;
    float pitch_rad = Math_PI + godot::Math::deg_to_rad(pitch_deg);
    godot::Vector2 cam_pt = pivot_pt + godot::Vector2(std::cos(pitch_rad), std::sin(pitch_rad)) * boom_r;

    draw_line(pivot_pt, cam_pt, p_rig_col, 2.0f);
    draw_circle(cam_pt, 5.0f, p_rig_col);

    if (show_labels) {
        godot::Ref<godot::Font> font = get_theme_default_font();
        int font_size = get_theme_default_font_size() > 0 ? get_theme_default_font_size() - 2 : 12;
        if (font.is_valid()) {
            draw_string(
                font,
                godot::Vector2(p_rect.position.x + 6.0f, p_rect.position.y + 15.0f),
                "Elevation (Pitch " + godot::String::num(pitch_deg, 0) + "°, Arm " + godot::String::num(arm_len, 1) + "m)",
                godot::HORIZONTAL_ALIGNMENT_LEFT,
                p_rect.size.x - 12.0f,
                font_size,
                p_text_col
            );
        }
    }
}

} // namespace ideam::godot_ext