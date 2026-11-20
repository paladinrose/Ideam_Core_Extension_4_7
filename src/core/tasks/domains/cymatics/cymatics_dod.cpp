#include "cymatics_dod.h"

#include <algorithm>

namespace ideam::core {

CymaticsDOD::CymaticsDOD() {
    configure(config_);
    set_sector_count(16u);
}

void CymaticsDOD::configure(const CymaticGridConfigPOD &p_cfg) {
    config_ = p_cfg;
    config_.width  = std::max<uint32_t>(3u, config_.width);
    config_.height = std::max<uint32_t>(3u, config_.height);
    config_.depth  = std::max<uint32_t>(1u, config_.depth);

    const size_t total = static_cast<size_t>(config_.width) *
                         static_cast<size_t>(config_.height) *
                         static_cast<size_t>(config_.depth);
    wave_prev_.assign(total, 0.0f);
    wave_curr_.assign(total, 0.0f);
    wave_next_.assign(total, 0.0f);
    modal_field_.assign(static_cast<size_t>(config_.width) * static_cast<size_t>(config_.height), 0.0f);
    reflectivity_.assign(total, 0.0f);
    shadow_classes_.assign(static_cast<size_t>(config_.width) * static_cast<size_t>(config_.height), 0u);
    listener_x_ = static_cast<float>(config_.width) * 0.5f;
    listener_y_ = static_cast<float>(config_.height) * 0.5f;
    source_x_   = listener_x_;
    source_y_   = listener_y_;
}

void CymaticsDOD::set_listener(float p_grid_x, float p_grid_y, float p_yaw_rad) noexcept {
    listener_x_       = p_grid_x;
    listener_y_       = p_grid_y;
    listener_yaw_rad_ = p_yaw_rad;
}

void CymaticsDOD::set_primary_source(float p_grid_x, float p_grid_y) noexcept {
    source_x_ = p_grid_x;
    source_y_ = p_grid_y;
}

void CymaticsDOD::set_sector_count(uint32_t p_count) {
    sectors_.assign(std::max<uint32_t>(4u, p_count), CymaticSectorPOD{});
}

void CymaticsDOD::inject_impulse(uint32_t p_x, uint32_t p_y, float p_amplitude) noexcept {
    if (p_x < config_.width && p_y < config_.height) {
        wave_curr_[p_y * config_.width + p_x] += p_amplitude;
    }
}

void CymaticsDOD::set_obstacle(uint32_t p_x, uint32_t p_y, float p_reflectivity) noexcept {
    if (p_x < config_.width && p_y < config_.height) {
        reflectivity_[p_y * config_.width + p_x] = std::clamp(p_reflectivity, 0.0f, 1.0f);
    }
}

bool CymaticsDOD::execute_pipeline(
    uint32_t p_stage_mask,
    const float *p_fft_peaks,
    uint32_t p_fft_count
) noexcept {
    sim_time_sec_ += config_.dt;

    if (p_stage_mask & CYM_STAGE_WAVE_PROPAGATE) {
        if (config_.depth > 1u) {
            CymaticsSolverCore::step_fdtd_3d(
                wave_prev_.data(), wave_curr_.data(), wave_next_.data(), reflectivity_.data(), config_
            );
        } else {
            CymaticsSolverCore::step_fdtd_2d(
                wave_prev_.data(), wave_curr_.data(), wave_next_.data(), reflectivity_.data(), config_
            );
        }
        wave_prev_.swap(wave_curr_);
        wave_curr_.swap(wave_next_);
    }

    if (p_stage_mask & CYM_STAGE_CHLADNI_MODAL) {
        CymaticsSolverCore::evaluate_chladni_modal_field(
            modal_field_.data(), config_, p_fft_peaks, p_fft_count, sim_time_sec_
        );
    }

    const float *active_2d = (config_.wave_mode == static_cast<uint8_t>(CymaticWaveMode::MODE_FDTD_WAVE))
        ? wave_curr_.data()
        : modal_field_.data();

    if (p_stage_mask & CYM_STAGE_SECTOR_REDUCE) {
        CymaticsSolverCore::reduce_directional_sectors(
            active_2d, config_, listener_x_, listener_y_, listener_yaw_rad_,
            sectors_.data(), static_cast<uint32_t>(sectors_.size())
        );
    }

    if (p_stage_mask & CYM_STAGE_SHADOW_ZONE) {
        CymaticsSolverCore::classify_shadow_zones(
            active_2d, reflectivity_.data(), shadow_classes_.data(), config_, source_x_, source_y_
        );
    }

    return true;
}

} // namespace ideam::core
