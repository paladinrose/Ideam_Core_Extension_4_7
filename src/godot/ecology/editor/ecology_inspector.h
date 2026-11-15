#pragma once

/**
 * @file ecology_inspector.h
 * @brief Custom EditorInspectorPlugin for EcologyBiosphereResource and EcologyBiosphereNode3D.
 */

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/object.hpp>

#include "godot/ecology/ecology_biosphere_resource.h"

namespace ideam::ecology {

class EcologyInspector : public godot::EditorInspectorPlugin {
    GDCLASS(EcologyInspector, godot::EditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    EcologyInspector() = default;
    ~EcologyInspector() override = default;

    bool _can_handle(godot::Object* p_object) const override;
    void _parse_begin(godot::Object* p_object) override;
};

} // namespace ideam::ecology
