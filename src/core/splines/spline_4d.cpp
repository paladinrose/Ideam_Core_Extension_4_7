#include "spline_4d.h"
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/color_space.hpp>

#include <cmath>

namespace ideam::core {

template <typename T>
std::vector<typename Spline4D<T>::FrameType> Spline4D<T>::compute_bishop_frames(
    int p_subdivisions_per_section,
    const Vec3Type& p_initial_up,
    bool p_w_is_roll
) const {
    const std::vector<VecType> pts_4d = this->get_smoothed_points(p_subdivisions_per_section);
    return ParallelTransport3D<T>::compute_bishop_frames_4d(pts_4d, p_initial_up, this->is_wrap, p_w_is_roll);
}

template <typename T>
void Spline4D<T>::rotate_spline(const Vec3Type& p_euler_angles) {
    if (this->points.empty()) return;

    VecType min_bounds, max_bounds;
    this->get_min_max(min_bounds, max_bounds, false);

    // Rotation pivots on the 3D (XYZ) components while preserving W
    const Vec3Type center(
        (min_bounds.x + max_bounds.x) * static_cast<T>(0.5),
        (min_bounds.y + max_bounds.y) * static_cast<T>(0.5),
        (min_bounds.z + max_bounds.z) * static_cast<T>(0.5)
    );

    glm::tmat4x4<T, glm::defaultp> rot_only(static_cast<T>(1.0));
    rot_only = glm::rotate(rot_only, p_euler_angles.y, Vec3Type(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0)));
    rot_only = glm::rotate(rot_only, p_euler_angles.x, Vec3Type(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0)));
    rot_only = glm::rotate(rot_only, p_euler_angles.z, Vec3Type(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)));

    glm::tmat4x4<T, glm::defaultp> transform(static_cast<T>(1.0));
    transform = glm::translate(transform, center) * rot_only * glm::translate(transform, -center);

    for (auto& pt : this->points) {
        glm::tvec4<T, glm::defaultp> p(pt.point.x, pt.point.y, pt.point.z, static_cast<T>(1.0));
        p = transform * p;
        pt.point = VecType(p.x, p.y, p.z, pt.point.w);

        if (pt.use_back_handle) {
            glm::tvec4<T, glm::defaultp> bh(pt.back_handle.x, pt.back_handle.y, pt.back_handle.z, static_cast<T>(1.0));
            bh = rot_only * bh;
            pt.back_handle = VecType(bh.x, bh.y, bh.z, pt.back_handle.w);
        }
        if (pt.use_fore_handle) {
            glm::tvec4<T, glm::defaultp> fh(pt.fore_handle.x, pt.fore_handle.y, pt.fore_handle.z, static_cast<T>(1.0));
            fh = rot_only * fh;
            pt.fore_handle = VecType(fh.x, fh.y, fh.z, pt.fore_handle.w);
        }
    }

    this->set_cap_points();
    this->is_dirty = true;
}

template <typename T>
void Spline4D<T>::scale_spline(const Vec3Type& p_scale) {
    if (this->points.empty()) return;

    VecType min_bounds, max_bounds;
    this->get_min_max(min_bounds, max_bounds, false);

    const Vec3Type center(
        (min_bounds.x + max_bounds.x) * static_cast<T>(0.5),
        (min_bounds.y + max_bounds.y) * static_cast<T>(0.5),
        (min_bounds.z + max_bounds.z) * static_cast<T>(0.5)
    );

    for (auto& pt : this->points) {
        Vec3Type xyz(pt.point.x, pt.point.y, pt.point.z);
        xyz = ((xyz - center) * p_scale) + center;
        pt.point = VecType(xyz.x, xyz.y, xyz.z, pt.point.w);

        if (pt.use_back_handle) {
            Vec3Type bh(pt.back_handle.x, pt.back_handle.y, pt.back_handle.z);
            bh *= p_scale;
            pt.back_handle = VecType(bh.x, bh.y, bh.z, pt.back_handle.w);
        }
        if (pt.use_fore_handle) {
            Vec3Type fh(pt.fore_handle.x, pt.fore_handle.y, pt.fore_handle.z);
            fh *= p_scale;
            pt.fore_handle = VecType(fh.x, fh.y, fh.z, pt.fore_handle.w);
        }
    }

    this->set_cap_points();
    this->is_dirty = true;
}

template <typename T>
Spline4D<T> Spline4D<T>::create_gradient(std::span<const VecType> p_colors) {
    Spline4D<T> grad;
    grad.default_curve_type = CurveType::CATMULL_ROM;

    if (p_colors.size() < 2) {
        const Vec3Type hsv_red   = glm::hsvColor(Vec3Type(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0)));
        const Vec3Type hsv_green = glm::hsvColor(Vec3Type(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0)));

        grad.add_point(VecType(hsv_red.x, hsv_red.y, hsv_red.z, static_cast<T>(1.0)), CurveType::CATMULL_ROM);
        grad.add_point(VecType(hsv_green.x, hsv_green.y, hsv_green.z, static_cast<T>(1.0)), CurveType::CATMULL_ROM);
        return grad;
    }

    for (const auto& c : p_colors) {
        const Vec3Type hsv = glm::hsvColor(Vec3Type(c.x, c.y, c.z));
        grad.add_point(VecType(hsv.x, hsv.y, hsv.z, c.w), CurveType::CATMULL_ROM);
    }

    return grad;
}

// Explicit Instantiations
template class Spline4D<float>;
template class Spline4D<double>;

} // namespace ideam::core