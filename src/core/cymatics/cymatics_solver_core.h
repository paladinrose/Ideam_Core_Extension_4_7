#pragma once

/**
 * cymatics_solver_core.h
 * Pure C++26 acoustic wave, Chladni/Bessel modal field, nodal particle drift,
 * mesh resonance displacement, directional sector reduction, and shadow-zone classifier
 * (`src/core/cymatics/`).
 */

#include "cymatics_common.h"

#include <cstdint>
#include <vector>

namespace ideam::core {

class CymaticsSolverCore {
public:
    // --- CYM1: 2D / 3D FDTD Acoustic Wave Propagation ---
    static void step_fdtd_2d(
        const float *p_prev,
        const float *p_curr,
        float *p_next,
        const float *p_reflectivity,
        const CymaticGridConfigPOD &p_cfg
    ) noexcept;

    static void step_fdtd_3d(
        const float *p_prev,
        const float *p_curr,
        float *p_next,
        const float *p_reflectivity,
        const CymaticGridConfigPOD &p_cfg
    ) noexcept;

    // --- CYM2: Chladni Plate & Circular Bessel Modal Field ---
    [[nodiscard]] static float bessel_j(int p_order, float p_x) noexcept;

    static void evaluate_chladni_modal_field(
        float *p_out_field,
        const CymaticGridConfigPOD &p_cfg,
        const float *p_fft_peaks = nullptr,
        uint32_t p_fft_count     = 0u,
        float p_time_sec         = 0.0f
    ) noexcept;

    // --- CYM3: Stochastic Gradient Nodal Drift ---
    static void advect_particles_nodal_drift(
        float *p_pos_x,
        float *p_pos_y,
        uint32_t p_particle_count,
        const float *p_field,
        const CymaticGridConfigPOD &p_cfg,
        float p_drift_strength,
        float p_jitter_amplitude,
        uint32_t p_seed
    ) noexcept;

    // --- CYM4: 3D Mesh Resonance Displacement ---
    static void displace_mesh_resonance(
        float *p_verts_xyz,
        const float *p_normals_xyz,
        uint32_t p_vertex_count,
        const float *p_field,
        const CymaticGridConfigPOD &p_cfg,
        float p_displacement_scale,
        float *p_out_nodal_intensity = nullptr
    ) noexcept;

    // --- CYM5: Listener Directional Sector Reduction ---
    static void reduce_directional_sectors(
        const float *p_field,
        const CymaticGridConfigPOD &p_cfg,
        float p_listener_grid_x,
        float p_listener_grid_y,
        float p_reference_yaw_rad,
        CymaticSectorPOD *p_out_sectors,
        uint32_t p_sector_count
    ) noexcept;

    // --- CYM6: Acoustic Shadow Zone Classification (0=LOS, 1=Diffracted, 2=Reverberant, 3=Shadow) ---
    static void classify_shadow_zones(
        const float *p_field,
        const float *p_reflectivity,
        uint8_t *p_out_lod_classes,
        const CymaticGridConfigPOD &p_cfg,
        float p_source_grid_x,
        float p_source_grid_y
    ) noexcept;
};

} // namespace ideam::core
