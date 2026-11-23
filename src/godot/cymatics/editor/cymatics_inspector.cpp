#include "cymatics_inspector.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

void CymaticsEditorInspectorPlugin::_bind_methods() {}

bool CymaticsEditorInspectorPlugin::_can_handle(godot::Object *p_object) const {
    return godot::Object::cast_to<CymaticMap>(p_object) != nullptr;
}

void CymaticsEditorInspectorPlugin::_parse_begin(godot::Object *p_object) {
    CymaticMap *cmap = godot::Object::cast_to<CymaticMap>(p_object);
    if (!cmap) {
        return;
    }

    auto *box = memnew(godot::VBoxContainer);
    box->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);

    auto *lbl = memnew(godot::Label);
    lbl->set_text("CymaticMap Live Modal Field Preview");
    box->add_child(lbl);

    auto *grid = memnew(HeatmapGrid);
    grid->set_custom_minimum_size(godot::Vector2(200.0f, 180.0f));
    grid->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    grid->set_data(cmap->get_cymatic_field_bytes());
    box->add_child(grid);

    add_custom_control(box);
}
bool CymaticsEditorInspectorPlugin::_parse_property(
    godot::Object *p_object,
    godot::Variant::Type p_type,
    const godot::String &p_name,
    godot::PropertyHint p_hint_type,
    const godot::String &p_hint_string,
    godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
    bool p_wide
) {
    (void)p_object;
    (void)p_type;
    (void)p_name;
    (void)p_hint_type;
    (void)p_hint_string;
    (void)p_usage_flags;
    (void)p_wide;
    return false;
}
} // namespace ideam::godot_ext
