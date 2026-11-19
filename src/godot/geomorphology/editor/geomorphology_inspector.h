#pragma once

/**
 * @file geomorphology_inspector.h
 * @brief Thin EditorInspectorPlugin launcher for GeomorphologyTerrainResource and
 * GeomorphologyTerrainNode3D -> opens the full studio in Tier 2 `PlanetarySystemsModeler`.
 */

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/resource.hpp>

#include "godot/geomorphology/geomorphology_terrain_resource.h"

namespace ideam::godot_ext {

class GeomorphologyInspector : public godot::EditorInspectorPlugin {
    GDCLASS(GeomorphologyInspector, godot::EditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    GeomorphologyInspector() = default;
    ~GeomorphologyInspector() override = default;

    void _on_open_in_planetary_modeler(const godot::Ref<godot::Resource> &p_resource);

    bool _can_handle(godot::Object* p_object) const override;
    void _parse_begin(godot::Object* p_object) override;
    bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override { return false; }
};

} // namespace ideam::godot_ext
