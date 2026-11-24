#include "godot/ecology/ideam_ecology_plugin.h"

#include "godot/tools/tool_registry.h"
#include "godot/ecology/ui/ecology_biome_map_preview_canvas.h"
#include "godot/ecology/ui/whittaker_ordination_and_trophic_control.h"
#include "godot/geomorphology/ui/planetary_systems_modeler.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

IdeamEcologyPlugin *IdeamEcologyPlugin::singleton = nullptr;

IdeamEcologyPlugin::IdeamEcologyPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamEcologyPlugin::~IdeamEcologyPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamEcologyPlugin::get_static_undo_redo() {
    return singleton ? singleton->get_undo_redo() : nullptr;
}
#endif

void IdeamEcologyPlugin::_bind_methods() {
    using namespace godot;
    ClassDB::bind_static_method(
        "IdeamEcologyPlugin",
        D_METHOD("is_tasks_handshake_available"),
        &IdeamEcologyPlugin::is_tasks_handshake_available);
}

void IdeamEcologyPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamEcology", true);

    inspector_plugin_.instantiate();
    inspector_plugin_->set_tool_context(get_or_create_plugin_tool_context());
    add_inspector_plugin(inspector_plugin_);
    biosphere_gizmo_.instantiate();
    biosphere_gizmo_->set_tool_context(get_or_create_plugin_tool_context());
    register_ideam_gizmo(biosphere_gizmo_);

    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(PlanetarySystemsModeler::WORKSPACE_ID);

    reg.register_control({
        ws_id, "EcologyBiosphereResource", "Ecology", "Ecology Biosphere", UPDATE_EXPLICIT_TRIGGER,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            using namespace godot;
            (void)p_ctx;
            Ref<EcologyBiosphereResource> bio_res = p_asset;
            if (bio_res.is_null()) return nullptr;

            auto *root_vbox = memnew(VBoxContainer);
            root_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            root_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);

            auto *layer_select = memnew(OptionButton);
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

            auto *preview_canvas = memnew(EcologyBiomeMapPreviewCanvas);
            preview_canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            preview_canvas->set_v_size_flags(Control::SIZE_EXPAND_FILL);
            preview_canvas->set_biosphere_resource(bio_res);
            root_vbox->add_child(preview_canvas);

            auto *quadrat_control = memnew(WhittakerOrdinationAndTrophicControl);
            quadrat_control->set_biosphere_resource(bio_res);
            root_vbox->add_child(quadrat_control);

            auto *toolbar = memnew(HBoxContainer);
            auto *btn_regen = memnew(Button);
            btn_regen->set_text("Reset Biosphere");
            toolbar->add_child(btn_regen);

            auto *btn_step1 = memnew(Button);
            btn_step1->set_text("+1 Succession Step");
            toolbar->add_child(btn_step1);

            auto *btn_step10 = memnew(Button);
            btn_step10->set_text("+10 Steps");
            toolbar->add_child(btn_step10);

            auto *btn_fire = memnew(Button);
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

            return root_vbox;
        },
        nullptr
    });
}

void IdeamEcologyPlugin::_exit_tree() {
    ToolRegistry::get_singleton().unregister_class(
        godot::StringName(PlanetarySystemsModeler::WORKSPACE_ID),
        "EcologyBiosphereResource"
    );
    cleanup_ideam_spatial_registrations();
    biosphere_gizmo_.unref();
    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
    cleanup_ideam_spatial_registrations();
    set_plugin_active("IdeamEcology", false);
}

} // namespace ideam::godot_ext
