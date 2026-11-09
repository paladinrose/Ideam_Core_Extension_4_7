#include "spline_3d.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace ideam::core {

template <typename T>
std::vector<typename Spline3D<T>::FrameType> Spline3D<T>::compute_bishop_frames(
    int p_subdivisions_per_section,
    const VecType& p_initial_up
) const {
    if (this->points.empty()) return {};
    if (p_subdivisions_per_section < 2) p_subdivisions_per_section = 2;

    std::vector<VecType> sample_positions;
    std::vector<VecType> sample_tangents;

    if (this->sections.empty()) {
        sample_positions.push_back(this->points[0].point);
        sample_tangents.push_back(VecType(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)));
        return ParallelTransport3D<T>::compute_bishop_frames(sample_positions, sample_tangents, p_initial_up, false);
    }

    const T inv_sub = static_cast<T>(1.0) / static_cast<T>(p_subdivisions_per_section);
    sample_positions.reserve(this->sections.size() * static_cast<size_t>(p_subdivisions_per_section) + 1);
    sample_tangents.reserve(this->sections.size() * static_cast<size_t>(p_subdivisions_per_section) + 1);

    for (size_t s = 0; s < this->sections.size(); ++s) {
        const CurveType ctype = this->sections[s].curve_type;
        const int steps = (ctype == CurveType::CATMULL_ROM || ctype == CurveType::CUBIC_BEZIER)
            ? p_subdivisions_per_section
            : 1;
        const T step_dt = (steps == 1) ? static_cast<T>(1.0) : inv_sub;

        for (int j = 0; j < steps; ++j) {
            const T lt = static_cast<T>(j) * step_dt;
            sample_positions.push_back(this->evaluate_section(s, lt));
            sample_tangents.push_back(this->evaluate_section_derivative(s, lt));
        }
    }

    const size_t last_sec = this->sections.size() - 1;
    sample_positions.push_back(this->points.back().point);
    sample_tangents.push_back(this->evaluate_section_derivative(last_sec, static_cast<T>(1.0)));

    return ParallelTransport3D<T>::compute_bishop_frames(
        sample_positions,
        sample_tangents,
        p_initial_up,
        this->is_wrap
    );
}

template <typename T>
void Spline3D<T>::rotate_spline(const VecType& p_euler_angles) {
    if (this->points.empty()) return;

    VecType min_bounds, max_bounds;
    this->get_min_max(min_bounds, max_bounds, false);
    const VecType center = (min_bounds + max_bounds) * static_cast<T>(0.5);

    glm::tmat4x4<T, glm::defaultp> rot_only(static_cast<T>(1.0));
    rot_only = glm::rotate(rot_only, p_euler_angles.y, VecType(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0)));
    rot_only = glm::rotate(rot_only, p_euler_angles.x, VecType(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0)));
    rot_only = glm::rotate(rot_only, p_euler_angles.z, VecType(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)));

    glm::tmat4x4<T, glm::defaultp> transform(static_cast<T>(1.0));
    transform = glm::translate(transform, center) * rot_only * glm::translate(transform, -center);

    for (auto& pt : this->points) {
        glm::tvec4<T, glm::defaultp> p(pt.point.x, pt.point.y, pt.point.z, static_cast<T>(1.0));
        p = transform * p;
        pt.point = VecType(p.x, p.y, p.z);

        if (pt.use_back_handle) {
            glm::tvec4<T, glm::defaultp> bh(pt.back_handle.x, pt.back_handle.y, pt.back_handle.z, static_cast<T>(1.0));
            bh = rot_only * bh;
            pt.back_handle = VecType(bh.x, bh.y, bh.z);
        }
        if (pt.use_fore_handle) {
            glm::tvec4<T, glm::defaultp> fh(pt.fore_handle.x, pt.fore_handle.y, pt.fore_handle.z, static_cast<T>(1.0));
            fh = rot_only * fh;
            pt.fore_handle = VecType(fh.x, fh.y, fh.z);
        }
    }

    this->set_cap_points();
    this->is_dirty = true;
}

template <typename T>
void Spline3D<T>::scale_spline(const VecType& p_scale) {
    if (this->points.empty()) return;

    VecType min_bounds, max_bounds;
    this->get_min_max(min_bounds, max_bounds, false);
    const VecType center = (min_bounds + max_bounds) * static_cast<T>(0.5);

    for (auto& pt : this->points) {
        pt.point = ((pt.point - center) * p_scale) + center;

        if (pt.use_back_handle) {
            pt.back_handle *= p_scale;
        }
        if (pt.use_fore_handle) {
            pt.fore_handle *= p_scale;
        }
    }

    this->set_cap_points();
    this->is_dirty = true;
}

// Explicit Instantiations
template class Spline3D<float>;
template class Spline3D<double>;

} // namespace ideam::core