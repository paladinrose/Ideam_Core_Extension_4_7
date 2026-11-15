#include "godot/climatology/ui/thermodynamic_sounding_skewt_control.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include <algorithm>

namespace ideam::climatology {

ThermodynamicSoundingSkewTControl::ThermodynamicSoundingSkewTControl() {
    set_custom_minimum_size(godot::Vector2(260.0f, 210.0f));
}

void ThermodynamicSoundingSkewTControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_atmosphere_resource", "resource"), &ThermodynamicSoundingSkewTControl::set_atmosphere_resource);
    ClassDB::bind_method(D_METHOD("get_atmosphere_resource"), &ThermodynamicSoundingSkewTControl::get_atmosphere_resource);

    ClassDB::bind_method(D_METHOD("set_inspected_cell", "cell"), &ThermodynamicSoundingSkewTControl::set_inspected_cell);
    ClassDB::bind_method(D_METHOD("get_inspected_cell"), &ThermodynamicSoundingSkewTControl::get_inspected_cell);

    ClassDB::bind_method(D_METHOD("refresh_sounding_view"), &ThermodynamicSoundingSkewTControl::refresh_sounding_view);
}

void ThermodynamicSoundingSkewTControl::set_atmosphere_resource(const godot::Ref<ClimatologyAtmosphereResource>& p_resource) {
    atmosphere_resource_ = p_resource;
    refresh_sounding_view();
}

godot::Ref<ClimatologyAtmosphereResource> ThermodynamicSoundingSkewTControl::get_atmosphere_resource() const {
    return atmosphere_resource_;
}

void ThermodynamicSoundingSkewTControl::set_inspected_cell(const godot::Vector2i& p_cell) {
    inspected_cell_ = p_cell;
    refresh_sounding_view();
}

godot::Vector2i ThermodynamicSoundingSkewTControl::get_inspected_cell() const {
    return inspected_cell_;
}

void ThermodynamicSoundingSkewTControl::refresh_sounding_view() {
    if (atmosphere_resource_.is_valid()) {
        cached_sounding_ = atmosphere_resource_->inspect_cell_sounding(inspected_cell_.x, inspected_cell_.y);
    } else {
        cached_sounding_.clear();
    }
    queue_redraw();
}

void ThermodynamicSoundingSkewTControl::_draw() {
    using namespace godot;

    const Vector2 sz = get_size();
    draw_rect(Rect2(Vector2(0, 0), sz), Color(0.09f, 0.12f, 0.17f, 1.0f), true);
    draw_rect(Rect2(Vector2(0, 0), sz), Color(0.28f, 0.36f, 0.48f, 1.0f), false, 1.0f);

    Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
    if (cached_sounding_.is_empty() || font.is_null()) {
        return;
    }

    const float temp_c = static_cast<float>(cached_sounding_.get("surface_air_temp_c", 18.0f));
    const float pres_hpa = static_cast<float>(cached_sounding_.get("boundary_pressure_hpa", 1013.25f));
    const float rh_pct = static_cast<float>(cached_sounding_.get("relative_humidity_pct", 65.0f));
    const float lcl_m = static_cast<float>(cached_sounding_.get("lifting_condensation_lvl_m", 1100.0f));
    const float cape = static_cast<float>(cached_sounding_.get("convective_cape_j_kg", 450.0f));
    const float srh = static_cast<float>(cached_sounding_.get("storm_rel_helicity_m2_s2", 75.0f));
    const float gust_mps = static_cast<float>(cached_sounding_.get("tornado_wind_gust_mps", 6.0f));
    const int ef_tier = static_cast<int>(cached_sounding_.get("local_ef_tier", 0));
    const String koppen = cached_sounding_.get("koppen_name", "Cfa");

    String title = String("Radiosonde (") + String::num_int64(inspected_cell_.x) + ", " +
                   String::num_int64(inspected_cell_.y) + ") | " + koppen;
    draw_string(font, Vector2(10.0f, 18.0f), title, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 12, Color(0.92f, 0.96f, 1.0f));

    String thermo = String("T: ") + String::num(temp_c, 1) + " C | P: " + String::num(pres_hpa, 1) +
                    " hPa | RH: " + String::num(rh_pct, 0) + "% | LCL: " + String::num(lcl_m, 0) + "m";
    draw_string(font, Vector2(10.0f, 38.0f), thermo, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(0.76f, 0.88f, 0.98f));

    // Draw CAPE & SRH instability bars
    const float bar_x = 12.0f;
    const float bar_w = std::max(80.0f, sz.x - 24.0f);
    const float cape_frac = std::clamp(cape / 4000.0f, 0.0f, 1.0f);
    draw_rect(Rect2(Vector2(bar_x, 50.0f), Vector2(bar_w, 16.0f)), Color(0.16f, 0.20f, 0.26f, 1.0f), true);
    draw_rect(Rect2(Vector2(bar_x, 50.0f), Vector2(bar_w * cape_frac, 16.0f)), Color(0.96f, 0.45f, 0.18f, 1.0f), true);
    draw_string(font, Vector2(bar_x + 6.0f, 62.0f), String("CAPE: ") + String::num(cape, 0) + " J/kg", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(1.0f, 1.0f, 1.0f));

    const float srh_frac = std::clamp(srh / 500.0f, 0.0f, 1.0f);
    draw_rect(Rect2(Vector2(bar_x, 74.0f), Vector2(bar_w, 16.0f)), Color(0.16f, 0.20f, 0.26f, 1.0f), true);
    draw_rect(Rect2(Vector2(bar_x, 74.0f), Vector2(bar_w * srh_frac, 16.0f)), Color(0.68f, 0.32f, 0.95f, 1.0f), true);
    draw_string(font, Vector2(bar_x + 6.0f, 86.0f), String("0-3km SRH: ") + String::num(srh, 0) + " m2/s2", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(1.0f, 1.0f, 1.0f));

    // Draw Skew-T style dry vs moist lapse profile diagram in lower half
    const Rect2 diag(Vector2(12.0f, 100.0f), Vector2(bar_w, std::max(55.0f, sz.y - 112.0f)));
    draw_rect(diag, Color(0.06f, 0.08f, 0.12f, 1.0f), true);
    draw_rect(diag, Color(0.24f, 0.30f, 0.38f, 1.0f), false, 1.0f);

    const float lcl_norm = std::clamp(lcl_m / 3500.0f, 0.05f, 0.95f);
    const float lcl_y = diag.position.y + diag.size.y * (1.0f - lcl_norm);
    draw_line(Vector2(diag.position.x, lcl_y), Vector2(diag.position.x + diag.size.x, lcl_y), Color(0.45f, 0.80f, 1.0f, 0.65f), 1.5f);

    String vortex_line = String("Peak Gust: ") + String::num(gust_mps, 1) + " m/s";
    if (ef_tier > 0) {
        vortex_line += String(" | ACTIVE TORNADO VORTEX: EF") + String::num_int64(ef_tier - 1);
    }
    draw_string(font, Vector2(diag.position.x + 8.0f, diag.position.y + diag.size.y - 8.0f),
                vortex_line, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11,
                ef_tier > 0 ? Color(1.0f, 0.38f, 0.42f) : Color(0.75f, 0.85f, 0.92f));
}

} // namespace ideam::climatology
