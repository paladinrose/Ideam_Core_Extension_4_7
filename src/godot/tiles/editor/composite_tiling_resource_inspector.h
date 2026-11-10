#pragma once

/**
 * composite_tiling_resource_inspector.h
 * EditorInspectorPlugin for `CompositeTilingResource` (`src/godot/tiles/editor/`).
 *
 * Mounts `CompositeTilingControl` (`TilingRegionPreviewCanvas` + coverage/seam-stitch
 * selectors + Hexize & A* pathfinding sandbox) as the primary Inspector UI while keeping
 * the `tiling_layers`, `region_splines`, `layer_priorities`, `layer_seed_points`, and
 * `fallback_aabb` resource array properties editable below.
 */

#include "tiling_editor_inspector_base.h"

namespace ideam::godot_ext {

class CompositeTilingResourceInspector : public TilingEditorInspectorBase {
    GDCLASS(CompositeTilingResourceInspector, TilingEditorInspectorBase)

protected:
    static void _bind_methods();

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;

public:
    CompositeTilingResourceInspector();
    ~CompositeTilingResourceInspector() override;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext
