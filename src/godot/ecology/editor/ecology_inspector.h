#pragma once

/**
 * @file ecology_inspector.h
 * @brief Thin EditorInspectorPlugin launcher for EcologyBiosphereResource and
 * EcologyBiosphereNode3D -> opens the full studio in Tier 2 `PlanetarySystemsModeler`.
 */

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/resource.hpp>

#include "godot/ecology/ecology_biosphere_resource.h"

namespace ideam::godot_ext {

class EcologyInspector : public godot::EditorInspectorPlugin {
    GDCLASS(EcologyInspector, godot::EditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    EcologyInspector() = default;
    ~EcologyInspector() override = default;

    void _on_open_in_planetary_modeler(const godot::Ref<godot::Resource> &p_resource);

    bool _can_handle(godot::Object* p_object) const override;
    void _parse_begin(godot::Object* p_object) override;
    bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override { return false; }
};

} // namespace ideam::godot_ext
