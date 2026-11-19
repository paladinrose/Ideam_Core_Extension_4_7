#include "godot/ecology/editor/ecology_inspector.h"
#include "godot/geomorphology/ui/planetary_systems_modeler.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void EcologyInspector::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("_on_open_in_planetary_modeler", "resource"),
        &EcologyInspector::_on_open_in_planetary_modeler
    );
}

void EcologyInspector::_on_open_in_planetary_modeler(const godot::Ref<godot::Resource> &p_resource) {
    if (p_resource.is_valid()) {
        PlanetarySystemsModeler::open_planetary_resource(p_resource);
    }
}

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

    auto *bar = memnew(HBoxContainer);
    bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    auto *lbl = memnew(Label);
    lbl->set_text("IdeamEcology Biosphere");
    lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bar->add_child(lbl);

    auto *open_btn = memnew(Button);
    open_btn->set_text("Open in PlanetarySystemsModeler");
    open_btn->set_tooltip_text("Inspect Whittaker biomes, trophic food webs, and wildfire simulations in PlanetarySystemsModeler.");
    open_btn->connect("pressed", Callable(this, "_on_open_in_planetary_modeler").bind(bio_res));
    bar->add_child(open_btn);

    add_custom_control(bar);
}

} // namespace ideam::godot_ext
