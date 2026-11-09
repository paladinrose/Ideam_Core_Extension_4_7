#include "parallel_transport_3d.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

template <FloatingPointScalar T>
typename ParallelTransport3D<T>::Frame ParallelTransport3D<T>::make_initial_frame(
    const Vec3& p_position,
    const Vec3& p_tangent,
    const Vec3& p_up_hint,
    T p_scale,
    T p_roll_rad
) noexcept {
    Frame f;
    f.position   = p_position;
    f.arc_length = static_cast<T>(0.0);
    f.scale      = p_scale;
    f.roll_rad   = static_cast<T>(0.0);

    const T t_len_sq = glm::dot(p_tangent, p_tangent);
    f.tangent = (t_len_sq > GeometryConstants<T>::SQ_EPSILON)
        ? (p_tangent * (static_cast<T>(1.0) / std::sqrt(t_len_sq)))
        : Vec3(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0));

    // Orthogonalize p_up_hint against tangent
    Vec3 n = p_up_hint - f.tangent * glm::dot(p_up_hint, f.tangent);
    const T n_len_sq = glm::dot(n, n);
    if (n_len_sq > GeometryConstants<T>::SQ_EPSILON) {
        f.normal = n * (static_cast<T>(1.0) / std::sqrt(n_len_sq));
        f.binormal = glm::normalize(glm::cross(f.tangent, f.normal));
    } else {
        GeometryKernelMath<T>::build_orthonormal_basis(f.tangent, f.normal, f.binormal);
    }

    if (std::abs(p_roll_rad) > GeometryConstants<T>::EPSILON) {
        f = apply_axial_roll(f, p_roll_rad);
    }
    return f;
}

template <FloatingPointScalar T>
typename ParallelTransport3D<T>::Frame ParallelTransport3D<T>::step_double_reflection_frame(
    const Frame& p_prev,
    const Vec3& p_next_pos,
    const Vec3& p_next_tangent,
    T p_next_scale,
    T p_next_roll_rad
) noexcept {
    Frame next = p_prev;
    const Vec3 v1 = p_next_pos - p_prev.position;
    const T c1 = glm::dot(v1, v1);

    next.position   = p_next_pos;
    next.arc_length = p_prev.arc_length + std::sqrt(c1);
    next.scale      = p_next_scale;

    const T t_len_sq = glm::dot(p_next_tangent, p_next_tangent);
    const Vec3 t1 = (t_len_sq > GeometryConstants<T>::SQ_EPSILON)
        ? (p_next_tangent * (static_cast<T>(1.0) / std::sqrt(t_len_sq)))
        : p_prev.tangent;

    if (c1 <= GeometryConstants<T>::SQ_EPSILON) {
        next.tangent = t1;
        return next;
    }

    // Wang et al. (2008): First reflection across plane perpendicular to chord v1 = x_{i+1} - x_i
    const T inv_c1 = static_cast<T>(2.0) / c1;
    const Vec3 r_l = p_prev.normal - v1 * (inv_c1 * glm::dot(v1, p_prev.normal));
    const Vec3 t_l = p_prev.tangent - v1 * (inv_c1 * glm::dot(v1, p_prev.tangent));

    // Second reflection across bisector v2 = t_{i+1} - t_l
    const Vec3 v2 = t1 - t_l;
    const T c2 = glm::dot(v2, v2);
    Vec3 n1 = r_l;
    if (c2 > GeometryConstants<T>::SQ_EPSILON) {
        n1 = r_l - v2 * ((static_cast<T>(2.0) / c2) * glm::dot(v2, r_l));
    }

    // Re-orthonormalize against numerical drift
    n1 = n1 - t1 * glm::dot(t1, n1);
    const T n1_len_sq = glm::dot(n1, n1);
    if (n1_len_sq > GeometryConstants<T>::SQ_EPSILON) {
        n1 *= (static_cast<T>(1.0) / std::sqrt(n1_len_sq));
    } else {
        n1 = p_prev.normal;
    }

    next.tangent  = t1;
    next.normal   = n1;
    next.binormal = glm::normalize(glm::cross(t1, n1));
    next.roll_rad = static_cast<T>(0.0);

    if (std::abs(p_next_roll_rad) > GeometryConstants<T>::EPSILON) {
        next = apply_axial_roll(next, p_next_roll_rad);
    }
    return next;
}

template <FloatingPointScalar T>
typename ParallelTransport3D<T>::Frame ParallelTransport3D<T>::apply_axial_roll(
    const Frame& p_frame,
    T p_angle_rad
) noexcept {
    Frame out = p_frame;
    const T c = std::cos(p_angle_rad);
    const T s = std::sin(p_angle_rad);
    out.normal   = p_frame.normal * c + p_frame.binormal * s;
    out.binormal = p_frame.binormal * c - p_frame.normal * s;
    out.roll_rad = p_frame.roll_rad + p_angle_rad;
    return out;
}

template <FloatingPointScalar T>
std::vector<typename ParallelTransport3D<T>::Frame> ParallelTransport3D<T>::compute_bishop_frames(
    std::span<const Vec3> p_positions,
    std::span<const Vec3> p_tangents,
    const Vec3& p_initial_up,
    bool p_closed_loop,
    std::span<const T> p_scales,
    std::span<const T> p_rolls_rad
) {
    std::vector<Frame> frames;
    const size_t n = p_positions.size();
    if (n == 0) return frames;

    frames.resize(n);

    // Resolve or estimate unit tangents
    std::vector<Vec3> tangents(n);
    if (p_tangents.size() == n) {
        for (size_t i = 0; i < n; ++i) {
            const T l2 = glm::dot(p_tangents[i], p_tangents[i]);
            tangents[i] = (l2 > GeometryConstants<T>::SQ_EPSILON)
                ? (p_tangents[i] * (static_cast<T>(1.0) / std::sqrt(l2)))
                : Vec3(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0));
        }
    } else if (n == 1) {
        tangents[0] = Vec3(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0));
    } else {
        for (size_t i = 0; i < n; ++i) {
            Vec3 diff;
            if (p_closed_loop) {
                const Vec3& prev = p_positions[(i + n - 1) % n];
                const Vec3& next = p_positions[(i + 1) % n];
                diff = next - prev;
            } else if (i == 0) {
                diff = p_positions[1] - p_positions[0];
            } else if (i == n - 1) {
                diff = p_positions[n - 1] - p_positions[n - 2];
            } else {
                diff = p_positions[i + 1] - p_positions[i - 1];
            }
            const T l2 = glm::dot(diff, diff);
            tangents[i] = (l2 > GeometryConstants<T>::SQ_EPSILON)
                ? (diff * (static_cast<T>(1.0) / std::sqrt(l2)))
                : Vec3(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0));
        }
    }

    const T init_scale = (!p_scales.empty()) ? p_scales[0] : static_cast<T>(1.0);
    frames[0] = make_initial_frame(p_positions[0], tangents[0], p_initial_up, init_scale, static_cast<T>(0.0));

    for (size_t i = 1; i < n; ++i) {
        const T s = (i < p_scales.size()) ? p_scales[i] : static_cast<T>(1.0);
        frames[i] = step_double_reflection_frame(frames[i - 1], p_positions[i], tangents[i], s, static_cast<T>(0.0));
    }

    // Closed-loop holonomy twist correction: distribute angular mismatch evenly by arc length
    if (p_closed_loop && n >= 3 && frames.back().arc_length > GeometryConstants<T>::EPSILON) {
        const Frame& first = frames.front();
        const Frame& last  = frames.back();
        const T cos_theta = std::clamp(glm::dot(last.normal, first.normal), static_cast<T>(-1.0), static_cast<T>(1.0));
        const T sin_theta = glm::dot(last.binormal, first.normal);
        const T holonomy_angle = std::atan2(sin_theta, cos_theta);

        const T total_len = last.arc_length;
        for (size_t i = 1; i < n; ++i) {
            const T u = frames[i].arc_length / total_len;
            frames[i] = apply_axial_roll(frames[i], -holonomy_angle * u);
        }
    }

    // Apply designer banking roll track after base RMF propagation
    if (!p_rolls_rad.empty()) {
        for (size_t i = 0; i < n && i < p_rolls_rad.size(); ++i) {
            if (std::abs(p_rolls_rad[i]) > GeometryConstants<T>::EPSILON) {
                frames[i] = apply_axial_roll(frames[i], p_rolls_rad[i]);
            }
        }
    }

    return frames;
}

template <FloatingPointScalar T>
std::vector<typename ParallelTransport3D<T>::Frame> ParallelTransport3D<T>::compute_bishop_frames_4d(
    std::span<const Vec4> p_points_4d,
    const Vec3& p_initial_up,
    bool p_closed_loop,
    bool p_w_is_roll
) {
    const size_t n = p_points_4d.size();
    std::vector<Vec3> positions(n);
    std::vector<T> w_track(n);

    for (size_t i = 0; i < n; ++i) {
        positions[i] = Vec3(p_points_4d[i].x, p_points_4d[i].y, p_points_4d[i].z);
        w_track[i]   = p_points_4d[i].w;
    }

    if (p_w_is_roll) {
        return compute_bishop_frames(positions, {}, p_initial_up, p_closed_loop, {}, w_track);
    }
    return compute_bishop_frames(positions, {}, p_initial_up, p_closed_loop, w_track, {});
}

template <FloatingPointScalar T>
ExtrudedMeshBuffers3D<T> ParallelTransport3D<T>::extrude_profile_along_frames(
    std::span<const Frame> p_frames,
    std::span<const Vec2> p_profile_2d,
    bool p_closed_profile,
    bool p_closed_path
) {
    ExtrudedMeshBuffers3D<T> mesh;
    if (p_frames.size() < 2 || p_profile_2d.size() < 2) {
        return mesh;
    }

    const size_t ring_count = p_frames.size();
    const size_t prof_count = p_profile_2d.size();
    const size_t total_verts = ring_count * prof_count;

    mesh.positions.resize(total_verts);
    mesh.normals.resize(total_verts);
    mesh.tangents.resize(total_verts);
    mesh.uvs.resize(total_verts);

    // Precompute profile perimeter U coordinates and 2D outward normals
    std::vector<T> prof_u(prof_count, static_cast<T>(0.0));
    std::vector<Vec2> prof_norm_2d(prof_count, Vec2(static_cast<T>(0.0), static_cast<T>(1.0)));

    T accum_u = static_cast<T>(0.0);
    for (size_t j = 1; j < prof_count; ++j) {
        accum_u += glm::distance(p_profile_2d[j], p_profile_2d[j - 1]);
        prof_u[j] = accum_u;
    }
    const T inv_prof_len = (accum_u > GeometryConstants<T>::EPSILON)
        ? (static_cast<T>(1.0) / accum_u)
        : static_cast<T>(1.0);

    for (size_t j = 0; j < prof_count; ++j) {
        Vec2 edge_dir;
        if (p_closed_profile) {
            const Vec2& prev = p_profile_2d[(j + prof_count - 1) % prof_count];
            const Vec2& next = p_profile_2d[(j + 1) % prof_count];
            edge_dir = next - prev;
        } else if (j == 0) {
            edge_dir = p_profile_2d[1] - p_profile_2d[0];
        } else if (j == prof_count - 1) {
            edge_dir = p_profile_2d[prof_count - 1] - p_profile_2d[prof_count - 2];
        } else {
            edge_dir = p_profile_2d[j + 1] - p_profile_2d[j - 1];
        }
        const Vec2 n2 = Vec2(edge_dir.y, -edge_dir.x);
        const T l2 = glm::dot(n2, n2);
        prof_norm_2d[j] = (l2 > GeometryConstants<T>::SQ_EPSILON)
            ? (n2 * (static_cast<T>(1.0) / std::sqrt(l2)))
            : Vec2(static_cast<T>(0.0), static_cast<T>(1.0));
    }

    const T total_path_len = std::max(p_frames.back().arc_length, GeometryConstants<T>::EPSILON);

    for (size_t i = 0; i < ring_count; ++i) {
        const Frame& f = p_frames[i];
        const T v_coord = f.arc_length / total_path_len;

        for (size_t j = 0; j < prof_count; ++j) {
            const size_t idx = i * prof_count + j;
            mesh.positions[idx] = f.transform_profile_point(p_profile_2d[j]);

            const Vec3 world_n = glm::normalize(f.normal * prof_norm_2d[j].x + f.binormal * prof_norm_2d[j].y);
            mesh.normals[idx]  = world_n;
            mesh.tangents[idx] = Vec4(f.tangent.x, f.tangent.y, f.tangent.z, static_cast<T>(1.0));
            mesh.uvs[idx]      = Vec2(prof_u[j] * inv_prof_len, v_coord);
        }
    }

    const size_t seg_rings = p_closed_path ? ring_count : (ring_count - 1);
    const size_t seg_prof  = p_closed_profile ? prof_count : (prof_count - 1);
    mesh.indices.reserve(seg_rings * seg_prof * 6);

    for (size_t i = 0; i < seg_rings; ++i) {
        const size_t next_i = (i + 1) % ring_count;
        for (size_t j = 0; j < seg_prof; ++j) {
            const size_t next_j = (j + 1) % prof_count;

            const uint32_t i00 = static_cast<uint32_t>(i * prof_count + j);
            const uint32_t i10 = static_cast<uint32_t>(next_i * prof_count + j);
            const uint32_t i11 = static_cast<uint32_t>(next_i * prof_count + next_j);
            const uint32_t i01 = static_cast<uint32_t>(i * prof_count + next_j);

            mesh.indices.push_back(i00);
            mesh.indices.push_back(i10);
            mesh.indices.push_back(i11);

            mesh.indices.push_back(i00);
            mesh.indices.push_back(i11);
            mesh.indices.push_back(i01);
        }
    }

    return mesh;
}

// Explicit Instantiations
template struct ParallelTransport3D<float>;
template struct ParallelTransport3D<double>;

} // namespace ideam::core