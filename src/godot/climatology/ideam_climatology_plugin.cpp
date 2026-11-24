#include "godot/climatology/ideam_climatology_plugin.h"

#include "godot/climatology/ui/climatology_synoptic_map_preview_canvas.h"
#include "godot/climatology/ui/thermodynamic_sounding_skewt_control.h"
#include "godot/tools/tool_registry.h"
#include "godot/geomorphology/ui/planetary_systems_modeler.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

IdeamClimatologyPlugin *IdeamClimatologyPlugin::singleton = nullptr;

IdeamClimatologyPlugin::IdeamClimatologyPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamClimatologyPlugin::~IdeamClimatologyPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamClimatologyPlugin::get_static_undo_redo() {
    return singleton ? singleton->get_undo_redo() : nullptr;
}
#endif

void IdeamClimatologyPlugin::_bind_methods() {
    using namespace godot;
    ClassDB::bind_static_method(
        "IdeamClimatologyPlugin",
        D_METHOD("is_tasks_handshake_available"),
        &IdeamClimatologyPlugin::is_tasks_handshake_available);
}

void IdeamClimatologyPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamClimatology", true);

    inspector_plugin_.instantiate();
    inspector_plugin_->set_tool_context(get_or_create_plugin_tool_context());
    add_inspector_plugin(inspector_plugin_);
    weather_gizmo_.instantiate();
    weather_gizmo_->set_tool_context(get_or_create_plugin_tool_context());
    register_ideam_gizmo(weather_gizmo_);

    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(PlanetarySystemsModeler::WORKSPACE_ID);

    reg.register_control({
        ws_id, "ClimatologyAtmosphereResource", "Climatology", "Climatology Atmosphere", UPDATE_EXPLICIT_TRIGGER,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            using namespace godot;
            (void)p_ctx;
            Ref<ClimatologyAtmosphereResource> atmo_res = p_asset;
            if (atmo_res.is_null()) return nullptr;

            auto *root_vbox = memnew(VBoxContainer);
            root_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            root_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);

            auto *layer_select = memnew(OptionButton);
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

            auto *preview_canvas = memnew(ClimatologySynopticMapPreviewCanvas);
            preview_canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            preview_canvas->set_v_size_flags(Control::SIZE_EXPAND_FILL);
            preview_canvas->set_atmosphere_resource(atmo_res);
            root_vbox->add_child(preview_canvas);

            auto *sounding_control = memnew(ThermodynamicSoundingSkewTControl);
            sounding_control->set_atmosphere_resource(atmo_res);
            root_vbox->add_child(sounding_control);

            auto *toolbar = memnew(HBoxContainer);
            auto *btn_regen = memnew(Button);
            btn_regen->set_text("Reset Climate");
            toolbar->add_child(btn_regen);

            auto *btn_step1 = memnew(Button);
            btn_step1->set_text("+1 Weather Step");
            toolbar->add_child(btn_step1);

            auto *btn_step10 = memnew(Button);
            btn_step10->set_text("+10 Steps");
            toolbar->add_child(btn_step10);

            auto *btn_tornado = memnew(Button);
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

            return root_vbox;
        },
        nullptr
    });
}

void IdeamClimatologyPlugin::_exit_tree() {
    ToolRegistry::get_singleton().unregister_class(
        godot::StringName(PlanetarySystemsModeler::WORKSPACE_ID),
        "ClimatologyAtmosphereResource"
    );
    cleanup_ideam_spatial_registrations();
    weather_gizmo_.unref();
    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
    cleanup_ideam_spatial_registrations();
    set_plugin_active("IdeamClimatology", false);
}

} // namespace ideam::godot_ext
