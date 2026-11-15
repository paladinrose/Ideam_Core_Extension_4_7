#include "godot/geomorphology/ui/stratigraphic_column_inspector_control.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include <algorithm>

namespace ideam::geomorphology {

StratigraphicColumnInspectorControl::StratigraphicColumnInspectorControl() {
    set_custom_minimum_size(godot::Vector2(260.0f, 220.0f));
}

void StratigraphicColumnInspectorControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_terrain_resource", "resource"), &StratigraphicColumnInspectorControl::set_terrain_resource);
    ClassDB::bind_method(D_METHOD("get_terrain_resource"), &StratigraphicColumnInspectorControl::get_terrain_resource);

    ClassDB::bind_method(D_METHOD("set_inspected_cell", "cell"), &StratigraphicColumnInspectorControl::set_inspected_cell);
    ClassDB::bind_method(D_METHOD("get_inspected_cell"), &StratigraphicColumnInspectorControl::get_inspected_cell);

    ClassDB::bind_method(D_METHOD("refresh_column_view"), &StratigraphicColumnInspectorControl::refresh_column_view);
}

void StratigraphicColumnInspectorControl::set_terrain_resource(const godot::Ref<GeomorphologyTerrainResource>& p_resource) {
    terrain_resource_ = p_resource;
    refresh_column_view();
}

godot::Ref<GeomorphologyTerrainResource> StratigraphicColumnInspectorControl::get_terrain_resource() const {
    return terrain_resource_;
}

void StratigraphicColumnInspectorControl::set_inspected_cell(const godot::Vector2i& p_cell) {
    inspected_cell_ = p_cell;
    refresh_column_view();
}

godot::Vector2i StratigraphicColumnInspectorControl::get_inspected_cell() const {
    return inspected_cell_;
}

void StratigraphicColumnInspectorControl::refresh_column_view() {
    if (terrain_resource_.is_valid()) {
        cached_column_data_ = terrain_resource_->inspect_cell_column(inspected_cell_.x, inspected_cell_.y);
    } else {
        cached_column_data_.clear();
    }
    queue_redraw();
}

void StratigraphicColumnInspectorControl::_draw() {
    using namespace godot;

    const Vector2 sz = get_size();
    draw_rect(Rect2(Vector2(0, 0), sz), Color(0.11f, 0.13f, 0.16f, 1.0f), true);
    draw_rect(Rect2(Vector2(0, 0), sz), Color(0.28f, 0.34f, 0.42f, 1.0f), false, 1.0f);

    Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
    if (cached_column_data_.is_empty()) {
        if (font.is_valid()) {
            draw_string(font, Vector2(12.0f, 24.0f), "No Geomorphology Column Selected", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 12, Color(0.75f, 0.78f, 0.82f));
        }
        return;
    }

    const Array strata = cached_column_data_.get("strata", Array());
    const float regolith_m = static_cast<float>(cached_column_data_.get("regolith_thickness_m", 0.0f));
    const float water_m = static_cast<float>(cached_column_data_.get("water_depth_m", 0.0f));
    const float karst_m = static_cast<float>(cached_column_data_.get("karst_cavity_void_m", 0.0f));
    const float total_elev_m = static_cast<float>(cached_column_data_.get("total_column_height_m", 0.0f));
    const int strahler = static_cast<int>(cached_column_data_.get("strahler_order", 1));

    float total_strata_m = regolith_m + water_m;
    for (int i = 0; i < strata.size(); ++i) {
        Dictionary ld = strata[i];
        total_strata_m += static_cast<float>(ld.get("thickness_m", 0.0f));
    }
    total_strata_m = std::max(1.0f, total_strata_m);

    const float col_x = 14.0f;
    const float col_w = 88.0f;
    const float top_margin = 28.0f;
    const float bot_margin = 14.0f;
    const float avail_h = std::max(40.0f, sz.y - top_margin - bot_margin);

    if (font.is_valid()) {
        String header = String("Borehole (") + String::num_int64(inspected_cell_.x) + ", " +
                        String::num_int64(inspected_cell_.y) + ") | Elev: " +
                        String::num(total_elev_m, 1) + "m | Strahler S" + String::num_int64(strahler);
        draw_string(font, Vector2(10.0f, 18.0f), header, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 12, Color(0.90f, 0.94f, 0.98f));
    }

    static const Color kLithColors[8] = {
        Color(0.76f, 0.48f, 0.52f, 1.0f),
        Color(0.24f, 0.26f, 0.30f, 1.0f),
        Color(0.84f, 0.72f, 0.42f, 1.0f),
        Color(0.72f, 0.80f, 0.84f, 1.0f),
        Color(0.46f, 0.52f, 0.58f, 1.0f),
        Color(0.92f, 0.90f, 0.94f, 1.0f),
        Color(0.62f, 0.54f, 0.42f, 1.0f),
        Color(0.32f, 0.48f, 0.24f, 1.0f)
    };

    float cursor_y = sz.y - bot_margin;
    for (int i = 0; i < strata.size(); ++i) {
        Dictionary ld = strata[i];
        const float thick = static_cast<float>(ld.get("thickness_m", 0.0f));
        const int lith_id = static_cast<int>(ld.get("lithology_id", 0));
        const String name = ld.get("archetype_name", "Stratum");
        const float bar_h = std::max(14.0f, (thick / total_strata_m) * avail_h);
        cursor_y -= bar_h;

        Rect2 r(Vector2(col_x, cursor_y), Vector2(col_w, bar_h));
        draw_rect(r, kLithColors[lith_id % 8], true);
        draw_rect(r, Color(0.15f, 0.15f, 0.18f, 1.0f), false, 1.0f);

        if (font.is_valid()) {
            String label = name + " (" + String::num(thick, 1) + "m)";
            draw_string(font, Vector2(col_x + col_w + 10.0f, cursor_y + bar_h * 0.65f), label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(0.86f, 0.88f, 0.92f));
        }
    }

    if (regolith_m > 0.05f) {
        const float reg_h = std::max(8.0f, (regolith_m / total_strata_m) * avail_h);
        cursor_y -= reg_h;
        Rect2 r(Vector2(col_x, cursor_y), Vector2(col_w, reg_h));
        draw_rect(r, Color(0.38f, 0.52f, 0.26f, 1.0f), true);
        if (font.is_valid()) {
            draw_string(font, Vector2(col_x + col_w + 10.0f, cursor_y + reg_h * 0.75f),
                        String("Regolith/Soil (") + String::num(regolith_m, 1) + "m)",
                        HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(0.68f, 0.90f, 0.62f));
        }
    }

    if (water_m > 0.01f) {
        const float wat_h = std::max(6.0f, (water_m / total_strata_m) * avail_h);
        cursor_y -= wat_h;
        Rect2 r(Vector2(col_x, cursor_y), Vector2(col_w, wat_h));
        draw_rect(r, Color(0.18f, 0.62f, 0.96f, 0.85f), true);
    }

    if (karst_m > 0.05f && font.is_valid()) {
        draw_string(font, Vector2(col_x + col_w + 10.0f, sz.y - 6.0f),
                    String("Karst Cavity Void: ") + String::num(karst_m, 2) + "m",
                    HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(0.85f, 0.55f, 0.98f));
    }
}

} // namespace ideam::geomorphology
