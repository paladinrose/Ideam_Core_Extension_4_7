#include "cymatics_solver_core.h"

#include <algorithm>
#include <cmath>

namespace ideam::core {

namespace {
constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

[[nodiscard]] inline uint32_t xorshift32(uint32_t &state) noexcept {
    state ^= state << 13u;
    state ^= state >> 17u;
    state ^= state << 5u;
    return state;
}

[[nodiscard]] inline float rand_signed(uint32_t &state) noexcept {
    return (static_cast<float>(xorshift32(state) & 0x00FFFFFFu) / 16777215.0f) * 2.0f - 1.0f;
}
} // namespace

void CymaticsSolverCore::step_fdtd_2d(
    const float *p_prev,
    const float *p_curr,
    float *p_next,
    const float *p_reflectivity,
    const CymaticGridConfigPOD &p_cfg
) noexcept {
    if (!p_prev || !p_curr || !p_next || p_cfg.width < 3u || p_cfg.height < 3u) {
        return;
    }
    const uint32_t w = p_cfg.width;
    const uint32_t h = p_cfg.height;
    const float dx   = std::max(1e-4f, p_cfg.cell_size);
    const float cfl  = std::min(0.49f, (p_cfg.wave_speed * p_cfg.dt / dx) * (p_cfg.wave_speed * p_cfg.dt / dx));
    const float damp = std::clamp(p_cfg.damping, 0.0f, 1.0f);

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            const uint32_t idx = y * w + x;
            if (x == 0 || y == 0 || x + 1u == w || y + 1u == h) {
                p_next[idx] = 0.0f;
                continue;
            }
            const float refl = p_reflectivity ? std::clamp(p_reflectivity[idx], 0.0f, 1.0f) : 0.0f;
            if (refl >= 0.99f) {
                p_next[idx] = 0.0f;
                continue;
            }
            const float laplacian =
                p_curr[idx - 1u] + p_curr[idx + 1u] + p_curr[idx - w] + p_curr[idx + w] - 4.0f * p_curr[idx];
            const float val = (2.0f * p_curr[idx] - p_prev[idx] + cfl * laplacian) * damp * (1.0f - refl * 0.5f);
            p_next[idx]     = std::clamp(val, -4.0f, 4.0f);
        }
    }
}

void CymaticsSolverCore::step_fdtd_3d(
    const float *p_prev,
    const float *p_curr,
    float *p_next,
    const float *p_reflectivity,
    const CymaticGridConfigPOD &p_cfg
) noexcept {
    if (p_cfg.depth <= 1u) {
        step_fdtd_2d(p_prev, p_curr, p_next, p_reflectivity, p_cfg);
        return;
    }
    if (!p_prev || !p_curr || !p_next || p_cfg.width < 3u || p_cfg.height < 3u || p_cfg.depth < 3u) {
        return;
    }
    const uint32_t w     = p_cfg.width;
    const uint32_t h     = p_cfg.height;
    const uint32_t d     = p_cfg.depth;
    const uint32_t slice = w * h;
    const float dx       = std::max(1e-4f, p_cfg.cell_size);
    const float cfl      = std::min(0.32f, (p_cfg.wave_speed * p_cfg.dt / dx) * (p_cfg.wave_speed * p_cfg.dt / dx));
    const float damp     = std::clamp(p_cfg.damping, 0.0f, 1.0f);

    for (uint32_t z = 0; z < d; ++z) {
        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                const uint32_t idx = z * slice + y * w + x;
                if (x == 0 || y == 0 || z == 0 || x + 1u == w || y + 1u == h || z + 1u == d) {
                    p_next[idx] = 0.0f;
                    continue;
                }
                const float refl = p_reflectivity ? std::clamp(p_reflectivity[idx], 0.0f, 1.0f) : 0.0f;
                if (refl >= 0.99f) {
                    p_next[idx] = 0.0f;
                    continue;
                }
                const float lap =
                    p_curr[idx - 1u] + p_curr[idx + 1u] +
                    p_curr[idx - w] + p_curr[idx + w] +
                    p_curr[idx - slice] + p_curr[idx + slice] - 6.0f * p_curr[idx];
                const float val = (2.0f * p_curr[idx] - p_prev[idx] + cfl * lap) * damp * (1.0f - refl * 0.5f);
                p_next[idx]     = std::clamp(val, -4.0f, 4.0f);
            }
        }
    }
}

float CymaticsSolverCore::bessel_j(int p_order, float p_x) noexcept {
    // Fast, deterministic power series for small/moderate arguments (|x| <= 16)
    const int n       = std::max(0, p_order);
    const float halfx = 0.5f * p_x;
    float term        = 1.0f;
    for (int i = 1; i <= n; ++i) {
        term *= halfx / static_cast<float>(i);
    }
    float sum       = term;
    const float sq  = -halfx * halfx;
    for (int k = 1; k <= 14; ++k) {
        term *= sq / (static_cast<float>(k) * static_cast<float>(k + n));
        sum += term;
        if (std::fabs(term) < 1e-6f) {
            break;
        }
    }
    return sum;
}

void CymaticsSolverCore::evaluate_chladni_modal_field(
    float *p_out_field,
    const CymaticGridConfigPOD &p_cfg,
    const float *p_fft_peaks,
    uint32_t p_fft_count,
    float p_time_sec
) noexcept {
    if (!p_out_field || p_cfg.width == 0u || p_cfg.height == 0u) {
        return;
    }
    const uint32_t w   = p_cfg.width;
    const uint32_t h   = p_cfg.height;
    const auto mode    = static_cast<CymaticWaveMode>(p_cfg.wave_mode);
    const float inv_w  = (w > 1u) ? (1.0f / static_cast<float>(w - 1u)) : 1.0f;
    const float inv_h  = (h > 1u) ? (1.0f / static_cast<float>(h - 1u)) : 1.0f;
    const int b_order  = std::max(0, static_cast<int>(std::lround(p_cfg.bessel_order)));
    const float b_root = std::max(0.5f, p_cfg.bessel_root);
    const float phase  = std::cos(p_time_sec * kTwoPi * 0.5f);

    const uint32_t harmonics = (p_fft_peaks && p_fft_count > 0u) ? std::min<uint32_t>(p_fft_count, 4u) : 1u;

    for (uint32_t y = 0; y < h; ++y) {
        const float ny = static_cast<float>(y) * inv_h; // [0, 1]
        const float cy = ny * 2.0f - 1.0f;              // [-1, 1]
        for (uint32_t x = 0; x < w; ++x) {
            const float nx = static_cast<float>(x) * inv_w;
            const float cx = nx * 2.0f - 1.0f;

            float accum = 0.0f;
            float wsum  = 0.0f;
            for (uint32_t k = 0; k < harmonics; ++k) {
                const float gain = (p_fft_peaks && p_fft_count > 0u) ? std::max(0.0f, p_fft_peaks[k]) : 1.0f;
                const float mk   = p_cfg.chladni_m + static_cast<float>(k);
                const float nk   = p_cfg.chladni_n + static_cast<float>(k) * 0.5f;

                float plate = p_cfg.chladni_a * std::cos(kPi * nk * nx) * std::cos(kPi * mk * ny) +
                              p_cfg.chladni_b * std::cos(kPi * mk * nx) * std::cos(kPi * nk * ny);

                float circ = 0.0f;
                const float r = std::sqrt(cx * cx + cy * cy);
                if (r <= 1.0f) {
                    const float theta = std::atan2(cy, cx);
                    circ = bessel_j(b_order + static_cast<int>(k), b_root * r) *
                           std::cos(static_cast<float>(b_order + static_cast<int>(k)) * theta);
                }

                float sample = plate;
                if (mode == CymaticWaveMode::MODE_BESSEL_CIRCULAR) {
                    sample = circ;
                } else if (mode == CymaticWaveMode::MODE_HYBRID) {
                    sample = 0.5f * (plate + circ);
                }
                accum += sample * gain;
                wsum += gain;
            }
            const float norm = (wsum > 1e-5f) ? (accum / wsum) : accum;
            p_out_field[y * w + x] = std::clamp(norm * (0.75f + 0.25f * phase), -1.0f, 1.0f);
        }
    }
}

void CymaticsSolverCore::advect_particles_nodal_drift(
    float *p_pos_x,
    float *p_pos_y,
    uint32_t p_particle_count,
    const float *p_field,
    const CymaticGridConfigPOD &p_cfg,
    float p_drift_strength,
    float p_jitter_amplitude,
    uint32_t p_seed
) noexcept {
    if (!p_pos_x || !p_pos_y || !p_field || p_cfg.width < 3u || p_cfg.height < 3u) {
        return;
    }
    const uint32_t w = p_cfg.width;
    const uint32_t h = p_cfg.height;
    uint32_t rng     = (p_seed == 0u) ? 0x9E3779B9u : p_seed;

    auto sample_sq = [&](int gx, int gy) -> float {
        const uint32_t cx = static_cast<uint32_t>(std::clamp(gx, 0, static_cast<int>(w - 1u)));
        const uint32_t cy = static_cast<uint32_t>(std::clamp(gy, 0, static_cast<int>(h - 1u)));
        const float v     = p_field[cy * w + cx];
        return v * v;
    };

    for (uint32_t i = 0; i < p_particle_count; ++i) {
        const float px = p_pos_x[i];
        const float py = p_pos_y[i];
        const int gx   = static_cast<int>(std::floor(px));
        const int gy   = static_cast<int>(std::floor(py));

        const float grad_x = 0.5f * (sample_sq(gx + 1, gy) - sample_sq(gx - 1, gy));
        const float grad_y = 0.5f * (sample_sq(gx, gy + 1) - sample_sq(gx, gy - 1));
        const float amp    = std::sqrt(sample_sq(gx, gy));

        const float nx = px - p_drift_strength * grad_x + rand_signed(rng) * p_jitter_amplitude * amp;
        const float ny = py - p_drift_strength * grad_y + rand_signed(rng) * p_jitter_amplitude * amp;

        p_pos_x[i] = std::clamp(nx, 0.0f, static_cast<float>(w - 1u));
        p_pos_y[i] = std::clamp(ny, 0.0f, static_cast<float>(h - 1u));
    }
}

void CymaticsSolverCore::displace_mesh_resonance(
    float *p_verts_xyz,
    const float *p_normals_xyz,
    uint32_t p_vertex_count,
    const float *p_field,
    const CymaticGridConfigPOD &p_cfg,
    float p_displacement_scale,
    float *p_out_nodal_intensity
) noexcept {
    if (!p_verts_xyz || !p_field || p_cfg.width == 0u || p_cfg.height == 0u) {
        return;
    }
    const float inv_span_x = 1.0f / std::max(1e-4f, static_cast<float>(p_cfg.width) * p_cfg.cell_size);
    const float inv_span_z = 1.0f / std::max(1e-4f, static_cast<float>(p_cfg.height) * p_cfg.cell_size);

    for (uint32_t i = 0; i < p_vertex_count; ++i) {
        const float vx = p_verts_xyz[i * 3u + 0u];
        const float vz = p_verts_xyz[i * 3u + 2u];
        const float u  = std::clamp(vx * inv_span_x + 0.5f, 0.0f, 0.9999f);
        const float v  = std::clamp(vz * inv_span_z + 0.5f, 0.0f, 0.9999f);

        const uint32_t gx = static_cast<uint32_t>(u * static_cast<float>(p_cfg.width));
        const uint32_t gy = static_cast<uint32_t>(v * static_cast<float>(p_cfg.height));
        const float amp   = p_field[gy * p_cfg.width + gx];

        const float nx = p_normals_xyz ? p_normals_xyz[i * 3u + 0u] : 0.0f;
        const float ny = p_normals_xyz ? p_normals_xyz[i * 3u + 1u] : 1.0f;
        const float nz = p_normals_xyz ? p_normals_xyz[i * 3u + 2u] : 0.0f;

        p_verts_xyz[i * 3u + 0u] += nx * amp * p_displacement_scale;
        p_verts_xyz[i * 3u + 1u] += ny * amp * p_displacement_scale;
        p_verts_xyz[i * 3u + 2u] += nz * amp * p_displacement_scale;

        if (p_out_nodal_intensity) {
            // 1.0 along exact nodal lines (amp == 0), decaying toward 0 at antinodes
            p_out_nodal_intensity[i] = std::clamp(1.0f - std::fabs(amp), 0.0f, 1.0f);
        }
    }
}

void CymaticsSolverCore::reduce_directional_sectors(
    const float *p_field,
    const CymaticGridConfigPOD &p_cfg,
    float p_listener_grid_x,
    float p_listener_grid_y,
    float p_reference_yaw_rad,
    CymaticSectorPOD *p_out_sectors,
    uint32_t p_sector_count
) noexcept {
    if (!p_field || !p_out_sectors || p_sector_count == 0u || p_cfg.width == 0u || p_cfg.height == 0u) {
        return;
    }
    const float sector_arc = kTwoPi / static_cast<float>(p_sector_count);
    for (uint32_t s = 0; s < p_sector_count; ++s) {
        p_out_sectors[s].energy      = 0.0f;
        p_out_sectors[s].azimuth_rad = static_cast<float>(s) * sector_arc;
    }

    for (uint32_t y = 0; y < p_cfg.height; ++y) {
        for (uint32_t x = 0; x < p_cfg.width; ++x) {
            const float dx = static_cast<float>(x) - p_listener_grid_x;
            const float dy = static_cast<float>(y) - p_listener_grid_y;
            const float d2 = dx * dx + dy * dy;
            if (d2 < 1e-4f) {
                continue;
            }
            const float val = p_field[y * p_cfg.width + x];
            const float e   = (val * val) / (1.0f + d2 * 0.25f);
            float ang       = std::atan2(dy, dx) - p_reference_yaw_rad;
            while (ang < 0.0f) ang += kTwoPi;
            while (ang >= kTwoPi) ang -= kTwoPi;
            const uint32_t sector = std::min<uint32_t>(
                p_sector_count - 1u,
                static_cast<uint32_t>(ang / sector_arc)
            );
            p_out_sectors[sector].energy += e;
        }
    }

    for (uint32_t s = 0; s < p_sector_count; ++s) {
        p_out_sectors[s].peak_hold = std::max(p_out_sectors[s].peak_hold * 0.94f, p_out_sectors[s].energy);
    }
}

void CymaticsSolverCore::classify_shadow_zones(
    const float *p_field,
    const float *p_reflectivity,
    uint8_t *p_out_lod_classes,
    const CymaticGridConfigPOD &p_cfg,
    float p_source_grid_x,
    float p_source_grid_y
) noexcept {
    if (!p_out_lod_classes || p_cfg.width == 0u || p_cfg.height == 0u) {
        return;
    }
    const uint32_t w = p_cfg.width;
    const uint32_t h = p_cfg.height;

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            const uint32_t idx = y * w + x;
            const float energy = p_field ? std::fabs(p_field[idx]) : 0.0f;

            // DDA ray march from source to (x, y) checking obstacle reflectivity
            const float dx   = static_cast<float>(x) - p_source_grid_x;
            const float dy   = static_cast<float>(y) - p_source_grid_y;
            const int steps  = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)))));
            bool occluded    = false;
            int obs_hits     = 0;

            if (p_reflectivity) {
                for (int s = 1; s < steps; ++s) {
                    const float t  = static_cast<float>(s) / static_cast<float>(steps);
                    const int sx   = std::clamp(static_cast<int>(std::lround(p_source_grid_x + dx * t)), 0, static_cast<int>(w - 1u));
                    const int sy   = std::clamp(static_cast<int>(std::lround(p_source_grid_y + dy * t)), 0, static_cast<int>(h - 1u));
                    if (p_reflectivity[static_cast<uint32_t>(sy) * w + static_cast<uint32_t>(sx)] >= 0.5f) {
                        occluded = true;
                        ++obs_hits;
                    }
                }
            }

            AcousticShadowClass cls = AcousticShadowClass::DIRECT_LOS;
            if (!occluded) {
                cls = (energy > 0.35f) ? AcousticShadowClass::DIRECT_LOS : AcousticShadowClass::REVERBERANT_CHAMBER;
            } else if (obs_hits == 1 && energy > 0.15f) {
                cls = AcousticShadowClass::DIFFRACTED_RIM;
            } else if (energy > 0.08f) {
                cls = AcousticShadowClass::REVERBERANT_CHAMBER;
            } else {
                cls = AcousticShadowClass::ACOUSTIC_SHADOW;
            }
            p_out_lod_classes[idx] = static_cast<uint8_t>(cls);
        }
    }
}

} // namespace ideam::core
