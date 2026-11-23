#pragma once

/**
 * cymatics_inspector.h
 * EditorInspectorPlugin previewing `CymaticMap` live inside a `HeatmapGrid`
 * (`src/godot/cymatics/editor/`).
 */

#include "../cymatic_map.h"
#include "../../controls/heatmap_grid.h"
#include "../../editor/ideam_editor_inspector_plugin.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class CymaticsEditorInspectorPlugin : public IdeamEditorInspectorPlugin {
    GDCLASS(CymaticsEditorInspectorPlugin, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    CymaticsEditorInspectorPlugin() = default;
    ~CymaticsEditorInspectorPlugin() override = default;

    bool _can_handle(godot::Object *p_object) const override;
    void _parse_begin(godot::Object *p_object) override;
    bool _parse_property(
    godot::Object *p_object,
    godot::Variant::Type p_type,
    const godot::String &p_name,
    godot::PropertyHint p_hint_type,
    const godot::String &p_hint_string,
    godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
    bool p_wide
) override;

};

} // namespace ideam::godot_ext
