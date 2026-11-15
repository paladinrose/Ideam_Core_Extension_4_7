#include "godot/geomorphology/editor/geomorphology_inspector.h"

#include "godot/geomorphology/ui/geomorphology_map_layer_preview_canvas.h"
#include "godot/geomorphology/ui/stratigraphic_column_inspector_control.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/callable.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::geomorphology {

void GeomorphologyInspector::_bind_methods() {}

bool GeomorphologyInspector::_can_handle(godot::Object* p_object) const {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<GeomorphologyTerrainResource>(p_object) != nullptr ||
           godot::Object::cast_to<GeomorphologyTerrainNode3D>(p_object) != nullptr;
}

void GeomorphologyInspector::_parse_begin(godot::Object* p_object) {
    using namespace godot;

    Ref<GeomorphologyTerrainResource> terrain_res;
    if (auto* direct_res = Object::cast_to<GeomorphologyTerrainResource>(p_object)) {
        terrain_res = Ref<GeomorphologyTerrainResource>(direct_res);
    } else if (auto* terrain_node = Object::cast_to<GeomorphologyTerrainNode3D>(p_object)) {
        terrain_res = terrain_node->get_terrain_resource();
    }

    if (terrain_res.is_null()) {
        return;
    }

    auto* root_vbox = memnew(VBoxContainer);
    root_vbox->set_name("GeomorphologyInspectorRoot");

    auto* title_label = memnew(Label);
    title_label->set_text("IdeamGeomorphology Studio (Tectonics, Pipe Hydrology & Karst)");
    root_vbox->add_child(title_label);

    auto* layer_select = memnew(OptionButton);
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

    auto* preview_canvas = memnew(GeomorphologyMapLayerPreviewCanvas);
    preview_canvas->set_terrain_resource(terrain_res);
    root_vbox->add_child(preview_canvas);

    auto* column_inspector = memnew(StratigraphicColumnInspectorControl);
    column_inspector->set_terrain_resource(terrain_res);
    root_vbox->add_child(column_inspector);

    auto* toolbar = memnew(HBoxContainer);
    auto* btn_regen = memnew(Button);
    btn_regen->set_text("Regenerate Seed");
    toolbar->add_child(btn_regen);

    auto* btn_step1 = memnew(Button);
    btn_step1->set_text("+1 Erosion Step");
    toolbar->add_child(btn_step1);

    auto* btn_step10 = memnew(Button);
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

    add_custom_control(root_vbox);
}

} // namespace ideam::geomorphology
