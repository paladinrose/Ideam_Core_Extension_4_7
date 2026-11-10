#include "wallpaper_tiling_resource_inspector.h"

#include "../ui/wallpaper_tiling_control.h"
#include "../wallpaper_tiling_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace ideam::godot_ext {

void WallpaperTilingResourceInspector::_bind_methods() {
}

WallpaperTilingResourceInspector::WallpaperTilingResourceInspector() = default;

WallpaperTilingResourceInspector::~WallpaperTilingResourceInspector() = default;

bool WallpaperTilingResourceInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) return false;
    return godot::Object::cast_to<WallpaperTilingResource>(p_object) != nullptr;
}

godot::Control *WallpaperTilingResourceInspector::_create_inspector_control(godot::Object *p_object) {
    WallpaperTilingResource *tiling = godot::Object::cast_to<WallpaperTilingResource>(p_object);
    if (!tiling) return nullptr;

    WallpaperTilingControl *ctrl = memnew(WallpaperTilingControl);
    ctrl->set_wallpaper_tiling(godot::Ref<WallpaperTilingResource>(tiling));
    return ctrl;
}

} // namespace ideam::godot_ext
