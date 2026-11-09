#include "spline_2d.h"
#include <cmath>

namespace ideam::core {

// --- Polygon & Boundary Wrappers ---

template <typename T>
bool Spline2D<T>::contains_point(const VecType& p_point, int p_subdivisions) const {
    if (!this->is_wrap) return false;
    const std::vector<VecType> pts = this->get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::contains_point(pts, p_point);
}

template <typename T>
T Spline2D<T>::get_area(int p_subdivisions) const {
    if (!this->is_wrap) return static_cast<T>(0.0);
    const std::vector<VecType> pts = this->get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::area(pts);
}

template <typename T>
T Spline2D<T>::get_abs_area(int p_subdivisions) const {
    if (!this->is_wrap) return static_cast<T>(0.0);
    const std::vector<VecType> pts = this->get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::abs_area(pts);
}

template <typename T>
T Spline2D<T>::get_perimeter(int p_subdivisions) const {
    const std::vector<VecType> pts = this->get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::perimeter(pts, false);
}

template <typename T>
typename Spline2D<T>::VecType Spline2D<T>::get_centroid(int p_subdivisions) const {
    const std::vector<VecType> pts = this->get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::centroid(pts);
}

template <typename T>
AABB2D<T> Spline2D<T>::compute_aabb(int p_subdivisions) const {
    const std::vector<VecType> pts = this->get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::compute_aabb(pts);
}

template <typename T>
std::vector<uint32_t> Spline2D<T>::triangulate(int p_subdivisions) const {
    if (!this->is_wrap) return {};
    const std::vector<VecType> pts = this->get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::triangulate(pts);
}

template <typename T>
std::vector<typename Spline2D<T>::VecType> Spline2D<T>::clip_with_convex_polygon(
    std::span<const VecType> p_convex_clip,
    int p_subdivisions
) const {
    if (!this->is_wrap) return {};
    const std::vector<VecType> pts = this->get_smoothed_points(p_subdivisions);
    return CSGMath2D<T>::clip_polygon_convex(pts, p_convex_clip);
}

// --- 2D Spatial Transformations ---

template <typename T>
void Spline2D<T>::rotate_spline(T p_angle_rad) {
    if (this->points.empty()) return;

    VecType min_bounds, max_bounds;
    this->get_min_max(min_bounds, max_bounds, false);
    const VecType center = (min_bounds + max_bounds) * static_cast<T>(0.5);

    const T c = std::cos(p_angle_rad);
    const T s = std::sin(p_angle_rad);

    auto rot_vec = [c, s](const VecType& v) noexcept -> VecType {
        return VecType(v.x * c - v.y * s, v.x * s + v.y * c);
    };

    for (auto& pt : this->points) {
        pt.point = center + rot_vec(pt.point - center);
        if (pt.use_back_handle) {
            pt.back_handle = rot_vec(pt.back_handle);
        }
        if (pt.use_fore_handle) {
            pt.fore_handle = rot_vec(pt.fore_handle);
        }
    }

    this->set_cap_points();
    this->is_dirty = true;
}

template <typename T>
void Spline2D<T>::scale_spline(const VecType& p_scale) {
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

// --- Shape Generators (Delegated to PolygonMath2D<T>) ---

template <typename T>
Spline2D<T> Spline2D<T>::from_vertices(
    std::span<const VecType> p_vertices,
    bool p_wrap,
    CurveType p_type
) {
    Spline2D<T> spline;
    spline.set_default_curve_type(p_type);
    for (const auto& v : p_vertices) {
        spline.add_point(v, p_type);
    }
    spline.set_wrap(p_wrap);
    return spline;
}

template <typename T>
Spline2D<T> Spline2D<T>::create_rectangle(const VecType& p_center, const VecType& p_size) {
    const auto verts = PolygonMath2D<T>::generate_rectangle_vertices(p_center, p_size);
    return from_vertices(verts, true, CurveType::LINEAR);
}

template <typename T>
Spline2D<T> Spline2D<T>::create_regular_polygon(int p_sides, T p_radius, T p_angle_offset) {
    const auto verts = PolygonMath2D<T>::generate_regular_polygon_vertices(p_sides, p_radius, p_angle_offset, true);
    return from_vertices(verts, true, CurveType::LINEAR);
}

template <typename T>
Spline2D<T> Spline2D<T>::create_star(int p_sides, T p_outer_radius, T p_inner_radius, T p_angle_offset) {
    const auto verts = PolygonMath2D<T>::generate_star_vertices(p_sides, p_outer_radius, p_inner_radius, p_angle_offset);
    return from_vertices(verts, true, CurveType::LINEAR);
}

// Explicit Instantiations
template class Spline2D<float>;
template class Spline2D<double>;

} // namespace ideam::core