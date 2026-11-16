#include "godot/ecology/editor/ecology_inspector.h"

#include "godot/ecology/ui/ecology_biome_map_preview_canvas.h"
#include "godot/ecology/ui/whittaker_ordination_and_trophic_control.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void EcologyInspector::_bind_methods() {}

bool EcologyInspector::_can_handle(godot::Object* p_object) const {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<EcologyBiosphereResource>(p_object) != nullptr ||
           godot::Object::cast_to<EcologyBiosphereNode3D>(p_object) != nullptr;
}

void EcologyInspector::_parse_begin(godot::Object* p_object) {
    using namespace godot;

    Ref<EcologyBiosphereResource> bio_res;
    if (auto* direct_res = Object::cast_to<EcologyBiosphereResource>(p_object)) {
        bio_res = Ref<EcologyBiosphereResource>(direct_res);
    } else if (auto* bio_node = Object::cast_to<EcologyBiosphereNode3D>(p_object)) {
        bio_res = bio_node->get_biosphere_resource();
    }

    if (bio_res.is_null()) {
        return;
    }

    auto* root_vbox = memnew(VBoxContainer);
    root_vbox->set_name("EcologyInspectorRoot");

    auto* title_label = memnew(Label);
    title_label->set_text("IdeamEcology Studio (Whittaker Biomes, Rothermel Wildfire & Trophic Web)");
    root_vbox->add_child(title_label);

    auto* layer_select = memnew(OptionButton);
    layer_select->add_item("1. Holdridge / Whittaker Life Zones", 0);
    layer_select->add_item("2. Seral Succession Stage (S0-S6)", 1);
    layer_select->add_item("3. Canopy & Understory Biomass (kg/m2)", 2);
    layer_select->add_item("4. Soil Organic Carbon & Mineral Nitrogen", 3);
    layer_select->add_item("5. Rothermel Wildfire Spread Risk & Intensity", 4);
    layer_select->add_item("6. Lotka-Volterra Herbivore & Predator Density", 5);
    layer_select->add_item("7. Evapotranspiration & Biogenic Albedo", 6);
    layer_select->add_item("8. Shannon-Wiener Biodiversity Index (H')", 7);
    layer_select->add_item("9. Alchemical Phytochemical Potency", 8);
    root_vbox->add_child(layer_select);

    auto* preview_canvas = memnew(EcologyBiomeMapPreviewCanvas);
    preview_canvas->set_biosphere_resource(bio_res);
    root_vbox->add_child(preview_canvas);

    auto* quadrat_control = memnew(WhittakerOrdinationAndTrophicControl);
    quadrat_control->set_biosphere_resource(bio_res);
    root_vbox->add_child(quadrat_control);

    auto* toolbar = memnew(HBoxContainer);
    auto* btn_regen = memnew(Button);
    btn_regen->set_text("Reset Biosphere");
    toolbar->add_child(btn_regen);

    auto* btn_step1 = memnew(Button);
    btn_step1->set_text("+1 Succession Step");
    toolbar->add_child(btn_step1);

    auto* btn_step10 = memnew(Button);
    btn_step10->set_text("+10 Steps");
    toolbar->add_child(btn_step10);

    auto* btn_fire = memnew(Button);
    btn_fire->set_text("Ignite Wildfire");
    toolbar->add_child(btn_fire);
    root_vbox->add_child(toolbar);

    layer_select->connect("item_selected", Callable(preview_canvas, "set_active_map_layer"));
    preview_canvas->connect("cell_selected", Callable(quadrat_control, "set_inspected_cell"));

    btn_regen->connect("pressed", callable_mp(bio_res.ptr(), &EcologyBiosphereResource::generate_initial_biosphere));
    btn_regen->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
    btn_regen->connect("pressed", Callable(quadrat_control, "refresh_quadrat_view"));

    btn_step1->connect("pressed", callable_mp(bio_res.ptr(), &EcologyBiosphereResource::step_simulation).bind(1, -1.0f));
    btn_step1->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
    btn_step1->connect("pressed", Callable(quadrat_control, "refresh_quadrat_view"));

    btn_step10->connect("pressed", callable_mp(bio_res.ptr(), &EcologyBiosphereResource::step_simulation).bind(10, -1.0f));
    btn_step10->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
    btn_step10->connect("pressed", Callable(quadrat_control, "refresh_quadrat_view"));

    btn_fire->connect("pressed", callable_mp(bio_res.ptr(), &EcologyBiosphereResource::ignite_wildfire_at_cell).bind(32, 32, 1800.0f));
    btn_fire->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
    btn_fire->connect("pressed", Callable(quadrat_control, "refresh_quadrat_view"));

    add_custom_control(root_vbox);
}

} // namespace ideam::godot_ext
