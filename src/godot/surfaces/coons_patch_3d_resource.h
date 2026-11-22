#pragma once

/**
 * coons_patch_3d_resource.h
 * Gordon / Coons Transfinite Interpolation 3D Surface Patch Resource (`src/godot/surfaces/`).
 *
 * Wraps `ideam::core::CoonsPatch3D<real_t>` on top of `Spline3DResource`:
 *   - 4 live boundary `Spline3DResource` curves (`north_v0`, `south_v1`, `west_u0`, `east_u1`)
 *     with per-edge orientation flags (`reverse_*`).
 *   - Optional interior Gordon guide rib curves (`interior_u_guides`, `interior_v_guides` with
 *     corresponding parameter arrays `interior_u_params`, `interior_v_params`).
 *   - Automatically reconnects to every boundary and guide spline's `changed` signal so editing
 *     any `Spline3DResource` in the viewport immediately updates the surface!
 */

#include "surface_patch_resource.h"
#include "../../core/surfaces/coons_patch_3d.h"
#include "../splines/spline_3d/spline_3d_resource.h"

#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

enum CoonsPatchBlendMode : int32_t {
    COONS_BLEND_LINEAR        = 0,
    COONS_BLEND_CUBIC_HERMITE = 1
};

class CoonsPatch3DResource : public SurfacePatchResource {
    GDCLASS(CoonsPatch3DResource, SurfacePatchResource)

private:
    using CorePatch = core::CoonsPatch3D<real_t>;

    mutable CorePatch core_patch_;
    mutable bool core_dirty_ = true;

    godot::Ref<Spline3DResource> north_v0_;
    godot::Ref<Spline3DResource> south_v1_;
    godot::Ref<Spline3DResource> west_u0_;
    godot::Ref<Spline3DResource> east_u1_;

    bool reverse_north_ = false;
    bool reverse_south_ = false;
    bool reverse_west_  = false;
    bool reverse_east_  = false;

    godot::TypedArray<Spline3DResource> interior_u_guides_;
    godot::PackedFloat32Array interior_u_params_;
    godot::TypedArray<Spline3DResource> interior_v_guides_;
    godot::PackedFloat32Array interior_v_params_;

    CoonsPatchBlendMode blend_mode_ = COONS_BLEND_CUBIC_HERMITE;
    bool use_arc_length_ = true;

    void _sync_core() const;
    void _connect_spline(const godot::Ref<Spline3DResource> &p_spline);
    void _disconnect_spline(const godot::Ref<Spline3DResource> &p_spline);
    void _on_spline_changed();

protected:
    static void _bind_methods();

public:
    CoonsPatch3DResource();
    ~CoonsPatch3DResource() override;

    [[nodiscard]] const CorePatch &get_core_patch() const;

    // --- SurfacePatchResource Overrides ---
    [[nodiscard]] bool is_patch_valid() const override;
    [[nodiscard]] godot::String get_patch_type_label() const override {
        return (interior_u_guides_.is_empty() && interior_v_guides_.is_empty()) ? "Coons Patch" : "Gordon Patch";
    }

    [[nodiscard]] core::SurfaceFrame3DPOD<real_t> sample_core_frame(
        real_t p_u,
        real_t p_v,
        uint32_t p_patch_id = 0u
    ) const override;

    [[nodiscard]] core::SurfaceQuadMesh3D<real_t> tessellate_core_quad_mesh(
        int p_lod_level = 0,
        uint32_t p_patch_id = 0u
    ) const override;

    // --- Boundary Spline3D Properties ---
    void set_north_v0(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_north_v0() const { return north_v0_; }

    void set_south_v1(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_south_v1() const { return south_v1_; }

    void set_west_u0(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_west_u0() const { return west_u0_; }

    void set_east_u1(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_east_u1() const { return east_u1_; }

    void set_reverse_north(bool p_rev);
    [[nodiscard]] bool get_reverse_north() const noexcept { return reverse_north_; }

    void set_reverse_south(bool p_rev);
    [[nodiscard]] bool get_reverse_south() const noexcept { return reverse_south_; }

    void set_reverse_west(bool p_rev);
    [[nodiscard]] bool get_reverse_west() const noexcept { return reverse_west_; }

    void set_reverse_east(bool p_rev);
    [[nodiscard]] bool get_reverse_east() const noexcept { return reverse_east_; }

    // --- Interior Gordon Guide Ribs ---
    void set_interior_u_guides(const godot::TypedArray<Spline3DResource> &p_guides);
    [[nodiscard]] godot::TypedArray<Spline3DResource> get_interior_u_guides() const { return interior_u_guides_; }

    void set_interior_u_params(const godot::PackedFloat32Array &p_params);
    [[nodiscard]] godot::PackedFloat32Array get_interior_u_params() const { return interior_u_params_; }

    void set_interior_v_guides(const godot::TypedArray<Spline3DResource> &p_guides);
    [[nodiscard]] godot::TypedArray<Spline3DResource> get_interior_v_guides() const { return interior_v_guides_; }

    void set_interior_v_params(const godot::PackedFloat32Array &p_params);
    [[nodiscard]] godot::PackedFloat32Array get_interior_v_params() const { return interior_v_params_; }

    void set_blend_mode(CoonsPatchBlendMode p_mode);
    [[nodiscard]] CoonsPatchBlendMode get_blend_mode() const noexcept { return blend_mode_; }

    void set_use_arc_length(bool p_arc_length);
    [[nodiscard]] bool get_use_arc_length() const noexcept { return use_arc_length_; }

    // --- Preset ---
    void setup_default_pillow_patch(real_t p_width = 2.0f, real_t p_depth = 2.0f, real_t p_arch_height = 0.45f);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::CoonsPatchBlendMode);
