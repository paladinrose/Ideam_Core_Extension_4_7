#pragma once

/**
 * nurbs_patch_3d.h
 * Tensor-product rational B-Spline / NURBS 3D surface patch (`ideam::core::NurbsPatch3D<T>`).
 *
 * Capabilities:
 *   - Arbitrary degrees `(degree_u, degree_v)` (default bicubic 3x3, automatically clamped to `count - 1`).
 *   - Rational control-point weights `weights[v * count_u + u]` enabling exact conic/cylindrical/spherical forms.
 *   - Clamped open-uniform or custom knot vectors along U and V, plus optional periodic seam wrapping (`wrap_u`, `wrap_v`).
 *   - Exact Cox–de Boor basis & derivative evaluation with rational quotient rule for analytic `dS/du`, `dS/dv`, and `sample_frame(u, v)`.
 *   - Direct lofting from an ordered span of `Spline3D<T>` profile ribs (`loft_from_splines`).
 *   - Watertight LOD-scalable quad-mesh tessellation (`tessellate_quad_mesh`).
 */

#include "surface_common.h"
#include "../splines/spline_3d.h"

#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

template <FloatingPointScalar T = float>
class NurbsPatch3D {
public:
    using Vec2  = glm::vec<2, T>;
    using Vec3  = glm::vec<3, T>;
    using Vec4  = glm::vec<4, T>;
    using Frame = SurfaceFrame3DPOD<T>;

private:
    uint32_t count_u_  = 0u;
    uint32_t count_v_  = 0u;
    uint32_t degree_u_ = 3u;
    uint32_t degree_v_ = 3u;
    bool wrap_u_       = false;
    bool wrap_v_       = false;

    // Row-major layout: index(u, v) = v * count_u_ + u
    std::vector<Vec3> control_points_;
    std::vector<T> weights_;

    std::vector<T> knots_u_;
    std::vector<T> knots_v_;
    bool custom_knots_u_ = false;
    bool custom_knots_v_ = false;

    void _rebuild_clamped_knots(std::vector<T> &r_knots, uint32_t p_count, uint32_t p_degree) const;
    [[nodiscard]] uint32_t _effective_degree_u() const noexcept;
    [[nodiscard]] uint32_t _effective_degree_v() const noexcept;

    [[nodiscard]] static uint32_t _find_knot_span(
        uint32_t p_n_ctrl,
        uint32_t p_degree,
        T p_param,
        std::span<const T> p_knots
    ) noexcept;

    /**
     * Evaluates non-zero B-spline basis functions N_{i,p}(u) and their first derivatives
     * dN_{i,p}(u)/du at `p_param` in knot span `p_span`.
     */
    static void _eval_basis_and_derivs(
        uint32_t p_span,
        T p_param,
        uint32_t p_degree,
        std::span<const T> p_knots,
        T *r_basis,
        T *r_deriv
    ) noexcept;

public:
    NurbsPatch3D() = default;
    ~NurbsPatch3D() = default;

    // ========================================================================
    // Grid Configuration & Control-Point Mutators
    // ========================================================================

    void resize_grid(uint32_t p_count_u, uint32_t p_count_v, uint32_t p_degree_u = 3u, uint32_t p_degree_v = 3u);
    void clear();

    [[nodiscard]] bool is_valid() const noexcept {
        return count_u_ >= 2u && count_v_ >= 2u && control_points_.size() == static_cast<size_t>(count_u_) * count_v_;
    }

    [[nodiscard]] uint32_t get_count_u() const noexcept { return count_u_; }
    [[nodiscard]] uint32_t get_count_v() const noexcept { return count_v_; }
    [[nodiscard]] uint32_t get_degree_u() const noexcept { return degree_u_; }
    [[nodiscard]] uint32_t get_degree_v() const noexcept { return degree_v_; }

    void set_degree_u(uint32_t p_degree);
    void set_degree_v(uint32_t p_degree);

    void set_wrap_u(bool p_wrap) noexcept { wrap_u_ = p_wrap; }
    [[nodiscard]] bool get_wrap_u() const noexcept { return wrap_u_; }

    void set_wrap_v(bool p_wrap) noexcept { wrap_v_ = p_wrap; }
    [[nodiscard]] bool get_wrap_v() const noexcept { return wrap_v_; }

    [[nodiscard]] inline size_t flat_index(uint32_t p_u, uint32_t p_v) const noexcept {
        return static_cast<size_t>(p_v) * count_u_ + p_u;
    }

    void set_control_point(uint32_t p_u, uint32_t p_v, const Vec3 &p_pos, T p_weight = static_cast<T>(1));
    [[nodiscard]] Vec3 get_control_point(uint32_t p_u, uint32_t p_v) const noexcept;
    void set_weight(uint32_t p_u, uint32_t p_v, T p_weight);
    [[nodiscard]] T get_weight(uint32_t p_u, uint32_t p_v) const noexcept;

    [[nodiscard]] std::span<const Vec3> get_control_points() const noexcept { return control_points_; }
    [[nodiscard]] std::span<const T> get_weights() const noexcept { return weights_; }
    [[nodiscard]] std::span<const T> get_knots_u() const noexcept { return knots_u_; }
    [[nodiscard]] std::span<const T> get_knots_v() const noexcept { return knots_v_; }

    void set_control_points_bulk(
        uint32_t p_count_u,
        uint32_t p_count_v,
        std::span<const Vec3> p_points,
        std::span<const T> p_weights = {}
    );

    void set_custom_knots_u(std::span<const T> p_knots);
    void set_custom_knots_v(std::span<const T> p_knots);
    void reset_uniform_knots();

    // ========================================================================
    // Lofting from Spline3D Curves & Parametric Presets
    // ========================================================================

    /**
     * Lofts a smooth NURBS patch through an ordered sequence of `Spline3D<T>` profile ribs.
     * Each rib is sampled at `p_samples_u` uniform points along U; the rib sequence spans V.
     */
    void loft_from_splines(
        std::span<const Spline3D<T>> p_ribs,
        uint32_t p_samples_u = 6u,
        uint32_t p_degree_u = 3u,
        uint32_t p_degree_v = 3u,
        bool p_arc_length_parametrize = true
    );

    void setup_flat_grid(
        T p_width = static_cast<T>(2.0),
        T p_depth = static_cast<T>(2.0),
        uint32_t p_count_u = 4u,
        uint32_t p_count_v = 4u
    );

    void setup_cylinder_patch(
        T p_radius = static_cast<T>(1.0),
        T p_height = static_cast<T>(2.0),
        T p_arc_radians = static_cast<T>(6.283185307179586),
        uint32_t p_count_u = 9u,
        uint32_t p_count_v = 4u
    );

    void setup_dome_patch(
        T p_radius = static_cast<T>(1.0),
        T p_height = static_cast<T>(1.0),
        uint32_t p_count_u = 5u,
        uint32_t p_count_v = 5u
    );

    // ========================================================================
    // Analytic Surface Evaluation, Boundary Queries & LOD Quad Tessellation
    // ========================================================================

    [[nodiscard]] Vec3 sample_position(T p_u, T p_v) const noexcept;
    [[nodiscard]] Frame sample_frame(T p_u, T p_v, uint32_t p_patch_id = 0u) const noexcept;

    /**
     * Samples `p_divs + 1` frames along one of the four boundary edges.
     */
    [[nodiscard]] std::vector<Frame> sample_boundary_edge(
        SurfaceBoundaryEdge p_edge,
        uint32_t p_divs,
        bool p_reverse = false,
        uint32_t p_patch_id = 0u
    ) const;

    /**
     * Tessellates this NURBS patch into an even, LOD-scalable quad mesh.
     */
    [[nodiscard]] SurfaceQuadMesh3D<T> tessellate_quad_mesh(
        uint32_t p_base_u_divs = 12u,
        uint32_t p_base_v_divs = 12u,
        uint8_t p_lod_level = 0u,
        uint32_t p_patch_id = 0u
    ) const;
};

} // namespace ideam::core
