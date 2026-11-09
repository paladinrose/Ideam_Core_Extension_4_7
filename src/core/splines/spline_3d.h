#pragma once

#include "spline_base.h"
#include "../geometry/parallel_transport_3d.h"
#include <vector>

namespace ideam::core {

/**
 * Spline3D
 * Represents a curve in 3D space. Includes spatial transformation
 * utilities, hierarchical proximity lookups, and twist-free Bishop frame generation.
 */
template <typename T = float>
class Spline3D : public SplineBase<3, T> {
public:
    using VecType   = typename SplineBase<3, T>::VecType;
    using FrameType = CurveFrame3DPOD<T>;

    Spline3D() = default;
    ~Spline3D() override = default;

    // --- 3D Parallel Transport / Moving Frames (Backed by ParallelTransport3D<T>) ---

    /**
     * Generates a twist-free sequence of Rotation-Minimizing Bishop Frames along the 3D spline
     * using exact analytic section tangents (`evaluate_section_derivative`).
     */
    [[nodiscard]] std::vector<FrameType> compute_bishop_frames(
        int p_subdivisions_per_section = 10,
        const VecType& p_initial_up = VecType(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0))
    ) const;

    // --- Spatial Transformations ---

    /**
     * Rotates all control points and handles around the center of the spline's bounding box.
     * @param p_euler_angles Rotation in radians (X, Y, Z).
     */
    void rotate_spline(const VecType& p_euler_angles);

    /**
     * Scales all control points and handles outward/inward relative to the spline's bounding box center.
     */
    void scale_spline(const VecType& p_scale);
};

} // namespace ideam::core