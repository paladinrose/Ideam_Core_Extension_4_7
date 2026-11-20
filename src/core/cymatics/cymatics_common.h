#pragma once

/**
 * cymatics_common.h
 * Pure C++26 PODs and enums for `IdeamCymatics` (`src/core/cymatics/`).
 */

#include <cstdint>

namespace ideam::core {

enum class CymaticWaveMode : uint8_t {
    MODE_FDTD_WAVE       = 0,
    MODE_CHLADNI_PLATE   = 1,
    MODE_BESSEL_CIRCULAR = 2,
    MODE_HYBRID          = 3
};

enum class AcousticShadowClass : uint8_t {
    DIRECT_LOS          = 0,
    DIFFRACTED_RIM      = 1,
    REVERBERANT_CHAMBER = 2,
    ACOUSTIC_SHADOW     = 3
};

enum class JukeboxLoopMode : int32_t {
    LOOP_ALL    = 0,
    LOOP_SINGLE = 1,
    LOOP_NONE   = 2
};

struct alignas(64) CymaticGridConfigPOD {
    uint32_t width       = 32;
    uint32_t height      = 32;
    uint32_t depth       = 1;
    float cell_size      = 1.0f;
    float wave_speed     = 1.0f;
    float damping        = 0.985f;
    float dt             = 0.016f;
    float chladni_m      = 3.0f;
    float chladni_n      = 5.0f;
    float chladni_a      = 1.0f;
    float chladni_b      = -1.0f;
    float bessel_order   = 2.0f;
    float bessel_root    = 5.1356f; // First non-trivial root of J_2
    uint8_t wave_mode    = static_cast<uint8_t>(CymaticWaveMode::MODE_HYBRID);
    uint8_t reserved[11] = {0};
};
static_assert(sizeof(CymaticGridConfigPOD) == 64, "CymaticGridConfigPOD must be 64 bytes!");

struct alignas(32) AcousticSourcePOD {
    float pos_x        = 0.0f;
    float pos_y        = 0.0f;
    float pos_z        = 0.0f;
    float frequency_hz = 440.0f;
    float amplitude    = 1.0f;
    float phase        = 0.0f;
    int32_t category   = 0;
    uint32_t active    = 1u;
};

struct alignas(16) CymaticSectorPOD {
    float energy              = 0.0f;
    float peak_hold           = 0.0f;
    int32_t dominant_category = 0;
    float azimuth_rad         = 0.0f;
};

} // namespace ideam::core
