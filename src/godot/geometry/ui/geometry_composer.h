#pragma once

/**
 * geometry_composer.h
 * Tier 2 Orchestration Workspace (`ToolWorkspace`) unifying Geometry, Splines, and Tiles
 * (`src/godot/geometry/ui/`).
 *
 * Sidebar Tabs:
 *   - `General`: `Polygon2DResource`, `VoronoiDiagram2DResource`, `SDFGrid2DResource`,
 *     `TensorField2DResource` (registered by `IdeamGeometryPlugin`).
 *   - `Splines`: `Spline1DResource`, `Spline2DResource`, `Spline3DResource`, `Spline4DResource`,
 *     `SplineTweenResource` (registered by `IdeamSplinesPlugin`).
 *   - `Tiles`: `WallpaperTilingResource`, `CompositeTilingResource`,
 *     `HyperbolicPoincareTilingResource`, `SphericalGoldbergPolyhedronResource`
 *     (registered by `IdeamTilesPlugin`).
 *
 * Strict Tier 2 Contract:
 *   `GeometryComposer` never directly mutates geometry, spline, or tiling data. It orchestrates
 *   the registered Tier 1 controls inside the shared 3-pane `ToolWorkspace` shell.
 */

#include "../../tools/tool_workspace.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>

namespace ideam::godot_ext {

class GeometryComposer : public ToolWorkspace {
    GDCLASS(GeometryComposer, ToolWorkspace)

private:
    static GeometryComposer *shared_instance_;
    static godot::Window *shared_window_;

    godot::Button *fit_view_btn_ = nullptr;
    godot::Button *grid_btn_     = nullptr;
    godot::Button *y_down_btn_   = nullptr;

    [[nodiscard]] godot::Control *_find_active_canvas_recursive(godot::Node *p_root) const;
    void _sync_canvas_command_buttons();

protected:
    static void _bind_methods();

    void _populate_command_bar(godot::HBoxContainer *p_custom_actions) override;
    void _on_active_asset_switched(const godot::Ref<godot::Resource> &p_asset) override;

public:
    static constexpr const char *WORKSPACE_ID = "GeometryComposer";

    GeometryComposer();
    ~GeometryComposer() override;

    // --- Command Bar Callbacks ---
    void _on_fit_view_pressed();
    void _on_grid_toggled(bool p_show);
    void _on_y_down_toggled(bool p_y_down);

    // --- Unified Static Launcher API (GraphComposer / MemoryProfiler Pattern) ---
    static GeometryComposer *get_or_create_shared_composer(godot::Control *p_owner = nullptr);
    static void open_geometry_resource(
        const godot::Ref<godot::Resource> &p_resource,
        godot::Control *p_owner = nullptr
    );
};

} // namespace ideam::godot_ext
