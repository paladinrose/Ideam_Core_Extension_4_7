#pragma once

/**
 * wallpaper_tiling_resource_inspector.h
 * EditorInspectorPlugin for `WallpaperTilingResource` (`src/godot/tiles/editor/`).
 *
 * Mounts `WallpaperTilingControl` (`WallpaperGroupSelectorControl` + `TileFundamentalDomainCanvas`
 * + Tactile vertex parameter sliders + Escher edge spline sculptor) as the primary Inspector UI.
 */

#include "tiling_editor_inspector_base.h"

namespace ideam::godot_ext {

class WallpaperTilingResourceInspector : public TilingEditorInspectorBase {
    GDCLASS(WallpaperTilingResourceInspector, TilingEditorInspectorBase)

protected:
    static void _bind_methods();

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;

public:
    WallpaperTilingResourceInspector();
    ~WallpaperTilingResourceInspector() override;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext
