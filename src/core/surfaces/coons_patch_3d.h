#pragma once

/**
 * coons_patch_3d.h
 * Gordon / Coons bicubic transfinite interpolation 3D surface patch (`ideam::core::CoonsPatch3D<T>`).
 *
 * Capabilities:
 *   - Accepts 4 boundary `Spline3D<T>` curves (`north_v0`, `south_v1`, `west_u0`, `east_v1`)
 *     with automatic corner-compatibility averaging if boundary endpoints have slight drift.
 *   - Supports optional interior guide curves (`interior_u_guides` at `v in (0, 1)` and
 *     `interior_v_guides` at `u in (0, 1)`), seamlessly upgrading a classic Coons patch
 *     into a full **Gordon Curve-Network Surface** ($S = S_u + S_v - S_{uv}$).
 *   - Configurable blending basis: `LINEAR` (standard bilinear Coons) or `CUBIC_HERMITE`
 *     (smooth $3t^2 - 2t^3$ transfinite blend minimizing cross-boundary kinks).
 *   - Analytic / high-precision `(u, v)` frame queries (`sample_frame`) and LOD-scalable
 *     quad-mesh tessellation (`tessellate_quad_mesh`).
 */

#include "surface_common.h"
#include "../splines/spline_3d.h"

#include <cstdint>
#include <vector>

namespace ideam::core {

enum class CoonsBlendMode : uint8_t {
    LINEAR        = 0, ///< Linear transfinite blending functions (1 - t, t)
    CUBIC_HERMITE = 1  ///< Smooth cubic Hermite blending (2t^3 - 3t^2 + 1, 3t^2 - 2t^3)
};

template <FloatingPointScalar T = float>
struct GordonGuideCurve3D {
    T param = static_cast<T>(0.5); ///< Fixed parameter in (0, 1) (v for U-ribs, u for V-ribs)
    Spline3D<T> curve;
    bool reverse = false;
};

template <FloatingPointScalar T = float>
class CoonsPatch3D {
public:
    using Vec2  = glm::vec<2, T>;
    using Vec3  = glm::vec<3, T>;
    using Vec4  = glm::vec<4, T>;
    using Frame = SurfaceFrame3DPOD<T>;

private:
    Spline3D<T> north_v0_; ///< v = 0, u in [0, 1]
    Spline3D<T> south_v1_; ///< v = 1, u in [0, 1]
    Spline3D<T> west_u0_;  ///< u = 0, v in [0, 1]
    Spline3D<T> east_u1_;  ///< u = 1, v in [0, 1]

    bool reverse_north_ = false;
    bool reverse_south_ = false;
    bool reverse_west_  = false;
    bool reverse_east_  = false;

    std::vector<GordonGuideCurve3D<T>> interior_u_guides_; ///< Profile ribs running along U at fixed V
    std::vector<GordonGuideCurve3D<T>> interior_v_guides_; ///< Profile ribs running along V at fixed U

    CoonsBlendMode blend_mode_ = CoonsBlendMode::CUBIC_HERMITE;
    bool use_arc_length_       = true;

    [[nodiscard]] Vec3 _eval_curve(const Spline3D<T> &p_curve, T p_t, bool p_reverse) const;
    [[nodiscard]] T _blend_weight(T p_t) const noexcept;
    [[nodiscard]] static T _lagrange_basis(size_t p_idx, T p_t, std::span<const T> p_nodes) noexcept;

public:
    CoonsPatch3D() = default;
    ~CoonsPatch3D() = default;

    // ========================================================================
    // Boundary & Interior Guide Curve Configuration
    // ========================================================================

    void set_boundary_curves(
        const Spline3D<T> &p_north_v0,
        const Spline3D<T> &p_south_v1,
        const Spline3D<T> &p_west_u0,
        const Spline3D<T> &p_east_u1,
        bool p_rev_north = false,
        bool p_rev_south = false,
        bool p_rev_west  = false,
        bool p_rev_east  = false
    );

    void set_north_v0(const Spline3D<T> &p_curve, bool p_reverse = false) { north_v0_ = p_curve; reverse_north_ = p_reverse; }
    void set_south_v1(const Spline3D<T> &p_curve, bool p_reverse = false) { south_v1_ = p_curve; reverse_south_ = p_reverse; }
    void set_west_u0(const Spline3D<T> &p_curve, bool p_reverse = false)  { west_u0_  = p_curve; reverse_west_  = p_reverse; }
    void set_east_u1(const Spline3D<T> &p_curve, bool p_reverse = false)  { east_u1_  = p_curve; reverse_east_  = p_reverse; }

    [[nodiscard]] const Spline3D<T> &get_north_v0() const noexcept { return north_v0_; }
    [[nodiscard]] const Spline3D<T> &get_south_v1() const noexcept { return south_v1_; }
    [[nodiscard]] const Spline3D<T> &get_west_u0() const noexcept  { return west_u0_; }
    [[nodiscard]] const Spline3D<T> &get_east_u1() const noexcept  { return east_u1_; }

    void clear_interior_guides();
    void add_interior_u_guide(T p_v_param, const Spline3D<T> &p_curve, bool p_reverse = false);
    void add_interior_v_guide(T p_u_param, const Spline3D<T> &p_curve, bool p_reverse = false);

    [[nodiscard]] size_t get_interior_u_guide_count() const noexcept { return interior_u_guides_.size(); }
    [[nodiscard]] size_t get_interior_v_guide_count() const noexcept { return interior_v_guides_.size(); }

    void set_blend_mode(CoonsBlendMode p_mode) noexcept { blend_mode_ = p_mode; }
    [[nodiscard]] CoonsBlendMode get_blend_mode() const noexcept { return blend_mode_; }

    void set_use_arc_length(bool p_arc_length) noexcept { use_arc_length_ = p_arc_length; }
    [[nodiscard]] bool get_use_arc_length() const noexcept { return use_arc_length_; }

    [[nodiscard]] bool is_valid() const noexcept {
        return north_v0_.get_point_count() >= 2u &&
               south_v1_.get_point_count() >= 2u &&
               west_u0_.get_point_count()  >= 2u &&
               east_u1_.get_point_count()  >= 2u;
    }

    /**
     * Configures a default gently-domed Coons patch for quick testing / presets.
     */
    void setup_default_pillow_patch(T p_width = static_cast<T>(2.0), T p_depth = static_cast<T>(2.0), T p_arch_height = static_cast<T>(0.45));

    // ========================================================================
    // Surface Evaluation, Boundary Queries & LOD Quad Tessellation
    // ========================================================================

    [[nodiscard]] Vec3 sample_position(T p_u, T p_v) const;
    [[nodiscard]] Frame sample_frame(T p_u, T p_v, uint32_t p_patch_id = 0u) const;

    [[nodiscard]] std::vector<Frame> sample_boundary_edge(
        SurfaceBoundaryEdge p_edge,
        uint32_t p_divs,
        bool p_reverse = false,
        uint32_t p_patch_id = 0u
    ) const;

    [[nodiscard]] SurfaceQuadMesh3D<T> tessellate_quad_mesh(
        uint32_t p_base_u_divs = 12u,
        uint32_t p_base_v_divs = 12u,
        uint8_t p_lod_level = 0u,
        uint32_t p_patch_id = 0u
    ) const;
};

} // namespace ideam::core
