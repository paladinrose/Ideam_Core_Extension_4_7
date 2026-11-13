#include "tensor_field_2d.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::core {

namespace {

[[nodiscard]] inline glm::vec2 align_vector_sign(const glm::vec2 &p_vec, const glm::vec2 &p_ref) noexcept {
    if (glm::dot(p_ref, p_ref) <= 1e-12f) return p_vec;
    return (glm::dot(p_vec, p_ref) >= 0.0f) ? p_vec : -p_vec;
}

/**
 * Wraps a double-angle difference `p_delta_2theta` into `(-pi, pi]` so dividing by 2
 * yields the exact principal-axis rotation in `(-pi/2, pi/2]`.
 */
[[nodiscard]] inline float wrap_angle_pi(float p_angle) noexcept {
    constexpr float PI  = std::numbers::pi_v<float>;
    constexpr float TAU = 2.0f * PI;
    while (p_angle <= -PI) p_angle += TAU;
    while (p_angle >   PI) p_angle -= TAU;
    return p_angle;
}

/**
 * Point-to-segment squared distance and closest tangent helper.
 */
inline float closest_segment_tangent_and_dist_sq(
    const glm::vec2 &p_pt,
    const glm::vec2 &p_a,
    const glm::vec2 &p_b,
    glm::vec2 &r_tangent
) noexcept {
    const glm::vec2 ab = p_b - p_a;
    const float ab_len_sq = glm::dot(ab, ab);
    if (ab_len_sq <= 1e-12f) {
        r_tangent = glm::vec2(1.0f, 0.0f);
        const glm::vec2 d = p_pt - p_a;
        return glm::dot(d, d);
    }
    const float t = std::clamp(glm::dot(p_pt - p_a, ab) / ab_len_sq, 0.0f, 1.0f);
    const glm::vec2 closest = p_a + ab * t;
    const glm::vec2 diff = p_pt - closest;
    r_tangent = ab * (1.0f / std::sqrt(ab_len_sq));
    return glm::dot(diff, diff);
}

/**
 * Segment-segment intersection in 2D (`p0->p1` vs `q0->q1`).
 */
inline bool intersect_segments_2d(
    const glm::vec2 &p_p0,
    const glm::vec2 &p_p1,
    const glm::vec2 &p_q0,
    const glm::vec2 &p_q1,
    glm::vec2 &r_hit_pos,
    float &r_t,
    float &r_u
) noexcept {
    const glm::vec2 r = p_p1 - p_p0;
    const glm::vec2 s = p_q1 - p_q0;
    const float rxs   = GeometryKernelMath<float>::cross_2d(r, s);
    if (std::abs(rxs) <= 1e-6f) return false;

    const glm::vec2 qp = p_q0 - p_p0;
    const float inv_rxs = 1.0f / rxs;
    const float t = GeometryKernelMath<float>::cross_2d(qp, s) * inv_rxs;
    const float u = GeometryKernelMath<float>::cross_2d(qp, r) * inv_rxs;

    if (t >= -1e-4f && t <= 1.0001f && u >= -1e-4f && u <= 1.0001f) {
        r_t       = std::clamp(t, 0.0f, 1.0f);
        r_u       = std::clamp(u, 0.0f, 1.0f);
        r_hit_pos = p_p0 + r * r_t;
        return true;
    }
    return false;
}

} // anonymous namespace

// ============================================================================
// Basis & Composite Field Evaluation
// ============================================================================

SymmetricTensor2DPod TensorFieldMath2D::evaluate_single_basis(
    const glm::vec2 &p_world_pos,
    const TensorBasisElementPod &p_basis,
    std::span<const PolylineSlicePOD> p_polylines,
    std::span<const glm::vec2> p_polyline_verts,
    const SDFGridHeaderPOD *p_sdf_header,
    std::span<const float> p_sdf_cells
) noexcept {
    if (p_basis.enabled == 0u || std::abs(p_basis.weight) <= 1e-7f) {
        return SymmetricTensor2DPod{0.0f, 0.0f, 0.0f, 0.0f};
    }

    const glm::vec2 delta = p_world_pos - p_basis.position;
    const float dist_sq   = glm::dot(delta, delta);
    const float env       = p_basis.weight * evaluate_distance_decay(dist_sq, p_basis.decay);
    if (std::abs(env) <= 1e-7f) {
        return SymmetricTensor2DPod{0.0f, 0.0f, 0.0f, 0.0f};
    }

    switch (static_cast<TensorBasisType>(p_basis.basis_type)) {
        case TENSOR_BASIS_GRIDLINE: {
            return SymmetricTensor2DPod::from_tangent(p_basis.direction, env, p_basis.aux_param * env, env);
        }

        case TENSOR_BASIS_RADIAL: {
            return SymmetricTensor2DPod::from_radial_offset(delta, env, p_basis.aux_param * env, env);
        }

        case TENSOR_BASIS_VORTEX: {
            if (dist_sq <= 1e-12f) {
                return SymmetricTensor2DPod{0.0f, 0.0f, 0.0f, 0.0f};
            }
            const float c = std::cos(p_basis.aux_param);
            const float s = std::sin(p_basis.aux_param);
            const glm::vec2 rotated(delta.x * c - delta.y * s, delta.x * s + delta.y * c);
            return SymmetricTensor2DPod::from_tangent(rotated, env, 0.0f, env);
        }

        case TENSOR_BASIS_POLYLINE_ATTRACTOR: {
            if (p_basis.source_slice_id < p_polylines.size()) {
                const PolylineSlicePOD &slice = p_polylines[p_basis.source_slice_id];
                if (slice.vertex_count >= 2u &&
                    slice.vertex_offset + slice.vertex_count <= p_polyline_verts.size()) {
                    SymmetricTensor2DPod accum{0.0f, 0.0f, 0.0f, 0.0f};
                    const size_t seg_count = (slice.is_closed != 0u)
                        ? slice.vertex_count
                        : (slice.vertex_count - 1u);
                    for (size_t i = 0; i < seg_count; ++i) {
                        const glm::vec2 a = p_polyline_verts[slice.vertex_offset + i];
                        const glm::vec2 b = p_polyline_verts[slice.vertex_offset + ((i + 1u) % slice.vertex_count)];
                        glm::vec2 tang(1.0f, 0.0f);
                        const float seg_d2 = closest_segment_tangent_and_dist_sq(p_world_pos, a, b, tang);
                        const float seg_w  = p_basis.weight * evaluate_distance_decay(seg_d2, p_basis.decay);
                        accum += SymmetricTensor2DPod::from_tangent(tang, seg_w, 0.0f, seg_w);
                    }
                    return accum;
                }
            }
            return SymmetricTensor2DPod::from_tangent(p_basis.direction, env, 0.0f, env);
        }

        case TENSOR_BASIS_SDF_GRADIENT: {
            if (p_sdf_header != nullptr &&
                p_sdf_header->width >= 2u &&
                p_sdf_header->height >= 2u &&
                p_sdf_header->cell_offset + p_sdf_header->width * p_sdf_header->height <= p_sdf_cells.size()) {
                const glm::vec2 rel = (p_world_pos - p_sdf_header->origin) / p_sdf_header->cell_size;
                const int32_t ix = std::clamp(static_cast<int32_t>(rel.x), 1, static_cast<int32_t>(p_sdf_header->width) - 2);
                const int32_t iy = std::clamp(static_cast<int32_t>(rel.y), 1, static_cast<int32_t>(p_sdf_header->height) - 2);
                const uint32_t w = p_sdf_header->width;
                const float *base = p_sdf_cells.data() + p_sdf_header->cell_offset;
                const float gx = base[iy * w + (ix + 1)] - base[iy * w + (ix - 1)];
                const float gy = base[(iy + 1) * w + ix] - base[(iy - 1) * w + ix];
                // Major eigenvector aligns along the iso-contour tangent (-gy, gx), matching HeightmapTensorField.cs!
                const glm::vec2 contour_tangent(-gy, gx);
                return SymmetricTensor2DPod::from_tangent(contour_tangent, env, 0.0f, env);
            }
            return SymmetricTensor2DPod::from_tangent(p_basis.direction, env, 0.0f, env);
        }

        case TENSOR_BASIS_WEDGE_SINGULARITY: {
            // In a Wedge (+1/2 index), 2*theta_field = +phi => (a, b) = env * (dx, dy) / |delta|
            if (dist_sq <= 1e-12f) return SymmetricTensor2DPod{0.0f, 0.0f, 0.0f, 0.0f};
            const float inv_r = env / std::sqrt(dist_sq);
            return SymmetricTensor2DPod{delta.x * inv_r, delta.y * inv_r, 0.0f, env};
        }

        case TENSOR_BASIS_TRISECTOR_SINGULARITY: {
            // In a Trisector (-1/2 index), 2*theta_field = -phi => (a, b) = env * (dx, -dy) / |delta|
            if (dist_sq <= 1e-12f) return SymmetricTensor2DPod{0.0f, 0.0f, 0.0f, 0.0f};
            const float inv_r = env / std::sqrt(dist_sq);
            return SymmetricTensor2DPod{delta.x * inv_r, -delta.y * inv_r, 0.0f, env};
        }

        default:
            return SymmetricTensor2DPod{0.0f, 0.0f, 0.0f, 0.0f};
    }
}

SymmetricTensor2DPod TensorFieldMath2D::sample_tensor_field(
    const glm::vec2 &p_world_pos,
    std::span<const TensorBasisElementPod> p_bases,
    std::span<const PolylineSlicePOD> p_polylines,
    std::span<const glm::vec2> p_polyline_verts,
    const SDFGridHeaderPOD *p_sdf_header,
    std::span<const float> p_sdf_cells
) noexcept {
    SymmetricTensor2DPod total{0.0f, 0.0f, 0.0f, 0.0f};
    for (const TensorBasisElementPod &basis : p_bases) {
        total += evaluate_single_basis(
            p_world_pos, basis, p_polylines, p_polyline_verts, p_sdf_header, p_sdf_cells
        );
    }
    return total;
}

void TensorFieldMath2D::evaluate_tensor_grid(
    const TensorGridHeaderPod &p_header,
    std::span<const TensorBasisElementPod> p_bases,
    std::span<SymmetricTensor2DPod> r_out_tensors,
    std::span<glm::vec2> r_out_major_eigen,
    std::span<glm::vec2> r_out_minor_eigen,
    std::span<const PolylineSlicePOD> p_polylines,
    std::span<const glm::vec2> p_polyline_verts,
    const SDFGridHeaderPOD *p_sdf_header,
    std::span<const float> p_sdf_cells
) noexcept {
    const uint32_t w = p_header.width;
    const uint32_t h = p_header.height;
    const uint32_t total_cells = w * h;
    if (total_cells == 0u) return;

    const size_t base_start = std::min<size_t>(p_header.basis_offset, p_bases.size());
    const size_t base_count = (p_header.basis_count > 0u)
        ? std::min<size_t>(p_header.basis_count, p_bases.size() - base_start)
        : (p_bases.size() - base_start);
    const std::span<const TensorBasisElementPod> active_bases = p_bases.subspan(base_start, base_count);

    for (uint32_t y = 0; y < h; ++y) {
        const float wy = p_header.origin.y + (static_cast<float>(y) + 0.5f) * p_header.cell_size.y;
        for (uint32_t x = 0; x < w; ++x) {
            const uint32_t cell_idx = p_header.cell_offset + y * w + x;
            const float wx = p_header.origin.x + (static_cast<float>(x) + 0.5f) * p_header.cell_size.x;
            const glm::vec2 pos(wx, wy);

            const SymmetricTensor2DPod t = sample_tensor_field(
                pos, active_bases, p_polylines, p_polyline_verts, p_sdf_header, p_sdf_cells
            );
            if (cell_idx < r_out_tensors.size()) {
                r_out_tensors[cell_idx] = t;
            }
            if (cell_idx < r_out_major_eigen.size() && cell_idx < r_out_minor_eigen.size()) {
                glm::vec2 maj, min_v;
                t.eigenvectors(maj, min_v);
                r_out_major_eigen[cell_idx] = maj;
                r_out_minor_eigen[cell_idx] = min_v;
            }
        }
    }
}

glm::vec2 TensorFieldMath2D::sample_grid_eigenvector_aligned(
    const glm::vec2 &p_world_pos,
    const glm::vec2 &p_prev_dir,
    bool p_follow_minor,
    const TensorGridHeaderPod &p_header,
    std::span<const SymmetricTensor2DPod> p_grid_tensors
) noexcept {
    if (p_header.width < 2u || p_header.height < 2u ||
        std::abs(p_header.cell_size.x) <= 1e-6f ||
        std::abs(p_header.cell_size.y) <= 1e-6f) {
        return glm::vec2(0.0f);
    }
    const uint32_t w = p_header.width;
    const uint32_t h = p_header.height;
    if (p_header.cell_offset + w * h > p_grid_tensors.size()) {
        return glm::vec2(0.0f);
    }

    const float gx = std::clamp(
        (p_world_pos.x - p_header.origin.x) / p_header.cell_size.x - 0.5f,
        0.0f,
        static_cast<float>(w - 1u)
    );
    const float gy = std::clamp(
        (p_world_pos.y - p_header.origin.y) / p_header.cell_size.y - 0.5f,
        0.0f,
        static_cast<float>(h - 1u)
    );

    const uint32_t x0 = static_cast<uint32_t>(gx);
    const uint32_t y0 = static_cast<uint32_t>(gy);
    const uint32_t x1 = std::min(x0 + 1u, w - 1u);
    const uint32_t y1 = std::min(y0 + 1u, h - 1u);
    const float tx    = gx - static_cast<float>(x0);
    const float ty    = gy - static_cast<float>(y0);

    // Bilinearly interpolate the tensor components `(a, b)` BEFORE extracting eigenvectors!
    // Interpolating `(a, b)` in tensor space is 100% immune to the 180-degree eigenvector sign flip!
    const SymmetricTensor2DPod *base = p_grid_tensors.data() + p_header.cell_offset;
    const SymmetricTensor2DPod &t00  = base[y0 * w + x0];
    const SymmetricTensor2DPod &t10  = base[y0 * w + x1];
    const SymmetricTensor2DPod &t01  = base[y1 * w + x0];
    const SymmetricTensor2DPod &t11  = base[y1 * w + x1];

    const SymmetricTensor2DPod top    = (1.0f - tx) * t00 + tx * t10;
    const SymmetricTensor2DPod bottom = (1.0f - tx) * t01 + tx * t11;
    const SymmetricTensor2DPod interp = (1.0f - ty) * top + ty * bottom;

    if (interp.is_degenerate(1e-5f)) {
        return glm::vec2(0.0f);
    }

    glm::vec2 maj, min_v;
    interp.eigenvectors(maj, min_v);
    const glm::vec2 chosen = p_follow_minor ? min_v : maj;
    return align_vector_sign(chosen, p_prev_dir);
}

glm::vec2 TensorFieldMath2D::rk4_step_eigenvector_field(
    const glm::vec2 &p_pos,
    const glm::vec2 &p_prev_dir,
    float p_step_size,
    bool p_follow_minor,
    const TensorGridHeaderPod &p_header,
    std::span<const SymmetricTensor2DPod> p_grid_tensors,
    std::span<const TensorBasisElementPod> p_fallback_bases
) noexcept {
    auto eval_dir = [&](const glm::vec2 &pt, const glm::vec2 &ref_dir) noexcept -> glm::vec2 {
        if (!p_grid_tensors.empty() && p_header.width >= 2u && p_header.height >= 2u) {
            return sample_grid_eigenvector_aligned(pt, ref_dir, p_follow_minor, p_header, p_grid_tensors);
        }
        const SymmetricTensor2DPod t = sample_tensor_field(pt, p_fallback_bases);
        if (t.is_degenerate(1e-5f)) return glm::vec2(0.0f);
        glm::vec2 maj, min_v;
        t.eigenvectors(maj, min_v);
        return align_vector_sign(p_follow_minor ? min_v : maj, ref_dir);
    };

    const glm::vec2 k1 = eval_dir(p_pos, p_prev_dir);
    if (glm::dot(k1, k1) <= 1e-10f) return glm::vec2(0.0f);

    const glm::vec2 k2 = eval_dir(p_pos + k1 * (0.5f * p_step_size), k1);
    const glm::vec2 k3 = eval_dir(p_pos + k2 * (0.5f * p_step_size), k2);
    const glm::vec2 k4 = eval_dir(p_pos + k3 * p_step_size, k3);

    glm::vec2 combined = (k1 + 2.0f * k2 + 2.0f * k3 + k4) * (1.0f / 6.0f);
    const float len = glm::length(combined);
    if (len <= 1e-6f) return glm::vec2(0.0f);
    combined = align_vector_sign(combined * (1.0f / len), p_prev_dir);
    return combined * p_step_size;
}

uint32_t TensorFieldMath2D::detect_grid_singularities(
    const TensorGridHeaderPod &p_header,
    std::span<const SymmetricTensor2DPod> p_grid_tensors,
    std::span<TensorSingularityPod> r_out_singularities
) noexcept {
    const uint32_t w = p_header.width;
    const uint32_t h = p_header.height;
    if (w < 2u || h < 2u || p_header.cell_offset + w * h > p_grid_tensors.size() || r_out_singularities.empty()) {
        return 0u;
    }

    const SymmetricTensor2DPod *base = p_grid_tensors.data() + p_header.cell_offset;
    uint32_t found = 0u;
    constexpr float PI = std::numbers::pi_v<float>;

    for (uint32_t y = 0; y + 1u < h && found < r_out_singularities.size(); ++y) {
        for (uint32_t x = 0; x + 1u < w && found < r_out_singularities.size(); ++x) {
            const SymmetricTensor2DPod &t00 = base[y * w + x];
            const SymmetricTensor2DPod &t10 = base[y * w + (x + 1u)];
            const SymmetricTensor2DPod &t11 = base[(y + 1u) * w + (x + 1u)];
            const SymmetricTensor2DPod &t01 = base[(y + 1u) * w + x];

            // Double-angles `psi = 2*theta = atan2(b, a)` around the counter-clockwise 2x2 cell loop
            const float psi00 = std::atan2(t00.b, t00.a);
            const float psi10 = std::atan2(t10.b, t10.a);
            const float psi11 = std::atan2(t11.b, t11.a);
            const float psi01 = std::atan2(t01.b, t01.a);

            const float d_psi =
                wrap_angle_pi(psi10 - psi00) +
                wrap_angle_pi(psi11 - psi10) +
                wrap_angle_pi(psi01 - psi11) +
                wrap_angle_pi(psi00 - psi01);

            // Poincaré index I = d_psi / (4 * pi) because psi = 2 * theta
            const float index = d_psi / (4.0f * PI);
            if (std::abs(index) < 0.25f) continue;

            TensorSingularityPod sing{};
            sing.position = p_header.origin + glm::vec2(
                (static_cast<float>(x) + 1.0f) * p_header.cell_size.x,
                (static_cast<float>(y) + 1.0f) * p_header.cell_size.y
            );
            sing.poincare_index = index;
            sing.cell_x         = static_cast<uint16_t>(x);
            sing.flags          = static_cast<uint8_t>(y & 0xFFu);

            if (index > 0.75f) {
                sing.singularity_type = TENSOR_SINGULARITY_NODE_CENTER;
            } else if (index > 0.25f) {
                sing.singularity_type = TENSOR_SINGULARITY_WEDGE;
            } else if (index < -0.75f) {
                sing.singularity_type = TENSOR_SINGULARITY_SADDLE;
            } else {
                sing.singularity_type = TENSOR_SINGULARITY_TRISECTOR;
            }

            r_out_singularities[found++] = sing;
        }
    }
    return found;
}

void TensorFieldMath2D::trace_single_hyperstreamline(
    const glm::vec2 &p_seed_pos,
    bool p_follow_minor,
    float p_step_size,
    uint32_t p_max_steps,
    const AABB2D<float> &p_bounds,
    const TensorGridHeaderPod &p_header,
    std::span<const SymmetricTensor2DPod> p_grid_tensors,
    std::span<const TensorBasisElementPod> p_fallback_bases,
    std::vector<glm::vec2> &r_out_points
) {
    r_out_points.clear();
    if (!p_bounds.contains_point(p_seed_pos) || p_step_size <= 1e-5f || p_max_steps == 0u) {
        return;
    }

    const uint32_t half_steps = std::max(1u, p_max_steps / 2u);
    std::vector<glm::vec2> backward_pts;
    backward_pts.reserve(half_steps);

    // 1. Trace backward (`-initial_dir`)
    glm::vec2 init_step = rk4_step_eigenvector_field(
        p_seed_pos, glm::vec2(0.0f), p_step_size, p_follow_minor, p_header, p_grid_tensors, p_fallback_bases
    );
    if (glm::dot(init_step, init_step) <= 1e-10f) return;

    {
        glm::vec2 pos      = p_seed_pos;
        glm::vec2 prev_dir = -glm::normalize(init_step);
        for (uint32_t s = 0; s < half_steps; ++s) {
            const glm::vec2 delta = rk4_step_eigenvector_field(
                pos, prev_dir, p_step_size, p_follow_minor, p_header, p_grid_tensors, p_fallback_bases
            );
            const float len = glm::length(delta);
            if (len <= 1e-5f) break;
            pos += delta;
            prev_dir = delta * (1.0f / len);
            backward_pts.push_back(pos);
            if (!p_bounds.contains_point(pos)) break;
        }
    }

    r_out_points.reserve(backward_pts.size() + 1u + half_steps);
    for (auto it = backward_pts.rbegin(); it != backward_pts.rend(); ++it) {
        r_out_points.push_back(*it);
    }
    r_out_points.push_back(p_seed_pos);

    // 2. Trace forward (`+initial_dir`)
    {
        glm::vec2 pos      = p_seed_pos;
        glm::vec2 prev_dir = glm::normalize(init_step);
        for (uint32_t s = 0; s < half_steps; ++s) {
            const glm::vec2 delta = rk4_step_eigenvector_field(
                pos, prev_dir, p_step_size, p_follow_minor, p_header, p_grid_tensors, p_fallback_bases
            );
            const float len = glm::length(delta);
            if (len <= 1e-5f) break;
            pos += delta;
            prev_dir = delta * (1.0f / len);
            r_out_points.push_back(pos);
            if (!p_bounds.contains_point(pos)) break;
        }
    }
}

TensorNetworkResult2D TensorFieldMath2D::build_hyperstreamline_network(
    const TensorGridHeaderPod &p_header,
    std::span<const SymmetricTensor2DPod> p_grid_tensors,
    std::span<const TensorBasisElementPod> p_fallback_bases,
    std::span<const TensorStreamlineSeedPod> p_custom_seeds,
    const TensorTracingConfig2D &p_config
) {
    TensorNetworkResult2D result{};
    const AABB2D<float> bounds = p_config.world_bounds.is_valid()
        ? p_config.world_bounds
        : AABB2D<float>(
              p_header.origin,
              p_header.origin + glm::vec2(
                  static_cast<float>(p_header.width) * p_header.cell_size.x,
                  static_cast<float>(p_header.height) * p_header.cell_size.y
              )
          );

    // 1. Detect topological singularities
    std::array<TensorSingularityPod, 64> temp_sings{};
    const uint32_t sing_cnt = detect_grid_singularities(p_header, p_grid_tensors, temp_sings);
    result.singularities.assign(temp_sings.begin(), temp_sings.begin() + sing_cnt);

    // 2. Build priority-ordered seed queue (custom seeds + singularity seeds + stratified grid seeds)
    std::vector<TensorStreamlineSeedPod> seed_queue;
    seed_queue.reserve(p_custom_seeds.size() + 64u);
    for (const auto &s : p_custom_seeds) {
        if (s.active != 0u && bounds.contains_point(s.position)) {
            seed_queue.push_back(s);
        }
    }

    // Add seeds around detected singularities
    for (const auto &sing : result.singularities) {
        const float offset = std::max(p_config.step_size, 0.25f);
        for (int k = 0; k < 4; ++k) {
            const float ang = static_cast<float>(k) * (0.5f * std::numbers::pi_v<float>);
            const glm::vec2 pt = sing.position + glm::vec2(std::cos(ang), std::sin(ang)) * offset;
            if (bounds.contains_point(pt)) {
                seed_queue.push_back(TensorStreamlineSeedPod{pt, 10.0f, static_cast<uint8_t>(k & 1), 0u, 1u, 0u});
            }
        }
    }

    // Add stratified major/minor seeds across the bounding box if needed
    const glm::vec2 extent = bounds.get_size();
    const float maj_sep    = std::max(0.25f, p_config.major_separation);
    const float min_sep    = std::max(0.25f, p_config.minor_separation);
    const uint32_t cols    = std::clamp(static_cast<uint32_t>(std::ceil(extent.x / maj_sep)), 2u, 24u);
    const uint32_t rows    = std::clamp(static_cast<uint32_t>(std::ceil(extent.y / min_sep)), 2u, 24u);

    for (uint32_t r = 0; r < rows; ++r) {
        for (uint32_t c = 0; c < cols; ++c) {
            const glm::vec2 pt(
                bounds.min_bounds.x + (static_cast<float>(c) + 0.5f) * (extent.x / static_cast<float>(cols)),
                bounds.min_bounds.y + (static_cast<float>(r) + 0.5f) * (extent.y / static_cast<float>(rows))
            );
            seed_queue.push_back(TensorStreamlineSeedPod{pt, 1.0f, 0u, 1u, 1u, 0u});
            seed_queue.push_back(TensorStreamlineSeedPod{pt, 0.9f, 1u, 1u, 1u, 0u});
        }
    }

    std::sort(seed_queue.begin(), seed_queue.end(), [](const auto &a, const auto &b) {
        return a.priority > b.priority;
    });

    // Helper to check if a point is too close to an already-traced streamline of the same family
    std::vector<glm::vec2> major_samples;
    std::vector<glm::vec2> minor_samples;
    auto is_too_close = [&](const glm::vec2 &pt, bool is_minor, float sep) noexcept -> bool {
        const float sep_sq = sep * sep;
        const auto &samples = is_minor ? minor_samples : major_samples;
        for (const glm::vec2 &existing : samples) {
            const glm::vec2 d = pt - existing;
            if (glm::dot(d, d) < sep_sq) return true;
        }
        return false;
    };

    // 3. Trace streamlines with separation culling and collinear reduction
    for (const auto &seed : seed_queue) {
        if (result.streamlines.size() >= p_config.max_streamlines) break;
        const bool is_minor = (seed.follow_minor != 0u);
        const float sep     = is_minor ? maj_sep : min_sep;

        if (is_too_close(seed.position, is_minor, sep * 0.85f)) {
            continue;
        }

        std::vector<glm::vec2> raw_pts;
        trace_single_hyperstreamline(
            seed.position,
            is_minor,
            p_config.step_size,
            p_config.max_steps_per_stream,
            bounds,
            p_header,
            p_grid_tensors,
            p_fallback_bases,
            raw_pts
        );
        if (raw_pts.size() < 2u) continue;

        // Collinear reduction (matching NetworkBuilder::LinearReduction in legacy C#)
        std::vector<glm::vec2> reduced;
        reduced.reserve(raw_pts.size());
        reduced.push_back(raw_pts.front());
        for (size_t i = 1; i + 1 < raw_pts.size(); ++i) {
            const glm::vec2 d0 = glm::normalize(raw_pts[i] - reduced.back());
            const glm::vec2 d1 = glm::normalize(raw_pts[i + 1] - raw_pts[i]);
            if (glm::dot(d0, d1) < p_config.collinear_cos_threshold) {
                reduced.push_back(raw_pts[i]);
            }
        }
        reduced.push_back(raw_pts.back());

        // Record spatial samples for separation checks
        auto &target_samples = is_minor ? minor_samples : major_samples;
        for (const glm::vec2 &p : raw_pts) {
            target_samples.push_back(p);
        }

        result.streamlines.push_back(std::move(reduced));
        result.streamline_is_minor.push_back(is_minor ? 1u : 0u);
    }

    // 4. Optional boundary loop & DCEL Planar Graph / Parcel Region construction
    struct RawSegment {
        glm::vec2 a;
        glm::vec2 b;
        std::vector<float> split_ts{0.0f, 1.0f};
    };

    std::vector<RawSegment> segments;
    for (const auto &stream : result.streamlines) {
        for (size_t i = 0; i + 1 < stream.size(); ++i) {
            segments.push_back(RawSegment{stream[i], stream[i + 1], {0.0f, 1.0f}});
        }
    }
    if (p_config.trace_boundary_loop && bounds.is_valid()) {
        const glm::vec2 c0(bounds.min_bounds.x, bounds.min_bounds.y);
        const glm::vec2 c1(bounds.max_bounds.x, bounds.min_bounds.y);
        const glm::vec2 c2(bounds.max_bounds.x, bounds.max_bounds.y);
        const glm::vec2 c3(bounds.min_bounds.x, bounds.max_bounds.y);
        segments.push_back(RawSegment{c0, c1, {0.0f, 1.0f}});
        segments.push_back(RawSegment{c1, c2, {0.0f, 1.0f}});
        segments.push_back(RawSegment{c2, c3, {0.0f, 1.0f}});
        segments.push_back(RawSegment{c3, c0, {0.0f, 1.0f}});
    }

    // Split intersecting segments (replaces TracerEdge::Split in legacy C#)
    for (size_t i = 0; i < segments.size(); ++i) {
        for (size_t j = i + 1; j < segments.size(); ++j) {
            glm::vec2 hit{};
            float t = 0.0f, u = 0.0f;
            if (intersect_segments_2d(segments[i].a, segments[i].b, segments[j].a, segments[j].b, hit, t, u)) {
                segments[i].split_ts.push_back(t);
                segments[j].split_ts.push_back(u);
            }
        }
    }

    // Weld vertices within `weld_r`
    const float weld_r    = std::max(1e-3f, p_config.merge_distance * 0.25f);
    const float weld_r_sq = weld_r * weld_r;
    auto find_or_add_vertex = [&](const glm::vec2 &p_pt) -> uint32_t {
        for (size_t i = 0; i < result.graph_vertices.size(); ++i) {
            const glm::vec2 d = result.graph_vertices[i] - p_pt;
            if (glm::dot(d, d) <= weld_r_sq) {
                return static_cast<uint32_t>(i);
            }
        }
        result.graph_vertices.push_back(p_pt);
        return static_cast<uint32_t>(result.graph_vertices.size() - 1u);
    };

    struct UndirectedEdge {
        uint32_t u;
        uint32_t v;
    };
    std::vector<UndirectedEdge> unique_edges;

    for (auto &seg : segments) {
        std::sort(seg.split_ts.begin(), seg.split_ts.end());
        for (size_t k = 0; k + 1 < seg.split_ts.size(); ++k) {
            const float t0 = seg.split_ts[k];
            const float t1 = seg.split_ts[k + 1];
            if (t1 - t0 <= 1e-4f) continue;
            const glm::vec2 p0 = glm::mix(seg.a, seg.b, t0);
            const glm::vec2 p1 = glm::mix(seg.a, seg.b, t1);
            const uint32_t u   = find_or_add_vertex(p0);
            const uint32_t v   = find_or_add_vertex(p1);
            if (u == v) continue;
            const uint32_t lo = std::min(u, v);
            const uint32_t hi = std::max(u, v);
            bool exists = false;
            for (const auto &e : unique_edges) {
                if (e.u == lo && e.v == hi) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                unique_edges.push_back(UndirectedEdge{lo, hi});
            }
        }
    }

    // Build PlanarHalfEdgePOD pairs (2 * unique_edges.size()) and sort outgoing half-edges CCW per vertex
    result.half_edges.resize(unique_edges.size() * 2u);
    std::vector<std::vector<uint32_t>> outgoing_per_vert(result.graph_vertices.size());

    for (size_t i = 0; i < unique_edges.size(); ++i) {
        const uint32_t he0 = static_cast<uint32_t>(2u * i);
        const uint32_t he1 = static_cast<uint32_t>(2u * i + 1u);
        const uint32_t u   = unique_edges[i].u;
        const uint32_t v   = unique_edges[i].v;

        result.half_edges[he0].origin_vertex = u;
        result.half_edges[he0].twin_edge     = he1;
        result.half_edges[he1].origin_vertex = v;
        result.half_edges[he1].twin_edge     = he0;

        outgoing_per_vert[u].push_back(he0);
        outgoing_per_vert[v].push_back(he1);
    }

    // Sort outgoing half-edges around each vertex by angle so we can link `next_edge` via planar face walking
    for (size_t v = 0; v < outgoing_per_vert.size(); ++v) {
        auto &adj = outgoing_per_vert[v];
        if (adj.empty()) continue;
        const glm::vec2 orig = result.graph_vertices[v];
        std::sort(adj.begin(), adj.end(), [&](uint32_t a_he, uint32_t b_he) {
            const uint32_t dest_a = result.half_edges[result.half_edges[a_he].twin_edge].origin_vertex;
            const uint32_t dest_b = result.half_edges[result.half_edges[b_he].twin_edge].origin_vertex;
            const glm::vec2 da    = result.graph_vertices[dest_a] - orig;
            const glm::vec2 db    = result.graph_vertices[dest_b] - orig;
            return std::atan2(da.y, da.x) < std::atan2(db.y, db.x);
        });

        // For an incoming half-edge `in_he` (whose twin is `adj[k]`), the next interior boundary edge
        // turning sharpest clockwise/left is the previous entry in the CCW-sorted list around `v`.
        const size_t deg = adj.size();
        for (size_t k = 0; k < deg; ++k) {
            const uint32_t out_he = adj[k];
            const uint32_t in_he  = result.half_edges[out_he].twin_edge;
            const uint32_t next_he = adj[(k + deg - 1u) % deg];
            result.half_edges[in_he].next_edge   = next_he;
            result.half_edges[next_he].prev_edge = in_he;
        }
    }

    // 5. Walk closed interior faces (replaces TracedRegionBuilder::WalkRegionBoundary in legacy C#)
    if (p_config.extract_closed_regions && !result.half_edges.empty()) {
        std::vector<uint8_t> visited(result.half_edges.size(), 0u);
        const float max_parcel_area = bounds.get_area() * 0.92f;

        for (size_t i = 0; i < result.half_edges.size(); ++i) {
            if (visited[i] != 0u) continue;

            std::vector<uint32_t> cycle_verts;
            uint32_t curr = static_cast<uint32_t>(i);
            bool valid_cycle = true;
            uint32_t guard = 0u;

            while (visited[curr] == 0u && guard < 256u) {
                visited[curr] = 1u;
                cycle_verts.push_back(result.half_edges[curr].origin_vertex);
                curr = result.half_edges[curr].next_edge;
                if (curr >= result.half_edges.size()) {
                    valid_cycle = false;
                    break;
                }
                ++guard;
            }

            if (!valid_cycle || curr != static_cast<uint32_t>(i) || cycle_verts.size() < 3u) {
                continue;
            }

            // Remove dead-end spurs (vertex followed by immediate backtrack A -> B -> A)
            bool removed_spur = true;
            while (removed_spur && cycle_verts.size() >= 3u) {
                removed_spur = false;
                for (size_t k = 0; k < cycle_verts.size(); ++k) {
                    const uint32_t prev_v = cycle_verts[(k + cycle_verts.size() - 1u) % cycle_verts.size()];
                    const uint32_t next_v = cycle_verts[(k + 1u) % cycle_verts.size()];
                    if (prev_v == next_v) {
                        cycle_verts.erase(cycle_verts.begin() + static_cast<ptrdiff_t>(k));
                        if (!cycle_verts.empty()) {
                            const size_t dup_idx = k % cycle_verts.size();
                            cycle_verts.erase(cycle_verts.begin() + static_cast<ptrdiff_t>(dup_idx));
                        }
                        removed_spur = true;
                        break;
                    }
                }
            }

            if (cycle_verts.size() < 3u) continue;

            std::vector<glm::vec2> poly;
            poly.reserve(cycle_verts.size());
            for (uint32_t vid : cycle_verts) {
                poly.push_back(result.graph_vertices[vid]);
            }

            const float s_area = PolygonMath2D<float>::signed_area(poly);
            const float abs_a  = std::abs(s_area);
            // Keep interior counter-clockwise faces (exclude the outer unbounded face & tiny slivers)
            if (s_area > 1e-3f && abs_a < max_parcel_area) {
                const uint32_t face_id = static_cast<uint32_t>(result.closed_regions.size());
                result.closed_regions.push_back(std::move(poly));
                (void)face_id;
            }
        }
    }

    return result;
}

} // namespace ideam::core
