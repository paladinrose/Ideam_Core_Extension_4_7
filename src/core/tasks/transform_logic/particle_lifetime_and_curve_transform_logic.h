#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/aosoa_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ParticleLifetimeAndCurveTransformLogic<T> (V2)
 * Advances particle Age += delta, computes normalized age t in [0, 1], and evaluates smooth
 * scale and color/opacity curves over the particle's lifetime.
 */
template <typename T = godot::Vector2>
struct alignas(64) ParticleLifetimeAndCurveTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = SoAStrategy;
    using DefaultView     = AOSOAView<T, 4, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::SOA | BufferLayoutType::TILED_SOA | BufferLayoutType::FLAT;
    static constexpr DataType required_types              = DataType::VECTOR2 | DataType::FLOAT32 | DataType::COLOR;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Particle Age Advance & Lifetime Curve Ramp";
    static constexpr std::string_view ui_category  = "Particles/Lifecycle";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t lifetime_buffer_id  = INVALID_ID; // 4B: Primary target Vector2(age, lifetime) or float(age) buffer
    uint32_t scale_buffer_id     = INVALID_ID; // 4B: Optional output float particle scale buffer
    uint32_t color_buffer_id     = INVALID_ID; // 4B: Optional output Color particle tint buffer
    float fallback_lifetime      = 2.0f;       // 4B: Fallback lifetime when lifetime_buffer_id is a scalar float -> 16B

    float scale_birth            = 0.25f;      // 4B: Scale at t = 0
    float scale_peak             = 1.0f;       // 4B: Scale at t = peak_normalized_time
    float scale_death            = 0.0f;       // 4B: Scale at t = 1
    float peak_normalized_time   = 0.25f;      // 4B: Normalized time in [0.01, 0.99] of peak scale -> 32B

    godot::Color color_birth{1.0f, 0.9f, 0.5f, 1.0f};  // 16B -> 48B
    godot::Color color_death{0.8f, 0.2f, 0.05f, 0.0f}; // 16B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary lbuf_prop;
        lbuf_prop["name"] = "lifetime_buffer_id";
        lbuf_prop["type"] = godot::Variant::INT;
        lbuf_prop["hint_string"] = "buffer_option";
        props.push_back(lbuf_prop);

        godot::Dictionary sbuf_prop;
        sbuf_prop["name"] = "scale_buffer_id";
        sbuf_prop["type"] = godot::Variant::INT;
        sbuf_prop["hint_string"] = "buffer_option";
        props.push_back(sbuf_prop);

        godot::Dictionary cbuf_prop;
        cbuf_prop["name"] = "color_buffer_id";
        cbuf_prop["type"] = godot::Variant::INT;
        cbuf_prop["hint_string"] = "buffer_option";
        props.push_back(cbuf_prop);

        godot::Dictionary fl_prop;
        fl_prop["name"] = "fallback_lifetime";
        fl_prop["type"] = godot::Variant::FLOAT;
        fl_prop["hint_string"] = "suffix:s";
        props.push_back(fl_prop);

        godot::Dictionary sb_prop;
        sb_prop["name"] = "scale_birth";
        sb_prop["type"] = godot::Variant::FLOAT;
        props.push_back(sb_prop);

        godot::Dictionary sp_prop;
        sp_prop["name"] = "scale_peak";
        sp_prop["type"] = godot::Variant::FLOAT;
        props.push_back(sp_prop);

        godot::Dictionary sd_prop;
        sd_prop["name"] = "scale_death";
        sd_prop["type"] = godot::Variant::FLOAT;
        props.push_back(sd_prop);

        godot::Dictionary pt_prop;
        pt_prop["name"] = "peak_normalized_time";
        pt_prop["type"] = godot::Variant::FLOAT;
        pt_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        pt_prop["hint_string"] = "0.01,0.99,0.01";
        props.push_back(pt_prop);

        godot::Dictionary cb_prop;
        cb_prop["name"] = "color_birth";
        cb_prop["type"] = godot::Variant::COLOR;
        props.push_back(cb_prop);

        godot::Dictionary cd_prop;
        cd_prop["name"] = "color_death";
        cd_prop["type"] = godot::Variant::COLOR;
        props.push_back(cd_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (lifetime_buffer_id != INVALID_ID) ? lifetime_buffer_id : scale_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("lifetime_buffer_id")) {
            lifetime_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["lifetime_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && lifetime_buffer_id == INVALID_ID) {
            lifetime_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && lifetime_buffer_id == INVALID_ID) {
            lifetime_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("scale_buffer_id")) {
            scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["scale_buffer_id"]));
        }
        if (p_props.has("color_buffer_id")) {
            color_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["color_buffer_id"]));
        }
        if (p_props.has("fallback_lifetime")) {
            fallback_lifetime = std::max(1e-3f, static_cast<float>(p_props["fallback_lifetime"]));
        }
        if (p_props.has("scale_birth")) {
            scale_birth = static_cast<float>(p_props["scale_birth"]);
        }
        if (p_props.has("scale_peak")) {
            scale_peak = static_cast<float>(p_props["scale_peak"]);
        }
        if (p_props.has("scale_death")) {
            scale_death = static_cast<float>(p_props["scale_death"]);
        }
        if (p_props.has("peak_normalized_time")) {
            peak_normalized_time = std::clamp(static_cast<float>(p_props["peak_normalized_time"]), 0.01f, 0.99f);
        }
        if (p_props.has("color_birth")) {
            color_birth = static_cast<godot::Color>(p_props["color_birth"]);
        }
        if (p_props.has("color_death")) {
            color_death = static_cast<godot::Color>(p_props["color_death"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, godot::Vector2> || std::is_same_v<T, float> || std::is_same_v<T, godot::Color>) {
            const GrantPartPOD* life_part = p_context.get_grant_part(lifetime_buffer_id);
            if (!life_part || !life_part->raw_base_ptr) return;

            const GrantPartPOD* scale_part = (scale_buffer_id != INVALID_ID) ? p_context.get_grant_part(scale_buffer_id) : nullptr;
            const GrantPartPOD* color_part = (color_buffer_id != INVALID_ID) ? p_context.get_grant_part(color_buffer_id) : nullptr;

            const bool is_packed_v2 = (life_part->element_stride >= sizeof(godot::Vector2));
            const uint32_t l_stride = std::max<uint32_t>(is_packed_v2 ? sizeof(godot::Vector2) : sizeof(float), life_part->element_stride);
            const int64_t n_particles = (life_part->selection.element_count > 0)
                ? life_part->selection.element_count
                : static_cast<int64_t>(life_part->capacity_bytes / l_stride);

            const float dt = (p_context.delta > 0.0) ? static_cast<float>(p_context.delta) : (1.0f / 60.0f);

            for (int64_t i = 0; i < n_particles; ++i) {
                float age = 0.0f;
                float lifetime = fallback_lifetime;

                if (is_packed_v2) {
                    godot::Vector2& al = *reinterpret_cast<godot::Vector2*>(life_part->raw_base_ptr + static_cast<size_t>(i) * l_stride);
                    al.x += dt;
                    age = al.x;
                    lifetime = std::max(1e-4f, al.y);
                } else {
                    float& a = *reinterpret_cast<float*>(life_part->raw_base_ptr + static_cast<size_t>(i) * l_stride);
                    a += dt;
                    age = a;
                }

                const float t = std::clamp(age / lifetime, 0.0f, 1.0f);

                if (scale_part && scale_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(float) <= scale_part->capacity_bytes) {
                    reinterpret_cast<float*>(scale_part->raw_base_ptr)[i] = _evaluate_scale_ramp(t);
                }
                if (color_part && color_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(godot::Color) <= color_part->capacity_bytes) {
                    reinterpret_cast<godot::Color*>(color_part->raw_base_ptr)[i] = color_birth.lerp(color_death, t);
                }
            }
        }
    }

private:
    inline float _evaluate_scale_ramp(float p_t) const noexcept {
        if (p_t <= peak_normalized_time) {
            const float u = p_t / peak_normalized_time;
            const float smooth = u * u * (3.0f - 2.0f * u);
            return scale_birth + (scale_peak - scale_birth) * smooth;
        }
        const float u = (p_t - peak_normalized_time) / (1.0f - peak_normalized_time);
        const float smooth = u * u * (3.0f - 2.0f * u);
        return scale_peak + (scale_death - scale_peak) * smooth;
    }
};

static_assert(sizeof(ParticleLifetimeAndCurveTransformLogic<>) == 64, "ParticleLifetimeAndCurveTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ParticleLifetimeAndCurveTransformLogic<>>, "ParticleLifetimeAndCurveTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core