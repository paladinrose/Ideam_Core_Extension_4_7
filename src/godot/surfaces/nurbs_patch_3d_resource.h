#pragma once

/**
 * nurbs_patch_3d_resource.h
 * Concrete Tensor-Product NURBS / B-Spline 3D Surface Patch Resource (`src/godot/surfaces/`).
 *
 * Wraps `ideam::core::NurbsPatch3D<real_t>` with:
 *   - Order-independent `.tres` serialization (`count_u`, `count_v`, `degree_u`, `degree_v`,
 *     `control_points`, `weights`, `wrap_u`, `wrap_v`).
 *   - Optional live lofting from an array of `Spline3DResource` profile ribs (`loft_from_splines`).
 *   - Interactive control-point editing with `commit_set_control_point` / `commit_apply_snapshot`
 *     routed through `IdeamUndoRedo`.
 *   - Built-in presets (`setup_flat_grid`, `setup_cylinder_patch`, `setup_dome_patch`).
 */

#include "surface_patch_resource.h"
#include "../../core/surfaces/nurbs_patch_3d.h"
#include "../splines/spline_3d/spline_3d_resource.h"

#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

class NurbsPatch3DResource : public SurfacePatchResource {
    GDCLASS(NurbsPatch3DResource, SurfacePatchResource)

private:
    using CorePatch = core::NurbsPatch3D<real_t>;
    using Vec3      = typename CorePatch::Vec3;

    CorePatch core_patch_;
    godot::TypedArray<Spline3DResource> loft_ribs_;
    int loft_samples_u_ = 6;
    bool loft_arc_length_ = true;

    // Staging caches for order-independent .tres deserialization
    int pending_count_u_ = 0;
    int pending_count_v_ = 0;
    godot::PackedVector3Array pending_control_points_;
    godot::PackedFloat32Array pending_weights_;

    void _apply_pending_grid();
    void _connect_rib_signals();
    void _disconnect_rib_signals();
    void _on_rib_spline_changed();

protected:
    static void _bind_methods();

public:
    NurbsPatch3DResource();
    ~NurbsPatch3DResource() override;

    // --- Direct C++ Core Accessors ---
    [[nodiscard]] const CorePatch &get_core_patch() const noexcept { return core_patch_; }
    [[nodiscard]] CorePatch &get_core_patch_mut() noexcept { return core_patch_; }

    // --- SurfacePatchResource Overrides ---
    [[nodiscard]] bool is_patch_valid() const override { return core_patch_.is_valid(); }
    [[nodiscard]] godot::String get_patch_type_label() const override { return "NURBS Patch"; }

    [[nodiscard]] core::SurfaceFrame3DPOD<real_t> sample_core_frame(
        real_t p_u,
        real_t p_v,
        uint32_t p_patch_id = 0u
    ) const override {
        return core_patch_.sample_frame(p_u, p_v, p_patch_id);
    }

    [[nodiscard]] core::SurfaceQuadMesh3D<real_t> tessellate_core_quad_mesh(
        int p_lod_level = 0,
        uint32_t p_patch_id = 0u
    ) const override {
        const uint8_t lod = static_cast<uint8_t>(std::clamp(p_lod_level, 0, 3));
        return core_patch_.tessellate_quad_mesh(
            static_cast<uint32_t>(base_u_divs_),
            static_cast<uint32_t>(base_v_divs_),
            lod,
            p_patch_id
        );
    }

    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    // --- Grid & Degree Properties ---
    void set_count_u(int p_count_u);
    [[nodiscard]] int get_count_u() const noexcept { return static_cast<int>(core_patch_.get_count_u()); }

    void set_count_v(int p_count_v);
    [[nodiscard]] int get_count_v() const noexcept { return static_cast<int>(core_patch_.get_count_v()); }

    void set_degree_u(int p_degree);
    [[nodiscard]] int get_degree_u() const noexcept { return static_cast<int>(core_patch_.get_degree_u()); }

    void set_degree_v(int p_degree);
    [[nodiscard]] int get_degree_v() const noexcept { return static_cast<int>(core_patch_.get_degree_v()); }

    void set_wrap_u(bool p_wrap);
    [[nodiscard]] bool get_wrap_u() const noexcept { return core_patch_.get_wrap_u(); }

    void set_wrap_v(bool p_wrap);
    [[nodiscard]] bool get_wrap_v() const noexcept { return core_patch_.get_wrap_v(); }

    void set_control_points_array(const godot::PackedVector3Array &p_points);
    [[nodiscard]] godot::PackedVector3Array get_control_points_array() const;

    void set_weights_array(const godot::PackedFloat32Array &p_weights);
    [[nodiscard]] godot::PackedFloat32Array get_weights_array() const;

    // --- Direct & Undoable Control-Point Mutators ---
    void set_control_point(int p_u, int p_v, const godot::Vector3 &p_pos, real_t p_weight = static_cast<real_t>(1.0));
    [[nodiscard]] godot::Vector3 get_control_point(int p_u, int p_v) const;
    void set_control_weight(int p_u, int p_v, real_t p_weight);
    [[nodiscard]] real_t get_control_weight(int p_u, int p_v) const;

    void commit_set_control_point(int p_u, int p_v, const godot::Vector3 &p_pos);
    void commit_apply_snapshot(const godot::Dictionary &p_before, const godot::String &p_action_name = "Edit NURBS Patch");

    // --- Spline3D Lofting ---
    void set_loft_ribs(const godot::TypedArray<Spline3DResource> &p_ribs);
    [[nodiscard]] godot::TypedArray<Spline3DResource> get_loft_ribs() const { return loft_ribs_; }

    void set_loft_samples_u(int p_samples);
    [[nodiscard]] int get_loft_samples_u() const noexcept { return loft_samples_u_; }

    void set_loft_arc_length(bool p_arc_length);
    [[nodiscard]] bool get_loft_arc_length() const noexcept { return loft_arc_length_; }

    void rebuild_from_loft_ribs();

    // --- Parametric Presets ---
    void setup_flat_grid(real_t p_width = 2.0f, real_t p_depth = 2.0f, int p_count_u = 4, int p_count_v = 4);
    void setup_cylinder_patch(real_t p_radius = 1.0f, real_t p_height = 2.0f, real_t p_arc_radians = 6.2831853f, int p_count_u = 9, int p_count_v = 4);
    void setup_dome_patch(real_t p_radius = 1.0f, real_t p_height = 1.0f, int p_count_u = 5, int p_count_v = 5);
};

} // namespace ideam::godot_ext
