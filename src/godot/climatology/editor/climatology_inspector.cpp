#include "godot/climatology/editor/climatology_inspector.h"
#include "godot/climatology/ideam_climatology_plugin.h"
#include "godot/geomorphology/ui/planetary_systems_modeler.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void ClimatologyInspector::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("_on_open_in_planetary_modeler", "resource"),
        &ClimatologyInspector::_on_open_in_planetary_modeler
    );
}

void ClimatologyInspector::_on_open_in_planetary_modeler(const godot::Ref<godot::Resource> &p_resource) {
    if (p_resource.is_valid()) {
        PlanetarySystemsModeler::open_planetary_resource(p_resource);
    }
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *ClimatologyInspector::get_undo_redo() const {
    return IdeamClimatologyPlugin::get_static_undo_redo();
}
#endif

bool ClimatologyInspector::_can_handle(godot::Object* p_object) const {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<ClimatologyAtmosphereResource>(p_object) != nullptr ||
           godot::Object::cast_to<ClimatologyWeatherNode3D>(p_object) != nullptr;
}

void ClimatologyInspector::_parse_begin(godot::Object* p_object) {
    using namespace godot;

    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    Ref<ClimatologyAtmosphereResource> atmo_res;
    if (auto* direct_res = Object::cast_to<ClimatologyAtmosphereResource>(p_object)) {
        atmo_res = Ref<ClimatologyAtmosphereResource>(direct_res);
    } else if (auto* weather_node = Object::cast_to<ClimatologyWeatherNode3D>(p_object)) {
        atmo_res = weather_node->get_atmosphere_resource();
    }

    if (atmo_res.is_null()) {
        return;
    }

    auto *bar = memnew(HBoxContainer);
    bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    auto *lbl = memnew(Label);
    lbl->set_text("IdeamClimatology Atmosphere");
    lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bar->add_child(lbl);

    auto *open_btn = memnew(Button);
    open_btn->set_text("Open in PlanetarySystemsModeler");
    open_btn->set_tooltip_text("Inspect synoptic maps, Skew-T soundings, and weather steps in PlanetarySystemsModeler.");
    open_btn->connect("pressed", Callable(this, "_on_open_in_planetary_modeler").bind(atmo_res));
    bar->add_child(open_btn);

    add_custom_control(bar);
}

} // namespace ideam::godot_ext
