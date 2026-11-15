#pragma once

/**
 * @file climatology_inspector.h
 * @brief Custom EditorInspectorPlugin for ClimatologyAtmosphereResource and ClimatologyWeatherNode3D.
 */

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/object.hpp>

#include "godot/climatology/climatology_weather_resource.h"

namespace ideam::climatology {

class ClimatologyInspector : public godot::EditorInspectorPlugin {
    GDCLASS(ClimatologyInspector, godot::EditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    ClimatologyInspector() = default;
    ~ClimatologyInspector() override = default;

    bool _can_handle(godot::Object* p_object) const override;
    void _parse_begin(godot::Object* p_object) override;
};

} // namespace ideam::climatology
