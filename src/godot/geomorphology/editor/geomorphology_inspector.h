#pragma once

/**
 * @file geomorphology_inspector.h
 * @brief Custom EditorInspectorPlugin for GeomorphologyTerrainResource and GeomorphologyTerrainNode3D.
 *
 * Embeds:
 * - Interactive Map Layer selector (9 diagnostic geological/hydrological layers)
 * - GeomorphologyMapLayerPreviewCanvas with click-to-inspect borehole selection
 * - StratigraphicColumnInspectorControl vertical bore-hole log
 * - World Generation & Simulation Stepping toolbar (Regenerate from Seed, +1 Step, +10 Steps)
 *   with IdeamUndoRedo snapshot integration.
 */

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/object.hpp>

#include "godot/geomorphology/geomorphology_terrain_resource.h"

namespace ideam::godot_ext {

class GeomorphologyInspector : public godot::EditorInspectorPlugin {
    GDCLASS(GeomorphologyInspector, godot::EditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    GeomorphologyInspector() = default;
    ~GeomorphologyInspector() override = default;

    bool _can_handle(godot::Object* p_object) const override;
    void _parse_begin(godot::Object* p_object) override;
    bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override { return false; }
};

} // namespace ideam::godot_ext
