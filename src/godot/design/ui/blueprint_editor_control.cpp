#include "blueprint_editor_control.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>

using namespace godot;

namespace ideam::godot_ext {

// ============================================================================
// BlueprintPreviewSurface
// ============================================================================

BlueprintPreviewSurface::BlueprintPreviewSurface() {
    set_custom_minimum_size(Vector2(0.0f, 320.0f));
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_clip_contents(true);
}

void BlueprintPreviewSurface::set_blueprint(const Ref<BlueprintResource> &p_bp) {
    blueprint_ = p_bp;
    queue_redraw();
}

void BlueprintPreviewSurface::_draw() {
    const Vector2 sz = get_size();
    draw_rect(Rect2(Vector2(0.0f, 0.0f), sz), Color(0.09f, 0.12f, 0.18f, 1.0f), true);

    if (blueprint_.is_null()) {
        return;
    }

    const Vector2 origin = sz * 0.5f;
    const auto &core     = blueprint_->get_core_blueprint();

    // Subtle blueprint grid
    const float grid_step = pixels_per_meter_;
    const Color grid_col(0.20f, 0.30f, 0.48f, 0.28f);
    for (float x = std::fmod(origin.x, grid_step); x < sz.x; x += grid_step) {
        draw_line(Vector2(x, 0.0f), Vector2(x, sz.y), grid_col, 1.0f);
    }
    for (float y = std::fmod(origin.y, grid_step); y < sz.y; y += grid_step) {
        draw_line(Vector2(0.0f, y), Vector2(sz.x, y), grid_col, 1.0f);
    }

    // 1. Spaces
    const auto sp_style = core::ArchitecturalBlueprintCore::get_layer_visual_style(core::BlueprintLayerType::LAYER_SPACES);
    const Color sp_stroke(sp_style.color_rgba.r, sp_style.color_rgba.g, sp_style.color_rgba.b, sp_style.color_rgba.a);
    const Color sp_fill(sp_style.color_rgba.r, sp_style.color_rgba.g, sp_style.color_rgba.b, 0.16f);
    for (const auto &sp : core.spaces()) {
        if (sp.vertices_2d.size() < 3) continue;
        PackedVector2Array pts;
        for (const auto &v : sp.vertices_2d) {
            pts.push_back(origin + Vector2(v.x * pixels_per_meter_, v.y * pixels_per_meter_));
        }
        draw_colored_polygon(pts, sp_fill);
        pts.push_back(pts[0]);
        draw_polyline(pts, sp_stroke, sp_style.stroke_weight_px, true);
    }

    // 2. Walls
    for (const auto &w : core.walls()) {
        const auto w_style = core::ArchitecturalBlueprintCore::get_layer_visual_style(
            core::BlueprintLayerType::LAYER_WALLS,
            w.is_exterior != 0u
        );
        const Color w_col(w_style.color_rgba.r, w_style.color_rgba.g, w_style.color_rgba.b, w_style.color_rgba.a);
        draw_line(
            origin + Vector2(w.start_2d.x * pixels_per_meter_, w.start_2d.y * pixels_per_meter_),
            origin + Vector2(w.end_2d.x * pixels_per_meter_, w.end_2d.y * pixels_per_meter_),
            w_col,
            w.stroke_weight_px,
            true
        );
    }

    // 3. Features / Apertures
    for (const auto &f : core.features()) {
        const Vector2 p = origin + Vector2(f.position_2d.x * pixels_per_meter_, f.position_2d.y * pixels_per_meter_);
        const Color f_col = (f.symbol_type == core::BlueprintFeatureSymbolType::SYMBOL_WINDOW_SASH)
            ? Color(0.45f, 0.88f, 1.0f, 0.95f)
            : Color(0.98f, 0.72f, 0.28f, 0.95f);
        draw_circle(p, 5.0f, f_col);
    }
}

// ============================================================================
// BlueprintEditorControl
// ============================================================================

void BlueprintEditorControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_blueprint", "blueprint"), &BlueprintEditorControl::set_blueprint);
    ClassDB::bind_method(D_METHOD("get_blueprint"), &BlueprintEditorControl::get_blueprint);
    ClassDB::bind_method(D_METHOD("_on_add_sample_room_pressed"), &BlueprintEditorControl::_on_add_sample_room_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_door_pressed"), &BlueprintEditorControl::_on_add_door_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_window_pressed"), &BlueprintEditorControl::_on_add_window_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_pressed"), &BlueprintEditorControl::_on_clear_pressed);
    ClassDB::bind_method(D_METHOD("_on_blueprint_changed"), &BlueprintEditorControl::_on_blueprint_changed);
}

BlueprintEditorControl::BlueprintEditorControl() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);

    auto *toolbar = memnew(HBoxContainer);
    add_child(toolbar);

    auto *btn_sample = memnew(Button);
    btn_sample->set_text("+ Sample Room");
    btn_sample->connect("pressed", Callable(this, "_on_add_sample_room_pressed"));
    toolbar->add_child(btn_sample);

    auto *btn_door = memnew(Button);
    btn_door->set_text("+ Door");
    btn_door->connect("pressed", Callable(this, "_on_add_door_pressed"));
    toolbar->add_child(btn_door);

    auto *btn_win = memnew(Button);
    btn_win->set_text("+ Window");
    btn_win->connect("pressed", Callable(this, "_on_add_window_pressed"));
    toolbar->add_child(btn_win);

    auto *btn_clear = memnew(Button);
    btn_clear->set_text("Clear");
    btn_clear->connect("pressed", Callable(this, "_on_clear_pressed"));
    toolbar->add_child(btn_clear);

    telemetry_label_ = memnew(Label);
    telemetry_label_->set_text("Spaces: 0 | Walls: 0 | Features: 0");
    add_child(telemetry_label_);

    preview_ = memnew(BlueprintPreviewSurface);
    add_child(preview_);
}

BlueprintEditorControl::~BlueprintEditorControl() {
    if (blueprint_.is_valid()) {
        const Callable cb(this, "_on_blueprint_changed");
        if (blueprint_->is_connected("changed", cb)) {
            blueprint_->disconnect("changed", cb);
        }
    }
}

void BlueprintEditorControl::set_blueprint(const Ref<BlueprintResource> &p_blueprint) {
    if (blueprint_ == p_blueprint) {
        return;
    }
    const Callable cb(this, "_on_blueprint_changed");
    if (blueprint_.is_valid() && blueprint_->is_connected("changed", cb)) {
        blueprint_->disconnect("changed", cb);
    }
    blueprint_ = p_blueprint;
    if (blueprint_.is_valid() && !blueprint_->is_connected("changed", cb)) {
        blueprint_->connect("changed", cb);
    }
    if (preview_) {
        preview_->set_blueprint(blueprint_);
    }
    _refresh_ui();
}

void BlueprintEditorControl::_refresh_ui() {
    if (!telemetry_label_) return;
    if (blueprint_.is_null()) {
        telemetry_label_->set_text("No BlueprintResource bound.");
        return;
    }
    telemetry_label_->set_text(
        String("Spaces: ") + String::num_int64(blueprint_->get_space_count()) +
        String(" | Walls: ") + String::num_int64(blueprint_->get_wall_count()) +
        String(" | Features: ") + String::num_int64(blueprint_->get_feature_count()) +
        String(" | Cut Height: ") + String::num(blueprint_->get_section_cut_height_meters(), 2) + String("m")
    );
    if (preview_) {
        preview_->queue_redraw();
    }
}

void BlueprintEditorControl::_on_add_sample_room_pressed() {
    if (blueprint_.is_valid()) {
        blueprint_->populate_sample_room();
    }
}

void BlueprintEditorControl::_on_add_door_pressed() {
    if (blueprint_.is_valid()) {
        blueprint_->add_feature_symbol(
            BlueprintResource::SYMBOL_DOOR_SINGLE,
            Vector2(0.0f, 2.0f),
            0.90f,
            2.10f,
            0.0f,
            "D1"
        );
        blueprint_->rebuild_reactive_layers();
    }
}

void BlueprintEditorControl::_on_add_window_pressed() {
    if (blueprint_.is_valid()) {
        blueprint_->add_feature_symbol(
            BlueprintResource::SYMBOL_WINDOW_SASH,
            Vector2(2.5f, 0.0f),
            1.20f,
            1.10f,
            0.90f,
            "W1"
        );
        blueprint_->rebuild_reactive_layers();
    }
}

void BlueprintEditorControl::_on_clear_pressed() {
    if (blueprint_.is_valid()) {
        blueprint_->clear_blueprint();
    }
}

void BlueprintEditorControl::_on_blueprint_changed() {
    _refresh_ui();
}

} // namespace ideam::godot_ext
