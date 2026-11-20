#pragma once

/**
 * cymatics_dod.h
 * Pure C++26 DOD pipeline engine for `IdeamCymatics` (`src/core/tasks/domains/cymatics/`).
 */

#include "cymatics_dod_common.h"
#include "../../../cymatics/cymatics_solver_core.h"

#include <cstdint>
#include <vector>

namespace ideam::core {

class CymaticsDOD {
private:
    CymaticGridConfigPOD config_;
    std::vector<float> wave_prev_;
    std::vector<float> wave_curr_;
    std::vector<float> wave_next_;
    std::vector<float> modal_field_;
    std::vector<float> reflectivity_;
    std::vector<uint8_t> shadow_classes_;
    std::vector<CymaticSectorPOD> sectors_;
    float listener_x_        = 16.0f;
    float listener_y_        = 16.0f;
    float listener_yaw_rad_  = 0.0f;
    float source_x_          = 16.0f;
    float source_y_          = 16.0f;
    float sim_time_sec_      = 0.0f;

public:
    CymaticsDOD();

    void configure(const CymaticGridConfigPOD &p_cfg);
    [[nodiscard]] const CymaticGridConfigPOD &get_config() const noexcept { return config_; }

    void set_listener(float p_grid_x, float p_grid_y, float p_yaw_rad) noexcept;
    void set_primary_source(float p_grid_x, float p_grid_y) noexcept;
    void set_sector_count(uint32_t p_count);
    void inject_impulse(uint32_t p_x, uint32_t p_y, float p_amplitude) noexcept;
    void set_obstacle(uint32_t p_x, uint32_t p_y, float p_reflectivity) noexcept;

    bool execute_pipeline(
        uint32_t p_stage_mask = CYM_STAGE_DEFAULT_MASK,
        const float *p_fft_peaks = nullptr,
        uint32_t p_fft_count     = 0u
    ) noexcept;

    [[nodiscard]] const std::vector<float> &get_wave_field() const noexcept { return wave_curr_; }
    [[nodiscard]] const std::vector<float> &get_modal_field() const noexcept { return modal_field_; }
    [[nodiscard]] const std::vector<uint8_t> &get_shadow_classes() const noexcept { return shadow_classes_; }
    [[nodiscard]] const std::vector<CymaticSectorPOD> &get_sectors() const noexcept { return sectors_; }
};

} // namespace ideam::core
