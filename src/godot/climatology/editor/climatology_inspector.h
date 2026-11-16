#pragma once

/**
 * @file climatology_inspector.h
 * @brief Custom EditorInspectorPlugin for ClimatologyAtmosphereResource and ClimatologyWeatherNode3D.
 */

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/object.hpp>

#include "godot/climatology/climatology_weather_resource.h"

namespace ideam::godot_ext {

class ClimatologyInspector : public godot::EditorInspectorPlugin {
    GDCLASS(ClimatologyInspector, godot::EditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    ClimatologyInspector() = default;
    ~ClimatologyInspector() override = default;

    bool _can_handle(godot::Object* p_object) const override;
    void _parse_begin(godot::Object* p_object) override;
    bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override { return false; }
};

} // namespace ideam::godot_ext
