#include "cymatic_map.h"

#include <algorithm>
#include <cstring>

namespace ideam::godot_ext {

void CymaticMap::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(CYMATIC_MODE_FDTD_WAVE);
    BIND_ENUM_CONSTANT(CYMATIC_MODE_CHLADNI_PLATE);
    BIND_ENUM_CONSTANT(CYMATIC_MODE_BESSEL_CIRCULAR);
    BIND_ENUM_CONSTANT(CYMATIC_MODE_HYBRID);

    ADD_SIGNAL(MethodInfo("field_updated"));

    ClassDB::bind_method(D_METHOD("set_grid_width", "width"), &CymaticMap::set_grid_width);
    ClassDB::bind_method(D_METHOD("get_grid_width"), &CymaticMap::get_grid_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_width", PROPERTY_HINT_RANGE, "3,256,1"), "set_grid_width", "get_grid_width");

    ClassDB::bind_method(D_METHOD("set_grid_height", "height"), &CymaticMap::set_grid_height);
    ClassDB::bind_method(D_METHOD("get_grid_height"), &CymaticMap::get_grid_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_height", PROPERTY_HINT_RANGE, "3,256,1"), "set_grid_height", "get_grid_height");

    ClassDB::bind_method(D_METHOD("set_grid_depth", "depth"), &CymaticMap::set_grid_depth);
    ClassDB::bind_method(D_METHOD("get_grid_depth"), &CymaticMap::get_grid_depth);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_depth", PROPERTY_HINT_RANGE, "1,64,1"), "set_grid_depth", "get_grid_depth");

    ClassDB::bind_method(D_METHOD("set_cell_size", "cell_size"), &CymaticMap::set_cell_size);
    ClassDB::bind_method(D_METHOD("get_cell_size"), &CymaticMap::get_cell_size);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size"), "set_cell_size", "get_cell_size");

    ClassDB::bind_method(D_METHOD("set_wave_speed", "speed"), &CymaticMap::set_wave_speed);
    ClassDB::bind_method(D_METHOD("get_wave_speed"), &CymaticMap::get_wave_speed);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wave_speed"), "set_wave_speed", "get_wave_speed");

    ClassDB::bind_method(D_METHOD("set_damping", "damping"), &CymaticMap::set_damping);
    ClassDB::bind_method(D_METHOD("get_damping"), &CymaticMap::get_damping);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "damping", PROPERTY_HINT_RANGE, "0.5,1.0,0.001"), "set_damping", "get_damping");

    ClassDB::bind_method(D_METHOD("set_chladni_m", "m"), &CymaticMap::set_chladni_m);
    ClassDB::bind_method(D_METHOD("get_chladni_m"), &CymaticMap::get_chladni_m);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chladni_m"), "set_chladni_m", "get_chladni_m");

    ClassDB::bind_method(D_METHOD("set_chladni_n", "n"), &CymaticMap::set_chladni_n);
    ClassDB::bind_method(D_METHOD("get_chladni_n"), &CymaticMap::get_chladni_n);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chladni_n"), "set_chladni_n", "get_chladni_n");

    ClassDB::bind_method(D_METHOD("set_chladni_a", "a"), &CymaticMap::set_chladni_a);
    ClassDB::bind_method(D_METHOD("get_chladni_a"), &CymaticMap::get_chladni_a);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chladni_a"), "set_chladni_a", "get_chladni_a");

    ClassDB::bind_method(D_METHOD("set_chladni_b", "b"), &CymaticMap::set_chladni_b);
    ClassDB::bind_method(D_METHOD("get_chladni_b"), &CymaticMap::get_chladni_b);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chladni_b"), "set_chladni_b", "get_chladni_b");

    ClassDB::bind_method(D_METHOD("set_bessel_order", "order"), &CymaticMap::set_bessel_order);
    ClassDB::bind_method(D_METHOD("get_bessel_order"), &CymaticMap::get_bessel_order);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bessel_order"), "set_bessel_order", "get_bessel_order");

    ClassDB::bind_method(D_METHOD("set_bessel_root", "root"), &CymaticMap::set_bessel_root);
    ClassDB::bind_method(D_METHOD("get_bessel_root"), &CymaticMap::get_bessel_root);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bessel_root"), "set_bessel_root", "get_bessel_root");

    ClassDB::bind_method(D_METHOD("set_wave_mode", "mode"), &CymaticMap::set_wave_mode);
    ClassDB::bind_method(D_METHOD("get_wave_mode"), &CymaticMap::get_wave_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "wave_mode", PROPERTY_HINT_ENUM, "FDTD Wave:0,Chladni Plate:1,Bessel Circular:2,Hybrid:3"),
        "set_wave_mode",
        "get_wave_mode"
    );

    ClassDB::bind_method(D_METHOD("set_auto_step_in_process", "enabled"), &CymaticMap::set_auto_step_in_process);
    ClassDB::bind_method(D_METHOD("get_auto_step_in_process"), &CymaticMap::get_auto_step_in_process);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_step_in_process"), "set_auto_step_in_process", "get_auto_step_in_process");

    ClassDB::bind_method(D_METHOD("set_fft_peaks", "peaks"), &CymaticMap::set_fft_peaks);
    ClassDB::bind_method(D_METHOD("get_fft_peaks"), &CymaticMap::get_fft_peaks);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "fft_peaks"), "set_fft_peaks", "get_fft_peaks");

    ClassDB::bind_method(D_METHOD("inject_impulse", "x", "y", "amplitude"), &CymaticMap::inject_impulse);
    ClassDB::bind_method(
        D_METHOD("set_obstacle_reflectivity", "x", "y", "reflectivity"),
        &CymaticMap::set_obstacle_reflectivity
    );
    ClassDB::bind_method(D_METHOD("step_wave", "dt"), &CymaticMap::step_wave, DEFVAL(0.016f));
    ClassDB::bind_method(D_METHOD("evaluate_modal_field"), &CymaticMap::evaluate_modal_field);

    ClassDB::bind_method(D_METHOD("get_cymatic_field_bytes"), &CymaticMap::get_cymatic_field_bytes);
    ClassDB::bind_method(D_METHOD("get_field_values"), &CymaticMap::get_field_values);
    ClassDB::bind_method(D_METHOD("compute_shadow_zones", "source_grid_pos"), &CymaticMap::compute_shadow_zones);
    ClassDB::bind_method(
        D_METHOD("reduce_directional_sectors", "listener_grid_pos", "reference_yaw_rad", "sector_count"),
        &CymaticMap::reduce_directional_sectors,
        DEFVAL(0.0f),
        DEFVAL(16)
    );
}

CymaticMap::CymaticMap() {
    _allocate_buffers();
    evaluate_modal_field();
    set_process(auto_step_in_process_);
}

void CymaticMap::_allocate_buffers() {
    config_.width  = std::max<uint32_t>(3u, config_.width);
    config_.height = std::max<uint32_t>(3u, config_.height);
    config_.depth  = std::max<uint32_t>(1u, config_.depth);

    const size_t n2d = static_cast<size_t>(config_.width) * config_.height;
    const size_t n3d = n2d * config_.depth;
    wave_prev_.assign(n3d, 0.0f);
    wave_curr_.assign(n3d, 0.0f);
    wave_next_.assign(n3d, 0.0f);
    modal_field_.assign(n2d, 0.0f);
    reflectivity_.assign(n3d, 0.0f);
    shadow_classes_.assign(n2d, 0u);
}

void CymaticMap::_process(double p_delta) {
    if (!auto_step_in_process_) {
        return;
    }
    step_wave(static_cast<float>(p_delta));
}

void CymaticMap::set_grid_width(int p_w) {
    config_.width = static_cast<uint32_t>(std::clamp(p_w, 3, 256));
    _allocate_buffers();
}

void CymaticMap::set_grid_height(int p_h) {
    config_.height = static_cast<uint32_t>(std::clamp(p_h, 3, 256));
    _allocate_buffers();
}

void CymaticMap::set_grid_depth(int p_d) {
    config_.depth = static_cast<uint32_t>(std::clamp(p_d, 1, 64));
    _allocate_buffers();
}

void CymaticMap::set_auto_step_in_process(bool p_auto) {
    auto_step_in_process_ = p_auto;
    set_process(p_auto);
}

void CymaticMap::inject_impulse(int p_x, int p_y, float p_amplitude) {
    if (p_x >= 0 && p_y >= 0 &&
        static_cast<uint32_t>(p_x) < config_.width &&
        static_cast<uint32_t>(p_y) < config_.height) {
        wave_curr_[static_cast<size_t>(p_y) * config_.width + static_cast<size_t>(p_x)] += p_amplitude;
    }
}

void CymaticMap::set_obstacle_reflectivity(int p_x, int p_y, float p_reflectivity) {
    if (p_x >= 0 && p_y >= 0 &&
        static_cast<uint32_t>(p_x) < config_.width &&
        static_cast<uint32_t>(p_y) < config_.height) {
        reflectivity_[static_cast<size_t>(p_y) * config_.width + static_cast<size_t>(p_x)] =
            std::clamp(p_reflectivity, 0.0f, 1.0f);
    }
}

void CymaticMap::step_wave(float p_dt) {
    config_.dt = std::clamp(p_dt, 0.001f, 0.05f);
    sim_time_sec_ += config_.dt;

    if (config_.wave_mode == static_cast<uint8_t>(core::CymaticWaveMode::MODE_FDTD_WAVE) ||
        config_.wave_mode == static_cast<uint8_t>(core::CymaticWaveMode::MODE_HYBRID)) {
        if (config_.depth > 1u) {
            core::CymaticsSolverCore::step_fdtd_3d(
                wave_prev_.data(), wave_curr_.data(), wave_next_.data(), reflectivity_.data(), config_
            );
        } else {
            core::CymaticsSolverCore::step_fdtd_2d(
                wave_prev_.data(), wave_curr_.data(), wave_next_.data(), reflectivity_.data(), config_
            );
        }
        wave_prev_.swap(wave_curr_);
        wave_curr_.swap(wave_next_);
    }

    if (config_.wave_mode != static_cast<uint8_t>(core::CymaticWaveMode::MODE_FDTD_WAVE)) {
        evaluate_modal_field();
    } else {
        emit_signal("field_updated");
    }
}

void CymaticMap::evaluate_modal_field() {
    const float *fft_ptr = fft_peaks_.is_empty() ? nullptr : fft_peaks_.ptr();
    const uint32_t count = static_cast<uint32_t>(fft_peaks_.size());
    core::CymaticsSolverCore::evaluate_chladni_modal_field(
        modal_field_.data(), config_, fft_ptr, count, sim_time_sec_
    );
    emit_signal("field_updated");
}

godot::PackedByteArray CymaticMap::get_cymatic_field_bytes() const {
    const size_t n2d = static_cast<size_t>(config_.width) * config_.height;
    const float *src = (config_.wave_mode == static_cast<uint8_t>(core::CymaticWaveMode::MODE_FDTD_WAVE))
        ? wave_curr_.data()
        : modal_field_.data();

    godot::PackedByteArray bytes;
    bytes.resize(static_cast<int64_t>(n2d * sizeof(float)));
    if (n2d > 0u && src) {
        std::memcpy(bytes.ptrw(), src, n2d * sizeof(float));
    }
    return bytes;
}

godot::PackedFloat32Array CymaticMap::get_field_values() const {
    const size_t n2d = static_cast<size_t>(config_.width) * config_.height;
    const float *src = (config_.wave_mode == static_cast<uint8_t>(core::CymaticWaveMode::MODE_FDTD_WAVE))
        ? wave_curr_.data()
        : modal_field_.data();

    godot::PackedFloat32Array out;
    out.resize(static_cast<int64_t>(n2d));
    if (n2d > 0u && src) {
        std::memcpy(out.ptrw(), src, n2d * sizeof(float));
    }
    return out;
}

godot::PackedInt32Array CymaticMap::compute_shadow_zones(const godot::Vector2 &p_source_grid_pos) {
    const float *src = (config_.wave_mode == static_cast<uint8_t>(core::CymaticWaveMode::MODE_FDTD_WAVE))
        ? wave_curr_.data()
        : modal_field_.data();

    core::CymaticsSolverCore::classify_shadow_zones(
        src, reflectivity_.data(), shadow_classes_.data(), config_, p_source_grid_pos.x, p_source_grid_pos.y
    );

    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(shadow_classes_.size()));
    for (size_t i = 0; i < shadow_classes_.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(shadow_classes_[i]));
    }
    return out;
}

godot::Array CymaticMap::reduce_directional_sectors(
    const godot::Vector2 &p_listener_grid_pos,
    float p_reference_yaw_rad,
    int p_sector_count
) const {
    const uint32_t k = static_cast<uint32_t>(std::clamp(p_sector_count, 4, 64));
    std::vector<core::CymaticSectorPOD> sectors(k);
    const float *src = (config_.wave_mode == static_cast<uint8_t>(core::CymaticWaveMode::MODE_FDTD_WAVE))
        ? wave_curr_.data()
        : modal_field_.data();

    core::CymaticsSolverCore::reduce_directional_sectors(
        src, config_, p_listener_grid_pos.x, p_listener_grid_pos.y, p_reference_yaw_rad, sectors.data(), k
    );

    godot::Array out;
    for (const auto &s : sectors) {
        godot::Dictionary d;
        d["energy"]            = s.energy;
        d["peak_hold"]         = s.peak_hold;
        d["dominant_category"] = s.dominant_category;
        d["azimuth_rad"]       = s.azimuth_rad;
        out.append(d);
    }
    return out;
}

} // namespace ideam::godot_ext
