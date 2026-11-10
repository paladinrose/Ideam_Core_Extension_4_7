#pragma once

/**
 * tiling_region_preview_canvas.h
 * Interactive 2D Bounded Multi-Layer Tiling, Seam Stitch, Hexize & Pathfinding Sandbox Canvas
 * (`src/godot/tiles/ui/`), extending `GeometryCanvas2DBase` (`src/godot/geometry/ui/`).
 *
 * Responsibilities:
 *   1. Renders a `CompositeTilingResource` across four diagnostic coloring modes:
 *        - `PREVIEW_COLOR_THREE_COLORING`: Crystallographic 3-coloring (`get_cell_colour`).
 *        - `PREVIEW_COLOR_LAYER_GROUP`: Color-coded by `WallpaperTilingResource` layer slot (`get_cell_group_slot`).
 *        - `PREVIEW_COLOR_GENERATION_RING`: Topological flood-fill ring wave (`get_cell_generation`).
 *        - `PREVIEW_COLOR_CELL_FLAGS`: Interior vs. boundary-clipped vs. quad-bridge stitch cells.
 *   2. Visualizes macro-hexagon super-cells (`Hexize`), extracted closed border `Spline2DResource`
 *      loops (`FindBorders`), input region boundary splines, and draggable `layer_seed_points`.
 *   3. Provides four interactive mouse sandbox modes:
 *        - `REGION_INTERACT_INSPECT`: Click any cell to inspect its topology & highlight neighbors.
 *        - `REGION_INTERACT_CARVE_CELL`: Click cells to remove them via `commit_remove_cells` with Undo/Redo.
 *        - `REGION_INTERACT_PATHFIND_SANDBOX`: Left-click sets A* Start Cell, Right-click (or Shift+Click)
 *          sets Goal Cell, rendering both the A* path ribbon and Dijkstra/Eikonal flow-field vectors.
 *        - `REGION_INTERACT_CYCLE_EDGE_PORT`: Click shared cell edges to cycle `TilingPortDirectionType`
 *          (`Free <-> In-Only <-> Out-Only <-> Closed`) with Undo/Redo.
 */

#include "../../geometry/ui/geometry_canvas_2d_base.h"
#include "../composite_tiling_resource.h"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

enum TilingPreviewColorMode : int32_t {
    PREVIEW_COLOR_THREE_COLORING  = 0,
    PREVIEW_COLOR_LAYER_GROUP     = 1,
    PREVIEW_COLOR_GENERATION_RING = 2,
    PREVIEW_COLOR_CELL_FLAGS      = 3
};

enum TilingRegionInteractionMode : int32_t {
    REGION_INTERACT_INSPECT          = 0,
    REGION_INTERACT_CARVE_CELL       = 1,
    REGION_INTERACT_PATHFIND_SANDBOX = 2,
    REGION_INTERACT_CYCLE_EDGE_PORT  = 3
};

class TilingRegionPreviewCanvas : public GeometryCanvas2DBase {
    GDCLASS(TilingRegionPreviewCanvas, GeometryCanvas2DBase)

private:
    TilingPreviewColorMode color_mode_           = PREVIEW_COLOR_THREE_COLORING;
    TilingRegionInteractionMode interact_mode_   = REGION_INTERACT_INSPECT;

    bool show_hex_super_cells_   = true;
    bool show_border_outlines_   = true;
    bool show_flow_field_        = false;
    bool use_manhattan_path_     = false;

    int selected_cell_id_        = -1;
    int hovered_cell_id_         = -1;
    int path_start_cell_id_      = -1;
    int path_goal_cell_id_       = -1;

    godot::PackedInt32Array cached_path_cells_;
    godot::PackedVector2Array cached_path_points_;
    godot::PackedVector2Array cached_flow_vectors_;

    void _recompute_navigation_overlay();
    [[nodiscard]] godot::Color _compute_cell_fill_color(
        const godot::Ref<CompositeTilingResource> &p_comp,
        int p_cell_id,
        int p_max_gen
    ) const;

protected:
    static void _bind_methods();

    // --- GeometryCanvas2DBase Overrides (Layer Seed Points mapped as Canvas Anchors) ---
    [[nodiscard]] int _get_canvas_point_count() const override;
    [[nodiscard]] int _get_canvas_section_count() const override { return 0; }
    [[nodiscard]] bool _is_canvas_wrapped() const override { return false; }

    [[nodiscard]] godot::Vector2 _get_point_world_pos(int p_index) const override;
    void _set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) override;

    [[nodiscard]] godot::PackedVector2Array _sample_curve_screen_points(
        const godot::Rect2 &p_plot
    ) const override;
    [[nodiscard]] bool _compute_world_bounds(godot::Rect2 &r_bounds) const override;

    void _draw_plot_background_overlay(const godot::Rect2 &p_plot) override;
    void _draw_plot_foreground_overlay(const godot::Rect2 &p_plot) override;

    [[nodiscard]] bool _try_begin_custom_drag(
        const godot::Ref<godot::InputEvent> &p_event,
        const godot::Rect2 &p_plot
    ) override;
    void _update_custom_hover(
        const godot::Vector2 &p_mouse_pos,
        const godot::Rect2 &p_plot
    ) override;
    void _on_resource_bound() override;

public:
    TilingRegionPreviewCanvas();
    ~TilingRegionPreviewCanvas() override = default;

    // --- Typed Resource Binding ---
    void set_composite_tiling(const godot::Ref<CompositeTilingResource> &p_composite);
    [[nodiscard]] godot::Ref<CompositeTilingResource> get_composite_tiling() const;

    // --- Color & Interaction Modes ---
    void set_color_mode(TilingPreviewColorMode p_mode);
    [[nodiscard]] TilingPreviewColorMode get_color_mode() const noexcept { return color_mode_; }

    void set_interaction_mode(TilingRegionInteractionMode p_mode);
    [[nodiscard]] TilingRegionInteractionMode get_interaction_mode() const noexcept { return interact_mode_; }

    void set_show_hex_super_cells(bool p_show);
    [[nodiscard]] bool get_show_hex_super_cells() const noexcept { return show_hex_super_cells_; }

    void set_show_border_outlines(bool p_show);
    [[nodiscard]] bool get_show_border_outlines() const noexcept { return show_border_outlines_; }

    void set_show_flow_field(bool p_show);
    [[nodiscard]] bool get_show_flow_field() const noexcept { return show_flow_field_; }

    void set_use_manhattan_path(bool p_manhattan);
    [[nodiscard]] bool get_use_manhattan_path() const noexcept { return use_manhattan_path_; }

    void set_selected_cell_id(int p_cell_id);
    [[nodiscard]] int get_selected_cell_id() const noexcept { return selected_cell_id_; }

    void set_path_endpoints(int p_start_cell, int p_goal_cell);
    [[nodiscard]] int get_path_start_cell_id() const noexcept { return path_start_cell_id_; }
    [[nodiscard]] int get_path_goal_cell_id() const noexcept { return path_goal_cell_id_; }
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::TilingPreviewColorMode);
VARIANT_ENUM_CAST(ideam::godot_ext::TilingRegionInteractionMode);
