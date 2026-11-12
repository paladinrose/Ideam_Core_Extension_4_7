#pragma once

/**
 * hyperbolic_poincare_tiling_resource.h
 * Non-Euclidean Hyperbolic Poincaré Disk tiling resource, interactive canvas,
 * control panel, and editor inspector (`src/godot/tiles/`).
 *
 * Capabilities:
 *   1. `HyperbolicPoincareTilingResource` (extends `TilingResource`):
 *      - Wraps `ideam::core::HyperbolicTiling2D<real_t>`, `TilingRegion2D<real_t>`,
 *        and `TilingNavigation2D<real_t>`.
 *      - Supports regular hyperbolic Schläfli symbols `{p, q}` with `(p - 2)(q - 2) > 4`
 *        plus quick presets (`{7, 3}`, `{5, 4}`, `{4, 5}`, `{3, 7}`, `{6, 4}`, `{8, 3}`).
 *      - Configurable `generation_rings` (`0..6`), `arc_subdivisions` (`4..48`), and
 *        `mobius_focus` inside the unit disk (`|z_0| < 1`), applying an exact `SU(1, 1)`
 *        hyperbolic isometry so any cell can be smoothly recentered at the origin.
 *      - Exposes full `TilingRegion2D` cell polygons, exact geodesic arcs, cell adjacency,
 *        hyperbolic A* pathfinding (`find_hyperbolic_path_cells`, `find_hyperbolic_path_points`),
 *        and multi-goal Eikonal flow-fields (`compute_hyperbolic_flow_field`), plus
 *        `IdeamUndoRedo` `commit_*` mutations.
 *   2. `PoincareDiskTilingCanvas` (extends `GeometryCanvas2DBase`):
 *      - Interactive 2D Poincaré unit-disk viewport rendering exact circular geodesic arcs,
 *        generation-colored hyperbolic polygons, cell IDs, A* shortest-path ribbons, and
 *        flow-field arrows.
 *      - Supports Middle-Drag / Right-Drag inside the unit disk to interactively pan the
 *        hyperbolic `mobius_focus` via `SU(1, 1)` Möbius transformations, and Left-Click /
 *        Shift+Left-Click to pick A* path start and goal cells.
 *   3. `HyperbolicPoincareTilingControl` (`VBoxContainer`) &
 *      `HyperbolicPoincareTilingResourceInspector` (`TilingEditorInspectorBase`):
 *      - Embeds Schläfli `{p, q}` spinboxes and preset selector, generation ring slider,
 *        Möbius focus reset button, telemetry bar, and `PoincareDiskTilingCanvas`.
 */

#include "tiling_resource.h"
#include "editor/tiling_editor_inspector_base.h"
#include "../geometry/ui/geometry_canvas_2d_base.h"
#include "../../core/tiles/hyperbolic_tiling_2d.h"
#include "../../core/tiles/tiling_navigation_2d.h"
#include "../../core/tiles/tiling_region_2d.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

/**
 * Named presets for iconic regular hyperbolic Schläfli symbols `{p, q}`.
 */
enum HyperbolicSchlafliPreset : int32_t {
    HYPERBOLIC_PRESET_CUSTOM        = 0,
    HYPERBOLIC_PRESET_HEPTAGON_7_3  = 1, ///< {7, 3} Order-3 Heptagonal Tiling (Hurwitz surface group)
    HYPERBOLIC_PRESET_PENTAGON_5_4  = 2, ///< {5, 4} Order-4 Pentagonal Tiling
    HYPERBOLIC_PRESET_SQUARE_4_5    = 3, ///< {4, 5} Order-5 Square Tiling
    HYPERBOLIC_PRESET_TRIANGLE_3_7  = 4, ///< {3, 7} Order-7 Triangular Tiling (Dual of {7, 3})
    HYPERBOLIC_PRESET_HEXAGON_6_4   = 5, ///< {6, 4} Order-4 Hexagonal Tiling
    HYPERBOLIC_PRESET_OCTAGON_8_3   = 6  ///< {8, 3} Order-3 Octagonal Tiling (Bolza surface group)
};

class HyperbolicPoincareTilingResource : public TilingResource {
    GDCLASS(HyperbolicPoincareTilingResource, TilingResource)

private:
    using CoreHyperbolic = core::HyperbolicTiling2D<real_t>;
    using CoreRegion     = core::TilingRegion2D<real_t>;
    using CoreNav        = core::TilingNavigation2D<real_t>;
    using Vec2           = typename CoreHyperbolic::Vec2;

    int schlafli_p_                = 7;
    int schlafli_q_                = 3;
    int generation_rings_          = 3;
    int arc_subdivisions_          = 12;
    godot::Vector2 mobius_focus_   = godot::Vector2(0.0f, 0.0f);

    mutable CoreHyperbolic core_hyperbolic_;
    mutable CoreRegion core_region_;
    mutable bool is_dirty_         = true;

    void _ensure_built() const;

    [[nodiscard]] static inline Vec2 _to_glm(const godot::Vector2 &p_v) noexcept {
        return Vec2(static_cast<real_t>(p_v.x), static_cast<real_t>(p_v.y));
    }
    [[nodiscard]] static inline godot::Vector2 _to_godot(const Vec2 &p_v) noexcept {
        return godot::Vector2(static_cast<float>(p_v.x), static_cast<float>(p_v.y));
    }

protected:
    static void _bind_methods();

public:
    HyperbolicPoincareTilingResource();
    ~HyperbolicPoincareTilingResource() override;

    // --- Direct C++ Core Accessors (For `TileRegion2DProjector3D`, Controls & DOD) ---
    [[nodiscard]] const CoreHyperbolic &get_core_hyperbolic() const {
        _ensure_built();
        return core_hyperbolic_;
    }
    [[nodiscard]] const CoreRegion &get_core_region() const {
        _ensure_built();
        return core_region_;
    }
    [[nodiscard]] CoreRegion &get_core_region_mut() {
        _ensure_built();
        return core_region_;
    }

    // ========================================================================
    // TilingResource Virtual Overrides
    // ========================================================================

    [[nodiscard]] int get_vertex_count() const override { return schlafli_p_; }
    [[nodiscard]] int get_aspect_count() const override { return 1; }
    [[nodiscard]] int get_edge_shape_count() const override { return 1; }

    [[nodiscard]] godot::PackedVector2Array get_prototile_outline(int p_subdivisions = 12) const override;
    [[nodiscard]] godot::PackedInt32Array get_prototile_triangles(int p_subdivisions = 12) const override;

    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    // ========================================================================
    // Schläfli Symbol `{p, q}`, Rings, Subdivisions & Möbius Focus
    // ========================================================================

    void set_schlafli_p(int p_p);
    [[nodiscard]] int get_schlafli_p() const { return schlafli_p_; }

    void set_schlafli_q(int p_q);
    [[nodiscard]] int get_schlafli_q() const { return schlafli_q_; }

    void set_schlafli_preset(HyperbolicSchlafliPreset p_preset);
    [[nodiscard]] HyperbolicSchlafliPreset get_schlafli_preset() const;

    void set_generation_rings(int p_rings);
    [[nodiscard]] int get_generation_rings() const { return generation_rings_; }

    void set_arc_subdivisions(int p_subdivs);
    [[nodiscard]] int get_arc_subdivisions() const { return arc_subdivisions_; }

    void set_mobius_focus(const godot::Vector2 &p_focus);
    [[nodiscard]] godot::Vector2 get_mobius_focus() const { return mobius_focus_; }

    [[nodiscard]] bool is_valid_hyperbolic_symbol() const {
        return CoreHyperbolic::is_hyperbolic_schlafli(schlafli_p_, schlafli_q_);
    }
    [[nodiscard]] real_t get_central_polygon_euclidean_radius() const;

    void rebuild();

    // ========================================================================
    // Undo/Redo Committed Mutations
    // ========================================================================

    void commit_set_schlafli_symbol(int p_p, int p_q);
    void commit_set_generation_rings(int p_rings);
    void commit_set_mobius_focus(const godot::Vector2 &p_focus);

    // ========================================================================
    // Poincaré Disk Cell & Geodesic Queries
    // ========================================================================

    [[nodiscard]] int get_cell_count() const;
    [[nodiscard]] int get_edge_count() const;
    [[nodiscard]] godot::PackedVector2Array get_cell_polygon(int p_cell_id) const;
    [[nodiscard]] godot::Vector2 get_cell_center(int p_cell_id) const;
    [[nodiscard]] int get_cell_generation(int p_cell_id) const;
    [[nodiscard]] int get_cell_colour(int p_cell_id) const;
    [[nodiscard]] godot::PackedInt32Array get_cell_neighbors(int p_cell_id) const;

    [[nodiscard]] godot::PackedVector2Array sample_geodesic_arc(
        const godot::Vector2 &p_z1,
        const godot::Vector2 &p_z2,
        int p_subdivisions = 16
    ) const;
    [[nodiscard]] real_t compute_hyperbolic_distance(
        const godot::Vector2 &p_z1,
        const godot::Vector2 &p_z2
    ) const;

    // ========================================================================
    // Spatial Picking, Hyperbolic A* Pathfinding & Flow-Fields
    // ========================================================================

    [[nodiscard]] int contains_point(const godot::Vector2 &p_disk_point) const;

    [[nodiscard]] godot::PackedInt32Array find_hyperbolic_path_cells(
        int p_start_cell,
        int p_goal_cell
    ) const;
    [[nodiscard]] godot::PackedVector2Array find_hyperbolic_path_points(
        int p_start_cell,
        int p_goal_cell,
        int p_arc_subdivisions = 12
    ) const;
    [[nodiscard]] godot::PackedVector2Array compute_hyperbolic_flow_field(
        const godot::PackedInt32Array &p_goal_cells
    ) const;
};

/**
 * Interactive 2D viewport for `HyperbolicPoincareTilingResource`.
 */
class PoincareDiskTilingCanvas : public GeometryCanvas2DBase {
    GDCLASS(PoincareDiskTilingCanvas, GeometryCanvas2DBase)

private:
    godot::Ref<HyperbolicPoincareTilingResource> tiling_resource_;

    int hovered_cell_id_       = -1;
    int path_start_cell_id_    = 0;
    int path_goal_cell_id_     = -1;

    bool show_geodesic_edges_  = true;
    bool show_cell_centers_    = false;
    bool show_path_overlay_    = true;
    bool show_flow_field_      = false;
    bool dragging_mobius_      = false;
    godot::Vector2 drag_start_focus_ = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 drag_start_disk_  = godot::Vector2(0.0f, 0.0f);

    void _on_resource_changed();

protected:
    static void _bind_methods();
    void _draw_geometry_canvas() override;

public:
    PoincareDiskTilingCanvas();
    ~PoincareDiskTilingCanvas() override;

    void _gui_input(const godot::Ref<godot::InputEvent> &p_event) override;

    void set_tiling_resource(const godot::Ref<HyperbolicPoincareTilingResource> &p_resource);
    [[nodiscard]] godot::Ref<HyperbolicPoincareTilingResource> get_tiling_resource() const { return tiling_resource_; }

    void set_show_geodesic_edges(bool p_show);
    [[nodiscard]] bool get_show_geodesic_edges() const { return show_geodesic_edges_; }

    void set_show_cell_centers(bool p_show);
    [[nodiscard]] bool get_show_cell_centers() const { return show_cell_centers_; }

    void set_show_path_overlay(bool p_show);
    [[nodiscard]] bool get_show_path_overlay() const { return show_path_overlay_; }

    void set_show_flow_field(bool p_show);
    [[nodiscard]] bool get_show_flow_field() const { return show_flow_field_; }

    void set_path_endpoints(int p_start_cell, int p_goal_cell);
    [[nodiscard]] int get_path_start_cell() const { return path_start_cell_id_; }
    [[nodiscard]] int get_path_goal_cell() const { return path_goal_cell_id_; }
};

/**
 * Complete composite inspector & runtime editor control for `HyperbolicPoincareTilingResource`.
 */
class HyperbolicPoincareTilingControl : public godot::VBoxContainer {
    GDCLASS(HyperbolicPoincareTilingControl, godot::VBoxContainer)

private:
    godot::Ref<HyperbolicPoincareTilingResource> tiling_resource_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

    godot::OptionButton *preset_selector_ = nullptr;
    godot::SpinBox *spin_p_               = nullptr;
    godot::SpinBox *spin_q_               = nullptr;
    godot::SpinBox *spin_rings_           = nullptr;
    godot::CheckBox *chk_path_            = nullptr;
    godot::CheckBox *chk_flow_            = nullptr;
    godot::Button *btn_reset_focus_       = nullptr;
    godot::Button *btn_frame_disk_        = nullptr;
    godot::Label *status_label_           = nullptr;
    PoincareDiskTilingCanvas *canvas_     = nullptr;

    bool updating_ui_ = false;

    void _build_ui();
    void _sync_from_resource();
    void _on_preset_selected(int p_index);
    void _on_schlafli_changed(double p_val);
    void _on_rings_changed(double p_val);
    void _on_reset_focus_pressed();
    void _on_frame_disk_pressed();
    void _on_toggle_changed(bool p_pressed);

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    HyperbolicPoincareTilingControl();
    ~HyperbolicPoincareTilingControl() override;

    void set_tiling_resource(const godot::Ref<HyperbolicPoincareTilingResource> &p_resource);
    [[nodiscard]] godot::Ref<HyperbolicPoincareTilingResource> get_tiling_resource() const { return tiling_resource_; }

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    [[nodiscard]] PoincareDiskTilingCanvas *get_canvas() const { return canvas_; }
};

/**
 * Godot EditorInspectorPlugin for `HyperbolicPoincareTilingResource`.
 */
class HyperbolicPoincareTilingResourceInspector : public TilingEditorInspectorBase {
    GDCLASS(HyperbolicPoincareTilingResourceInspector, TilingEditorInspectorBase)

protected:
    static void _bind_methods();

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;

public:
    HyperbolicPoincareTilingResourceInspector() = default;
    ~HyperbolicPoincareTilingResourceInspector() override = default;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::HyperbolicSchlafliPreset);