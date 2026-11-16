#include "godot/climatology/editor/climatology_inspector.h"

#include "godot/climatology/ui/climatology_synoptic_map_preview_canvas.h"
#include "godot/climatology/ui/thermodynamic_sounding_skewt_control.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void ClimatologyInspector::_bind_methods() {}

bool ClimatologyInspector::_can_handle(godot::Object* p_object) const {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<ClimatologyAtmosphereResource>(p_object) != nullptr ||
           godot::Object::cast_to<ClimatologyWeatherNode3D>(p_object) != nullptr;
}

void ClimatologyInspector::_parse_begin(godot::Object* p_object) {
    using namespace godot;

    Ref<ClimatologyAtmosphereResource> atmo_res;
    if (auto* direct_res = Object::cast_to<ClimatologyAtmosphereResource>(p_object)) {
        atmo_res = Ref<ClimatologyAtmosphereResource>(direct_res);
    } else if (auto* weather_node = Object::cast_to<ClimatologyWeatherNode3D>(p_object)) {
        atmo_res = weather_node->get_atmosphere_resource();
    }

    if (atmo_res.is_null()) {
        return;
    }

    auto* root_vbox = memnew(VBoxContainer);
    root_vbox->set_name("ClimatologyInspectorRoot");

    auto* title_label = memnew(Label);
    title_label->set_text("IdeamClimatology Studio (Synoptic Winds, Foehn & Supercell Tornadoes)");
    root_vbox->add_child(title_label);

    auto* layer_select = memnew(OptionButton);
    layer_select->add_item("1. Surface Air Temperature & Potential Temp", 0);
    layer_select->add_item("2. Synoptic Sea-Level Pressure (hPa)", 1);
    layer_select->add_item("3. Geostrophic Wind & Orographic Lift", 2);
    layer_select->add_item("4. Relative Humidity & Foehn Cloud Cover", 3);
    layer_select->add_item("5. Rainfall, Snowfall & Snowpack", 4);
    layer_select->add_item("6. Convective CAPE & 0-3km Helicity (SRH)", 5);
    layer_select->add_item("7. Supercell Tornado Vortex Wind (EF0-EF5)", 6);
    layer_select->add_item("8. Koppen-Geiger Climate Classification", 7);
    layer_select->add_item("9. Surface Radiative Energy Budget", 8);
    root_vbox->add_child(layer_select);

    auto* preview_canvas = memnew(ClimatologySynopticMapPreviewCanvas);
    preview_canvas->set_atmosphere_resource(atmo_res);
    root_vbox->add_child(preview_canvas);

    auto* sounding_control = memnew(ThermodynamicSoundingSkewTControl);
    sounding_control->set_atmosphere_resource(atmo_res);
    root_vbox->add_child(sounding_control);

    auto* toolbar = memnew(HBoxContainer);
    auto* btn_regen = memnew(Button);
    btn_regen->set_text("Reset Climate");
    toolbar->add_child(btn_regen);

    auto* btn_step1 = memnew(Button);
    btn_step1->set_text("+1 Weather Step");
    toolbar->add_child(btn_step1);

    auto* btn_step10 = memnew(Button);
    btn_step10->set_text("+10 Steps");
    toolbar->add_child(btn_step10);

    auto* btn_tornado = memnew(Button);
    btn_tornado->set_text("Spawn Tornado");
    toolbar->add_child(btn_tornado);
    root_vbox->add_child(toolbar);

    layer_select->connect("item_selected", Callable(preview_canvas, "set_active_map_layer"));
    preview_canvas->connect("cell_selected", Callable(sounding_control, "set_inspected_cell"));

    btn_regen->connect("pressed", callable_mp(atmo_res.ptr(), &ClimatologyAtmosphereResource::generate_initial_climate));
    btn_regen->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
    btn_regen->connect("pressed", Callable(sounding_control, "refresh_sounding_view"));

    btn_step1->connect("pressed", callable_mp(atmo_res.ptr(), &ClimatologyAtmosphereResource::step_simulation).bind(1, -1.0f, -1.0f));
    btn_step1->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
    btn_step1->connect("pressed", Callable(sounding_control, "refresh_sounding_view"));

    btn_step10->connect("pressed", callable_mp(atmo_res.ptr(), &ClimatologyAtmosphereResource::step_simulation).bind(10, -1.0f, -1.0f));
    btn_step10->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
    btn_step10->connect("pressed", Callable(sounding_control, "refresh_sounding_view"));

    btn_tornado->connect("pressed", callable_mp(atmo_res.ptr(), &ClimatologyAtmosphereResource::spawn_tornado_at_cell).bind(32, 32, -1.0f, -1.0f));
    btn_tornado->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
    btn_tornado->connect("pressed", Callable(sounding_control, "refresh_sounding_view"));

    add_custom_control(root_vbox);
}

} // namespace ideam::godot_ext
