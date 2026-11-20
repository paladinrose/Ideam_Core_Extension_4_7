#pragma once

/**
 * cymatic_map.h
 * `CymaticMap` (`godot::Node`) in `src/godot/cymatics/`.
 *
 * Holds 2D/3D acoustic wave and Chladni/Bessel modal field parameters, feeds
 * `CymaticAudioCompass` (via `get_cymatic_field_bytes()`), `IdeamCommunications` medium
 * resonance, `MindDOD` Wave 1 audio perception, and `GameplayAudioOption`.
 */

#include "../../core/cymatics/cymatics_common.h"
#include "../../core/cymatics/cymatics_solver_core.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <vector>

namespace ideam::godot_ext {

enum CymaticWaveModeEnum : int32_t {
    CYMATIC_MODE_FDTD_WAVE       = static_cast<int32_t>(core::CymaticWaveMode::MODE_FDTD_WAVE),
    CYMATIC_MODE_CHLADNI_PLATE   = static_cast<int32_t>(core::CymaticWaveMode::MODE_CHLADNI_PLATE),
    CYMATIC_MODE_BESSEL_CIRCULAR = static_cast<int32_t>(core::CymaticWaveMode::MODE_BESSEL_CIRCULAR),
    CYMATIC_MODE_HYBRID          = static_cast<int32_t>(core::CymaticWaveMode::MODE_HYBRID)
};

class CymaticMap : public godot::Node {
    GDCLASS(CymaticMap, godot::Node)

private:
    core::CymaticGridConfigPOD config_;
    bool auto_step_in_process_ = false;
    float sim_time_sec_        = 0.0f;
    godot::PackedFloat32Array fft_peaks_;

    std::vector<float> wave_prev_;
    std::vector<float> wave_curr_;
    std::vector<float> wave_next_;
    std::vector<float> modal_field_;
    std::vector<float> reflectivity_;
    std::vector<uint8_t> shadow_classes_;

    void _allocate_buffers();

protected:
    static void _bind_methods();

public:
    CymaticMap();
    ~CymaticMap() override = default;

    void _process(double p_delta) override;

    // --- Grid & Wave Properties ---
    void set_grid_width(int p_w);
    [[nodiscard]] int get_grid_width() const noexcept { return static_cast<int>(config_.width); }

    void set_grid_height(int p_h);
    [[nodiscard]] int get_grid_height() const noexcept { return static_cast<int>(config_.height); }

    void set_grid_depth(int p_d);
    [[nodiscard]] int get_grid_depth() const noexcept { return static_cast<int>(config_.depth); }

    void set_cell_size(float p_size) { config_.cell_size = p_size > 1e-4f ? p_size : 1e-4f; }
    [[nodiscard]] float get_cell_size() const noexcept { return config_.cell_size; }

    void set_wave_speed(float p_speed) { config_.wave_speed = p_speed; }
    [[nodiscard]] float get_wave_speed() const noexcept { return config_.wave_speed; }

    void set_damping(float p_damping) { config_.damping = p_damping; }
    [[nodiscard]] float get_damping() const noexcept { return config_.damping; }

    void set_chladni_m(float p_m) { config_.chladni_m = p_m; }
    [[nodiscard]] float get_chladni_m() const noexcept { return config_.chladni_m; }

    void set_chladni_n(float p_n) { config_.chladni_n = p_n; }
    [[nodiscard]] float get_chladni_n() const noexcept { return config_.chladni_n; }

    void set_chladni_a(float p_a) { config_.chladni_a = p_a; }
    [[nodiscard]] float get_chladni_a() const noexcept { return config_.chladni_a; }

    void set_chladni_b(float p_b) { config_.chladni_b = p_b; }
    [[nodiscard]] float get_chladni_b() const noexcept { return config_.chladni_b; }

    void set_bessel_order(float p_order) { config_.bessel_order = p_order; }
    [[nodiscard]] float get_bessel_order() const noexcept { return config_.bessel_order; }

    void set_bessel_root(float p_root) { config_.bessel_root = p_root; }
    [[nodiscard]] float get_bessel_root() const noexcept { return config_.bessel_root; }

    void set_wave_mode(CymaticWaveModeEnum p_mode) { config_.wave_mode = static_cast<uint8_t>(p_mode); }
    [[nodiscard]] CymaticWaveModeEnum get_wave_mode() const noexcept {
        return static_cast<CymaticWaveModeEnum>(config_.wave_mode);
    }

    void set_auto_step_in_process(bool p_auto);
    [[nodiscard]] bool get_auto_step_in_process() const noexcept { return auto_step_in_process_; }

    void set_fft_peaks(const godot::PackedFloat32Array &p_peaks) { fft_peaks_ = p_peaks; }
    [[nodiscard]] godot::PackedFloat32Array get_fft_peaks() const { return fft_peaks_; }

    [[nodiscard]] const core::CymaticGridConfigPOD &get_grid_config_pod() const noexcept { return config_; }

    // --- Simulation & Query API ---
    void inject_impulse(int p_x, int p_y, float p_amplitude);
    void set_obstacle_reflectivity(int p_x, int p_y, float p_reflectivity);
    void step_wave(float p_dt = 0.016f);
    void evaluate_modal_field();

    // Direct `CymaticAudioCompass` & `HeatmapGrid` contract: packed float32 bytes of active 2D field
    [[nodiscard]] godot::PackedByteArray get_cymatic_field_bytes() const;
    [[nodiscard]] godot::PackedFloat32Array get_field_values() const;
    [[nodiscard]] godot::PackedInt32Array compute_shadow_zones(const godot::Vector2 &p_source_grid_pos);
    [[nodiscard]] godot::Array reduce_directional_sectors(
        const godot::Vector2 &p_listener_grid_pos,
        float p_reference_yaw_rad = 0.0f,
        int p_sector_count = 16
    ) const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::CymaticWaveModeEnum);
