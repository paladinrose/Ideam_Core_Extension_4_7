#pragma once

/**
 * spherical_goldberg_polyhedron_resource.h
 * Spherical Goldberg Polyhedron (`GP(f, 0)`) planetary hex/pentagon grid resource,
 * 3D planetary node (`GeodesicPlanetNode3D`), 3D viewport gizmo (`GeodesicPlanetGizmo`),
 * and editor inspector (`src/godot/tiles/`).
 *
 * Capabilities:
 *   1. `SphericalGoldbergPolyhedronResource` (extends `TilingResource`):
 *      - Wraps `ideam::core::SphericalGoldbergPolyhedron<real_t>`, `TilingRegion2D<real_t>`,
 *        and `TilingNavigation2D<real_t>`.
 *      - Subdivides a regular icosahedron by frequency `f >= 1` (`10*f^2 + 2` cells:
 *        exactly 12 regular pentagons at the icosahedral poles and `10*(f^2 - 1)` hexagons).
 *      - Stores optional per-cell radial elevation offsets (`cell_elevations`) and per-cell
 *        biome/colour tags (`cell_tags`), with `IdeamUndoRedo` `commit_*` mutations.
 *      - Supports O(N) 3D unit-direction cell lookup (`locate_cell_from_direction`),
 *        spherical great-circle distance queries, spherical A* pathfinding across the
 *        entire closed globe (`find_spherical_path_cells`, `find_spherical_path_points_3d`),
 *        and planetary multi-goal Eikonal flow-fields (`compute_spherical_flow_field_3d`).
 *   2. `GeodesicPlanetNode3D` (extends `godot::Node3D`):
 *      - Renders the 3D Goldberg polyhedron as a `RenderingServer` `ArrayMesh` with radial
 *        prism extrusion per tile, configurable `tile_inset_ratio` bevels, optional
 *        `Spline1DResource` latitude/elevation profile modulation, optional `Spline4DResource`
 *        hypsometric biome color ramp, and a 3D spherical A* path ribbon overlay.
 *      - Exposes `raycast_pick_cell_3d(ray_origin, ray_dir)` and `get_cell_surface_transform_3d(cell_id)`
 *        for placing 3D `GamePiece` / `GameAgent` units directly on spherical tiles.
 *   3. `GeodesicPlanetGizmo` (extends `godot::EditorNode3DGizmoPlugin`):
 *      - Interactive 3D viewport gizmo for `GeodesicPlanetNode3D` with a primary planet
 *        radius handle and per-pentagon/selected-tile radial elevation handles.
 *   4. `SphericalGoldbergControl` & `SphericalGoldbergResourceInspector`:
 *      - Orthographic interactive hemisphere preview + frequency/radius controls.
 */

#include "tiling_resource.h"
#include "editor/tiling_editor_inspector_base.h"
#include "../geometry/ui/geometry_canvas_2d_base.h"
#include "../splines/spline_1d/spline_1d_resource.h"
#include "../splines/spline_4d/spline_4d_resource.h"
#include "../../core/tiles/spherical_goldberg_polyhedron.h"
#include "../../core/tiles/tiling_navigation_2d.h"
#include "../../core/tiles/tiling_region_2d.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class SphericalGoldbergPolyhedronResource : public TilingResource {
    GDCLASS(SphericalGoldbergPolyhedronResource, TilingResource)

private:
    using CoreGoldberg = core::SphericalGoldbergPolyhedron<real_t>;
    using CoreRegion   = core::TilingRegion2D<real_t>;
    using CoreNav      = core::TilingNavigation2D<real_t>;
    using Vec3         = typename CoreGoldberg::Vec3;

    int subdivision_frequency_               = 3;
    real_t planet_radius_                    = static_cast<real_t>(5.0);
    godot::PackedFloat32Array cell_elevations_;
    godot::PackedInt32Array cell_tags_;

    mutable CoreGoldberg core_goldberg_;
    mutable CoreRegion core_region_;
    mutable bool is_dirty_                   = true;

    void _ensure_built() const;

    [[nodiscard]] static inline Vec3 _to_glm3(const godot::Vector3 &p_v) noexcept {
        return Vec3(static_cast<real_t>(p_v.x), static_cast<real_t>(p_v.y), static_cast<real_t>(p_v.z));
    }
    [[nodiscard]] static inline godot::Vector3 _to_godot3(const Vec3 &p_v) noexcept {
        return godot::Vector3(static_cast<float>(p_v.x), static_cast<float>(p_v.y), static_cast<float>(p_v.z));
    }

protected:
    static void _bind_methods();

public:
    SphericalGoldbergPolyhedronResource();
    ~SphericalGoldbergPolyhedronResource() override;

    [[nodiscard]] int get_geometry_dimension() const override { return 3; }

    // --- Direct C++ Core Accessors ---
    [[nodiscard]] const CoreGoldberg &get_core_goldberg() const {
        _ensure_built();
        return core_goldberg_;
    }
    [[nodiscard]] const CoreRegion &get_core_region() const {
        _ensure_built();
        return core_region_;
    }

    // ========================================================================
    // TilingResource Virtual Overrides
    // ========================================================================

    [[nodiscard]] int get_vertex_count() const override { return 6; }
    [[nodiscard]] int get_aspect_count() const override { return 2; } // Hexagon + Pentagon
    [[nodiscard]] int get_edge_shape_count() const override { return 1; }

    [[nodiscard]] godot::PackedVector2Array get_prototile_outline(int p_subdivisions = 12) const override;
    [[nodiscard]] godot::PackedInt32Array get_prototile_triangles(int p_subdivisions = 12) const override;

    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    // ========================================================================
    // Frequency, Radius & Per-Cell Elevations / Tags
    // ========================================================================

    void set_subdivision_frequency(int p_freq);
    [[nodiscard]] int get_subdivision_frequency() const { return subdivision_frequency_; }

    void set_planet_radius(real_t p_radius);
    [[nodiscard]] real_t get_planet_radius() const { return planet_radius_; }

    void set_cell_elevations(const godot::PackedFloat32Array &p_elevations);
    [[nodiscard]] godot::PackedFloat32Array get_cell_elevations() const;

    void set_cell_elevation(int p_cell_id, real_t p_elevation);
    [[nodiscard]] real_t get_cell_elevation(int p_cell_id) const;

    void set_cell_tags(const godot::PackedInt32Array &p_tags);
    [[nodiscard]] godot::PackedInt32Array get_cell_tags() const;

    void set_cell_tag(int p_cell_id, int p_tag);
    [[nodiscard]] int get_cell_tag(int p_cell_id) const;

    void rebuild();

    // ========================================================================
    // Undo/Redo Committed Mutations
    // ========================================================================

    void commit_set_subdivision_frequency(int p_freq);
    void commit_set_planet_radius(real_t p_radius);
    void commit_set_cell_elevation(int p_cell_id, real_t p_elevation);
    void commit_set_cell_tag(int p_cell_id, int p_tag);

    // ========================================================================
    // Spherical Cell Topology & 3D Geometry Queries
    // ========================================================================

    [[nodiscard]] int get_cell_count() const;
    [[nodiscard]] int get_pentagon_count() const { return 12; }
    [[nodiscard]] int get_hexagon_count() const;

    [[nodiscard]] bool is_cell_pentagon(int p_cell_id) const;
    [[nodiscard]] godot::Vector3 get_cell_unit_normal(int p_cell_id) const;
    [[nodiscard]] godot::Vector3 get_cell_surface_center_3d(int p_cell_id) const;
    [[nodiscard]] godot::PackedVector3Array get_cell_polygon_3d(int p_cell_id, real_t p_inset_ratio = 0.0f) const;
    [[nodiscard]] godot::Transform3D get_cell_surface_transform_3d(int p_cell_id) const;
    [[nodiscard]] godot::PackedInt32Array get_cell_neighbors(int p_cell_id) const;
    [[nodiscard]] godot::PackedInt32Array get_pentagon_cell_ids() const;

    // ========================================================================
    // Spherical Picking, Great-Circle Pathfinding & Planetary Flow-Fields
    // ========================================================================

    [[nodiscard]] int locate_cell_from_direction(const godot::Vector3 &p_direction) const;

    [[nodiscard]] godot::PackedInt32Array find_spherical_path_cells(
        int p_start_cell,
        int p_goal_cell
    ) const;
    [[nodiscard]] godot::PackedVector3Array find_spherical_path_points_3d(
        int p_start_cell,
        int p_goal_cell,
        real_t p_radial_offset = 0.08f
    ) const;
    [[nodiscard]] godot::PackedVector3Array compute_spherical_flow_field_3d(
        const godot::PackedInt32Array &p_goal_cells
    ) const;
};

/**
 * 3D Scene Node rendering a `SphericalGoldbergPolyhedronResource` as a hex/pentagon
 * planetary mesh with radial tile extrusion, bevel insets, spline-driven elevation/biome
 * coloring, 3D raycast tile picking, and 3D spherical A* path visualization.
 */
class GeodesicPlanetNode3D : public godot::Node3D {
    GDCLASS(GeodesicPlanetNode3D, godot::Node3D)

private:
    godot::Ref<SphericalGoldbergPolyhedronResource> goldberg_resource_;
    godot::Ref<Spline1DResource> latitude_elevation_spline_;
    godot::Ref<Spline4DResource> biome_color_spline_;

    real_t tile_inset_ratio_          = static_cast<real_t>(0.06);
    real_t base_tile_extrusion_       = static_cast<real_t>(0.12);
    real_t latitude_elevation_scale_  = static_cast<real_t>(0.0);

    godot::Color pentagon_color_      = godot::Color(0.92f, 0.56f, 0.22f, 1.0f);
    godot::Color hexagon_color_a_     = godot::Color(0.22f, 0.54f, 0.78f, 1.0f);
    godot::Color hexagon_color_b_     = godot::Color(0.28f, 0.68f, 0.56f, 1.0f);
    godot::Color side_skirt_color_    = godot::Color(0.14f, 0.20f, 0.28f, 1.0f);

    int highlighted_cell_id_          = -1;
    int path_start_cell_id_           = -1;
    int path_goal_cell_id_            = -1;

    godot::Ref<godot::Material> material_override_;
    godot::Ref<godot::StandardMaterial3D> default_vertex_color_material_;
    godot::Ref<godot::ArrayMesh> mesh_;
    godot::RID mesh_instance_rid_;

    void _on_dependency_changed();
    void _disconnect_dependencies();
    void _connect_dependencies();

protected:
    static void _bind_methods();

public:
    GeodesicPlanetNode3D();
    ~GeodesicPlanetNode3D() override;

    void _notification(int p_what);

    void set_goldberg_resource(const godot::Ref<SphericalGoldbergPolyhedronResource> &p_res);
    [[nodiscard]] godot::Ref<SphericalGoldbergPolyhedronResource> get_goldberg_resource() const { return goldberg_resource_; }

    void set_latitude_elevation_spline(const godot::Ref<Spline1DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline1DResource> get_latitude_elevation_spline() const { return latitude_elevation_spline_; }

    void set_biome_color_spline(const godot::Ref<Spline4DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline4DResource> get_biome_color_spline() const { return biome_color_spline_; }

    void set_tile_inset_ratio(real_t p_ratio);
    [[nodiscard]] real_t get_tile_inset_ratio() const { return tile_inset_ratio_; }

    void set_base_tile_extrusion(real_t p_extrusion);
    [[nodiscard]] real_t get_base_tile_extrusion() const { return base_tile_extrusion_; }

    void set_latitude_elevation_scale(real_t p_scale);
    [[nodiscard]] real_t get_latitude_elevation_scale() const { return latitude_elevation_scale_; }

    void set_pentagon_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_pentagon_color() const { return pentagon_color_; }

    void set_hexagon_color_a(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_hexagon_color_a() const { return hexagon_color_a_; }

    void set_hexagon_color_b(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_hexagon_color_b() const { return hexagon_color_b_; }

    void set_side_skirt_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_side_skirt_color() const { return side_skirt_color_; }

    void set_highlighted_cell_id(int p_cell_id);
    [[nodiscard]] int get_highlighted_cell_id() const { return highlighted_cell_id_; }

    void set_path_endpoints(int p_start_cell, int p_goal_cell);
    [[nodiscard]] int get_path_start_cell() const { return path_start_cell_id_; }
    [[nodiscard]] int get_path_goal_cell() const { return path_goal_cell_id_; }

    void set_material_override(const godot::Ref<godot::Material> &p_material);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    [[nodiscard]] godot::Ref<godot::ArrayMesh> get_generated_mesh() const { return mesh_; }

    [[nodiscard]] real_t compute_effective_cell_radius(int p_cell_id) const;
    [[nodiscard]] int raycast_pick_cell_3d(
        const godot::Vector3 &p_world_ray_origin,
        const godot::Vector3 &p_world_ray_dir
    ) const;

    void rebuild_mesh();
};

/**
 * Interactive 3D Viewport Gizmo Plugin for `GeodesicPlanetNode3D`.
 * Handle 0 controls `planet_radius`; Handles 1..12 control radial elevation on the 12 pentagon hubs.
 */
class GeodesicPlanetGizmo : public godot::EditorNode3DGizmoPlugin {
    GDCLASS(GeodesicPlanetGizmo, godot::EditorNode3DGizmoPlugin)

private:
    real_t drag_initial_value_ = static_cast<real_t>(0.0);

protected:
    static void _bind_methods();

public:
    GeodesicPlanetGizmo();
    ~GeodesicPlanetGizmo() override = default;

    bool _has_gizmo(godot::Node3D *p_for_node_3d) const override;
    godot::String _get_gizmo_name() const override;
    int32_t _get_priority() const override;

    void _redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) override;
    godot::String _get_handle_name(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary
    ) const override;
    godot::Variant _get_handle_value(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary
    ) const override;
    void _begin_handle_action(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary
    ) override;
    void _set_handle(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary,
        godot::Camera3D *p_camera,
        const godot::Vector2 &p_screen_pos
    ) override;
    void _commit_handle(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary,
        const godot::Variant &p_restore,
        bool p_cancel
    ) override;
};

/**
 * Interactive orthographic globe preview canvas & control for `SphericalGoldbergPolyhedronResource`.
 */
class SphericalGoldbergPreviewCanvas : public GeometryCanvas2DBase {
    GDCLASS(SphericalGoldbergPreviewCanvas, GeometryCanvas2DBase)

private:
    godot::Ref<SphericalGoldbergPolyhedronResource> resource_;
    real_t yaw_radians_   = static_cast<real_t>(0.45);
    real_t pitch_radians_ = static_cast<real_t>(0.32);
    bool rotating_globe_  = false;
    godot::Vector2 last_mouse_pos_;

    void _on_resource_changed();

protected:
    static void _bind_methods();
    void _draw_geometry_canvas() override;

public:
    SphericalGoldbergPreviewCanvas();
    ~SphericalGoldbergPreviewCanvas() override;

    void _gui_input(const godot::Ref<godot::InputEvent> &p_event) override;

    void set_goldberg_resource(const godot::Ref<SphericalGoldbergPolyhedronResource> &p_res);
    [[nodiscard]] godot::Ref<SphericalGoldbergPolyhedronResource> get_goldberg_resource() const { return resource_; }
};

class SphericalGoldbergControl : public godot::VBoxContainer {
    GDCLASS(SphericalGoldbergControl, godot::VBoxContainer)

private:
    godot::Ref<SphericalGoldbergPolyhedronResource> resource_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

    godot::SpinBox *spin_frequency_            = nullptr;
    godot::SpinBox *spin_radius_               = nullptr;
    godot::Button *btn_reset_elevations_       = nullptr;
    godot::Label *summary_label_               = nullptr;
    SphericalGoldbergPreviewCanvas *canvas_    = nullptr;

    bool updating_ui_ = false;

    void _build_ui();
    void _sync_from_resource();
    void _on_frequency_changed(double p_val);
    void _on_radius_changed(double p_val);
    void _on_reset_elevations_pressed();

protected:
    static void _bind_methods();

public:
    SphericalGoldbergControl();
    ~SphericalGoldbergControl() override;

    void set_goldberg_resource(const godot::Ref<SphericalGoldbergPolyhedronResource> &p_res);
    [[nodiscard]] godot::Ref<SphericalGoldbergPolyhedronResource> get_goldberg_resource() const { return resource_; }

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }
};

class SphericalGoldbergResourceInspector : public TilingEditorInspectorBase {
    GDCLASS(SphericalGoldbergResourceInspector, TilingEditorInspectorBase)

protected:
    static void _bind_methods();

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;

public:
    SphericalGoldbergResourceInspector() = default;
    ~SphericalGoldbergResourceInspector() override = default;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext