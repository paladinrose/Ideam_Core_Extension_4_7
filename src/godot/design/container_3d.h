#pragma once

/**
 * container_3d.h
 * 3D Layout Objects (`Container3D`, `BoxContainer3D`, `GridContainer3D`,
 * `RadialContainer3D`, `SplineContainer3D`) and the Functional Shell Node
 * (`SemanticStructure3D`) in `src/godot/design/`.
 */

#include "style_3d_resource.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class Container3D : public godot::Node3D {
    GDCLASS(Container3D, godot::Node3D)

private:
    godot::Vector3 cell_spacing_ = godot::Vector3(1.0f, 1.0f, 1.0f);
    bool auto_refit_children_    = true;

protected:
    static void _bind_methods();

public:
    Container3D() = default;
    ~Container3D() override = default;

    void set_cell_spacing(const godot::Vector3 &p_spacing);
    [[nodiscard]] godot::Vector3 get_cell_spacing() const noexcept { return cell_spacing_; }

    void set_auto_refit_children(bool p_auto);
    [[nodiscard]] bool get_auto_refit_children() const noexcept { return auto_refit_children_; }

    virtual void refit_children_3d();
};

/**
 * `BoxContainer3D`:
 * Stacks child `Node3D` / `SemanticStructure3D` nodes along a primary axis (`0 = X, 1 = Y, 2 = Z`),
 * honoring each child's `rest_obb_extents`, `margin_extents`, and `size_flags`.
 */
class BoxContainer3D : public Container3D {
    GDCLASS(BoxContainer3D, Container3D)

private:
    int primary_axis_ = 1; ///< `0 = X, 1 = Y (Vertical), 2 = Z`
    bool center_align_ = false;

protected:
    static void _bind_methods();

public:
    BoxContainer3D() = default;
    ~BoxContainer3D() override = default;

    void set_primary_axis(int p_axis);
    [[nodiscard]] int get_primary_axis() const noexcept { return primary_axis_; }

    void set_center_align(bool p_center);
    [[nodiscard]] bool get_center_align() const noexcept { return center_align_; }

    void refit_children_3d() override;
};

/**
 * `GridContainer3D`:
 * Arranges child `Node3D` / `SemanticStructure3D` nodes in a 3D `(columns_x, rows_y, layers_z)` lattice.
 */
class GridContainer3D : public Container3D {
    GDCLASS(GridContainer3D, Container3D)

private:
    int columns_x_ = 2;
    int rows_y_    = 1;

protected:
    static void _bind_methods();

public:
    GridContainer3D() = default;
    ~GridContainer3D() override = default;

    void set_columns_x(int p_cols);
    [[nodiscard]] int get_columns_x() const noexcept { return columns_x_; }

    void set_rows_y(int p_rows);
    [[nodiscard]] int get_rows_y() const noexcept { return rows_y_; }

    void refit_children_3d() override;
};

/**
 * `RadialContainer3D`:
 * Arranges children around a circle or helix of `radius_` (e.g., 3..6 chair legs,
 * chandelier arms, or spiral staircase treads).
 */
class RadialContainer3D : public Container3D {
    GDCLASS(RadialContainer3D, Container3D)

private:
    real_t radius_             = static_cast<real_t>(3.0);
    real_t arc_angle_degrees_  = static_cast<real_t>(360.0);
    real_t helix_height_step_  = static_cast<real_t>(0.0);

protected:
    static void _bind_methods();

public:
    RadialContainer3D() = default;
    ~RadialContainer3D() override = default;

    void set_radius(real_t p_radius);
    [[nodiscard]] real_t get_radius() const noexcept { return radius_; }

    void set_arc_angle_degrees(real_t p_degrees);
    [[nodiscard]] real_t get_arc_angle_degrees() const noexcept { return arc_angle_degrees_; }

    void set_helix_height_step(real_t p_step);
    [[nodiscard]] real_t get_helix_height_step() const noexcept { return helix_height_step_; }

    void refit_children_3d() override;
};

/**
 * `SplineContainer3D`:
 * Organic / botanical / architectural path container that distributes and tapers children
 * along a 3D polyline / spline curve (`curve_points_3d_`).
 */
class SplineContainer3D : public Container3D {
    GDCLASS(SplineContainer3D, Container3D)

private:
    godot::PackedVector3Array curve_points_3d_;
    float taper_scale_start_ = 1.0f;
    float taper_scale_end_   = 0.5f;

protected:
    static void _bind_methods();

public:
    SplineContainer3D() = default;
    ~SplineContainer3D() override = default;

    void set_curve_points_3d(const godot::PackedVector3Array &p_points);
    [[nodiscard]] godot::PackedVector3Array get_curve_points_3d() const { return curve_points_3d_; }

    void set_taper_scale_start(float p_start);
    [[nodiscard]] float get_taper_scale_start() const noexcept { return taper_scale_start_; }

    void set_taper_scale_end(float p_end);
    [[nodiscard]] float get_taper_scale_end() const noexcept { return taper_scale_end_; }

    void refit_children_3d() override;
};

/**
 * `SemanticStructure3D`:
 * The 3D `Control` Functional Shell Node (`Surface -> Wall/Floor/Seat -> Chair`,
 * `Aperture -> Door/Window`, `Organic -> Trunk/Branch`).
 * Holds a 3D Box-Model (`rest_obb_extents`, `margin_extents`, `size_flags`),
 * `SemanticRoleID`, `SemanticTagSetResource`, and `Theme3DResource`, and smartly
 * applies resolved styles and bounds to its Godot children.
 */
class SemanticStructure3D : public godot::Node3D {
    GDCLASS(SemanticStructure3D, godot::Node3D)

private:
    uint32_t semantic_role_id_              = core::SEMANTIC_ROLE_SURFACE_WALL;
    godot::Vector3 rest_obb_extents_        = godot::Vector3(0.5f, 0.5f, 0.5f);
    godot::Vector3 margin_extents_          = godot::Vector3(0.0f, 0.0f, 0.0f);
    uint8_t size_flags_x_                   = core::DESIGN_SIZE_FILL;
    uint8_t size_flags_y_                   = core::DESIGN_SIZE_FILL;
    uint8_t size_flags_z_                   = core::DESIGN_SIZE_FILL;
    float stretch_ratio_                    = 1.0f;
    core::DesignScaleMode scale_mode_       = core::DesignScaleMode::UNIFORM_FIT;
    core::EdgeSnapPolicy edge_snap_policy_  = core::EdgeSnapPolicy::SNAP_WATERTIGHT;
    godot::Ref<Theme3DResource> active_theme_3d_;
    godot::Ref<SemanticTagSetResource> semantic_tags_;

protected:
    static void _bind_methods();

public:
    SemanticStructure3D() = default;
    ~SemanticStructure3D() override = default;

    void set_semantic_role_id(int p_role_id);
    [[nodiscard]] int get_semantic_role_id() const noexcept { return static_cast<int>(semantic_role_id_); }

    void set_rest_obb_extents(const godot::Vector3 &p_extents);
    [[nodiscard]] godot::Vector3 get_rest_obb_extents() const noexcept { return rest_obb_extents_; }

    void set_margin_extents(const godot::Vector3 &p_margin);
    [[nodiscard]] godot::Vector3 get_margin_extents() const noexcept { return margin_extents_; }

    void set_stretch_ratio(float p_ratio);
    [[nodiscard]] float get_stretch_ratio() const noexcept { return stretch_ratio_; }

    void set_scale_mode(int p_mode);
    [[nodiscard]] int get_scale_mode() const noexcept { return static_cast<int>(scale_mode_); }

    void set_edge_snap_policy(int p_policy);
    [[nodiscard]] int get_edge_snap_policy() const noexcept { return static_cast<int>(edge_snap_policy_); }

    void set_active_theme_3d(const godot::Ref<Theme3DResource> &p_theme);
    [[nodiscard]] godot::Ref<Theme3DResource> get_active_theme_3d() const { return active_theme_3d_; }

    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags);
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }

    void apply_theme_and_bounds_to_children();
    [[nodiscard]] core::StructureNodePod to_structure_node_pod(uint32_t p_node_id = 0u, uint32_t p_parent_id = 0xFFFFFFFFu) const;
};

} // namespace ideam::godot_ext
