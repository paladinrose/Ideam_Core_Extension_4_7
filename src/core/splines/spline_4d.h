#pragma once

#include "spline_base.h"
#include "../geometry/parallel_transport_3d.h"
#include <span>
#include <vector>

namespace ideam::core {

/**
 * Spline4D
 * Represents a curve in 4D space.
 * Often used for animated 3D transforms with a weight/scale/roll track (w),
 * or traversing perceptual color gradients (RGBA / HSVA).
 */
template <typename T = float>
class Spline4D : public SplineBase<4, T> {
public:
    using VecType   = typename SplineBase<4, T>::VecType;
    using Vec3Type  = glm::vec<3, T>;
    using FrameType = CurveFrame3DPOD<T>;

    Spline4D() = default;
    ~Spline4D() override = default;

    // --- 4D Parallel Transport / Moving Frames (XYZ + W Scale or Roll) ---

    /**
     * Generates a twist-free sequence of 3D Rotation-Minimizing Bishop Frames along the XYZ
     * spatial track of the 4D spline, applying the W channel as either cross-section scale
     * (`p_w_is_roll == false`) or axial banking roll in radians (`p_w_is_roll == true`).
     */
    [[nodiscard]] std::vector<FrameType> compute_bishop_frames(
        int p_subdivisions_per_section = 10,
        const Vec3Type& p_initial_up = Vec3Type(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)),
        bool p_w_is_roll = false
    ) const;

    // --- Semi-Spatial Transformations (XYZ transformed, W preserved) ---

    /**
     * Rotates the 3D components (X, Y, Z) of all control points and handles around the 3D center
     * of the spline's bounding box while preserving W.
     * @param p_euler_angles Rotation in radians (X, Y, Z).
     */
    void rotate_spline(const Vec3Type& p_euler_angles);

    /**
     * Scales the 3D components (X, Y, Z) of all control points and handles outward/inward relative
     * to the 3D bounding box center while preserving W.
     */
    void scale_spline(const Vec3Type& p_scale);

    // --- 4D Generators ---

    /**
     * Takes an array of RGB(A) colors, converts them to HSV(A), and builds
     * a smoothly interpolated 4D Catmull-Rom spline.
     */
    [[nodiscard]] static Spline4D<T> create_gradient(std::span<const VecType> p_colors);
};

} // namespace ideam::core