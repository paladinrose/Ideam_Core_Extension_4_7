#include "godot/geomorphology/ideam_geomorphology_plugin.h"

#include "godot/tools/tool_registry.h"
#include "godot/geomorphology/ui/geomorphology_map_layer_preview_canvas.h"
#include "godot/geomorphology/ui/planetary_systems_modeler.h"
#include "godot/geomorphology/ui/stratigraphic_column_inspector_control.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void IdeamGeomorphologyPlugin::_bind_methods() {
    using namespace godot;
    ClassDB::bind_static_method(
        "IdeamGeomorphologyPlugin",
        D_METHOD("is_tasks_handshake_available"),
        &IdeamGeomorphologyPlugin::is_tasks_handshake_available);
}

void IdeamGeomorphologyPlugin::_enter_tree() {
    inspector_plugin_.instantiate();
    add_inspector_plugin(inspector_plugin_);

    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(PlanetarySystemsModeler::WORKSPACE_ID);

    reg.register_workspace_launcher({
        ws_id,
        "Planetary Systems Modeler",
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (PlanetarySystemsModeler *modeler = PlanetarySystemsModeler::get_or_create_shared_modeler()) {
                if (p_ctx.is_valid()) modeler->set_tool_context(p_ctx);
                modeler->open_asset(p_asset);
            }
        }
    });

    reg.register_control({
        ws_id, "GeomorphologyTerrainResource", "Geomorphology", "Geomorphology Terrain", UPDATE_EXPLICIT_TRIGGER,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            using namespace godot;
            (void)p_ctx;
            Ref<GeomorphologyTerrainResource> terrain_res = p_asset;
            if (terrain_res.is_null()) return nullptr;

            auto *root_vbox = memnew(VBoxContainer);
            root_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            root_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);

            auto *layer_select = memnew(OptionButton);
            layer_select->add_item("1. Elevation & Shaded Relief", 0);
            layer_select->add_item("2. Tectonic Plates & Boundary Stress", 1);
            layer_select->add_item("3. Top Exposed Lithology", 2);
            layer_select->add_item("4. Surface Water Depth & Discharge", 3);
            layer_select->add_item("5. Stream Power Fluvial Erosion", 4);
            layer_select->add_item("6. Strahler Order & Drainage Basins", 5);
            layer_select->add_item("7. Carbonate Karst Cavity Potential", 6);
            layer_select->add_item("8. Pedogenic Soil Humus Fertility", 7);
            layer_select->add_item("9. Hydrothermal & Placer Mineral Veins", 8);
            root_vbox->add_child(layer_select);

            auto *preview_canvas = memnew(GeomorphologyMapLayerPreviewCanvas);
            preview_canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            preview_canvas->set_v_size_flags(Control::SIZE_EXPAND_FILL);
            preview_canvas->set_terrain_resource(terrain_res);
            root_vbox->add_child(preview_canvas);

            auto *column_inspector = memnew(StratigraphicColumnInspectorControl);
            column_inspector->set_terrain_resource(terrain_res);
            root_vbox->add_child(column_inspector);

            auto *toolbar = memnew(HBoxContainer);
            auto *btn_regen = memnew(Button);
            btn_regen->set_text("Regenerate Seed");
            toolbar->add_child(btn_regen);

            auto *btn_step1 = memnew(Button);
            btn_step1->set_text("+1 Erosion Step");
            toolbar->add_child(btn_step1);

            auto *btn_step10 = memnew(Button);
            btn_step10->set_text("+10 Erosion Steps");
            toolbar->add_child(btn_step10);
            root_vbox->add_child(toolbar);

            layer_select->connect("item_selected", Callable(preview_canvas, "set_active_map_layer"));
            preview_canvas->connect("cell_selected", Callable(column_inspector, "set_inspected_cell"));

            btn_regen->connect("pressed", callable_mp(terrain_res.ptr(), &GeomorphologyTerrainResource::generate_initial_world));
            btn_regen->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
            btn_regen->connect("pressed", Callable(column_inspector, "refresh_column_view"));

            btn_step1->connect("pressed", callable_mp(terrain_res.ptr(), &GeomorphologyTerrainResource::step_simulation).bind(1, -1.0f, -1.0f));
            btn_step1->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
            btn_step1->connect("pressed", Callable(column_inspector, "refresh_column_view"));

            btn_step10->connect("pressed", callable_mp(terrain_res.ptr(), &GeomorphologyTerrainResource::step_simulation).bind(10, -1.0f, -1.0f));
            btn_step10->connect("pressed", Callable(preview_canvas, "rebuild_preview_texture"));
            btn_step10->connect("pressed", Callable(column_inspector, "refresh_column_view"));

            return root_vbox;
        },
        nullptr
    });
}

void IdeamGeomorphologyPlugin::_exit_tree() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(PlanetarySystemsModeler::WORKSPACE_ID);
    reg.unregister_class(ws_id, "GeomorphologyTerrainResource");
    reg.unregister_workspace_launcher(ws_id);
    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
}

} // namespace ideam::godot_ext
