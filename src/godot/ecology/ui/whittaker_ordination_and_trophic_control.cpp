#include "godot/ecology/ui/whittaker_ordination_and_trophic_control.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include <algorithm>

namespace ideam::godot_ext {

WhittakerOrdinationAndTrophicControl::WhittakerOrdinationAndTrophicControl() {
    set_custom_minimum_size(godot::Vector2(260.0f, 220.0f));
}

void WhittakerOrdinationAndTrophicControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_biosphere_resource", "resource"), &WhittakerOrdinationAndTrophicControl::set_biosphere_resource);
    ClassDB::bind_method(D_METHOD("get_biosphere_resource"), &WhittakerOrdinationAndTrophicControl::get_biosphere_resource);

    ClassDB::bind_method(D_METHOD("set_inspected_cell", "cell"), &WhittakerOrdinationAndTrophicControl::set_inspected_cell);
    ClassDB::bind_method(D_METHOD("get_inspected_cell"), &WhittakerOrdinationAndTrophicControl::get_inspected_cell);

    ClassDB::bind_method(D_METHOD("refresh_quadrat_view"), &WhittakerOrdinationAndTrophicControl::refresh_quadrat_view);
}

void WhittakerOrdinationAndTrophicControl::set_biosphere_resource(const godot::Ref<EcologyBiosphereResource>& p_resource) {
    biosphere_resource_ = p_resource;
    refresh_quadrat_view();
}

godot::Ref<EcologyBiosphereResource> WhittakerOrdinationAndTrophicControl::get_biosphere_resource() const {
    return biosphere_resource_;
}

void WhittakerOrdinationAndTrophicControl::set_inspected_cell(const godot::Vector2i& p_cell) {
    inspected_cell_ = p_cell;
    refresh_quadrat_view();
}

godot::Vector2i WhittakerOrdinationAndTrophicControl::get_inspected_cell() const {
    return inspected_cell_;
}

void WhittakerOrdinationAndTrophicControl::refresh_quadrat_view() {
    if (biosphere_resource_.is_valid()) {
        cached_quadrat_ = biosphere_resource_->inspect_cell_quadrat(inspected_cell_.x, inspected_cell_.y);
    } else {
        cached_quadrat_.clear();
    }
    queue_redraw();
}

void WhittakerOrdinationAndTrophicControl::_draw() {
    using namespace godot;

    const Vector2 sz = get_size();
    draw_rect(Rect2(Vector2(0, 0), sz), Color(0.09f, 0.14f, 0.11f, 1.0f), true);
    draw_rect(Rect2(Vector2(0, 0), sz), Color(0.26f, 0.42f, 0.32f, 1.0f), false, 1.0f);

    Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
    if (cached_quadrat_.is_empty() || font.is_null()) {
        return;
    }

    const String biome_name = cached_quadrat_.get("life_zone_name", "Temperate Forest");
    const String seral_name = cached_quadrat_.get("seral_stage_name", "S5 Climax");
    const float canopy_kg = static_cast<float>(cached_quadrat_.get("canopy_biomass_kg_m2", 12.0f));
    const float under_kg = static_cast<float>(cached_quadrat_.get("understory_biomass_kg_m2", 3.0f));
    const float soc_kg = static_cast<float>(cached_quadrat_.get("soil_organic_carbon_kg_m2", 8.0f));
    const float min_n = static_cast<float>(cached_quadrat_.get("mineral_nitrogen_g_m2", 40.0f));
    const float herb_d = static_cast<float>(cached_quadrat_.get("grazer_herbivore_density", 0.3f)) +
                         static_cast<float>(cached_quadrat_.get("browser_herbivore_density", 0.2f));
    const float pred_d = static_cast<float>(cached_quadrat_.get("apex_predator_density", 0.1f));
    const float alch = static_cast<float>(cached_quadrat_.get("alchemical_phytochemical_grade", 0.3f));

    String title = String("Quadrat (") + String::num_int64(inspected_cell_.x) + ", " +
                   String::num_int64(inspected_cell_.y) + ") | " + biome_name;
    draw_string(font, Vector2(10.0f, 18.0f), title, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 12, Color(0.90f, 0.98f, 0.92f));
    draw_string(font, Vector2(10.0f, 35.0f), seral_name + " | Alchemical Grade: " + String::num(alch * 100.0f, 0) + "%",
                HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(0.76f, 0.92f, 0.80f));

    const float bar_x = 12.0f;
    const float bar_w = std::max(80.0f, sz.x - 24.0f);

    // Canopy + Understory Biomass bar
    const float bio_frac = std::clamp((canopy_kg + under_kg) / 45.0f, 0.0f, 1.0f);
    draw_rect(Rect2(Vector2(bar_x, 46.0f), Vector2(bar_w, 16.0f)), Color(0.14f, 0.20f, 0.16f, 1.0f), true);
    draw_rect(Rect2(Vector2(bar_x, 46.0f), Vector2(bar_w * bio_frac, 16.0f)), Color(0.22f, 0.72f, 0.36f, 1.0f), true);
    draw_string(font, Vector2(bar_x + 6.0f, 58.0f),
                String("Canopy: ") + String::num(canopy_kg, 1) + " kg/m2 | Understory: " + String::num(under_kg, 1) + " kg/m2",
                HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(1.0f, 1.0f, 1.0f));

    // Soil C & N bar
    const float soc_frac = std::clamp(soc_kg / 25.0f, 0.0f, 1.0f);
    draw_rect(Rect2(Vector2(bar_x, 68.0f), Vector2(bar_w, 16.0f)), Color(0.14f, 0.20f, 0.16f, 1.0f), true);
    draw_rect(Rect2(Vector2(bar_x, 68.0f), Vector2(bar_w * soc_frac, 16.0f)), Color(0.62f, 0.46f, 0.24f, 1.0f), true);
    draw_string(font, Vector2(bar_x + 6.0f, 80.0f),
                String("Soil Carbon: ") + String::num(soc_kg, 1) + " kg/m2 | Mineral N: " + String::num(min_n, 1) + " g/m2",
                HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(1.0f, 1.0f, 1.0f));

    // Trophic Web Pyramid box
    const Rect2 pyr(Vector2(bar_x, 92.0f), Vector2(bar_w, std::max(60.0f, sz.y - 102.0f)));
    draw_rect(pyr, Color(0.06f, 0.10f, 0.08f, 1.0f), true);
    draw_rect(pyr, Color(0.24f, 0.38f, 0.28f, 1.0f), false, 1.0f);

    draw_string(font, Vector2(pyr.position.x + 8.0f, pyr.position.y + 18.0f),
                String("Trophic Web — Herbivores: ") + String::num(herb_d, 2) + " | Apex Predators: " + String::num(pred_d, 2),
                HORIZONTAL_ALIGNMENT_LEFT, -1.0f, 11, Color(0.88f, 0.94f, 0.86f));
}

} // namespace ideam::godot_ext
