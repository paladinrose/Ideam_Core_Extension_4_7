#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * AudioPeakAndOnsetTransientQueryLogic<T, T_Strategy, T_View> (A10)
 * Evaluates positive spectral flux / envelope onset transients (cur - prev >= onset_threshold)
 * and absolute peak crossings across DOD audio spectrum or waveform buffers, culling or adding
 * reactive indices in r_selection.
 */
template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SwapView
>
struct alignas(64) AudioPeakAndOnsetTransientQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities =
        ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS | ViewCapability::SWAP_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA |
        BufferLayoutType::SPARSE_SET | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types =
        DataType::FLOAT32 | DataType::VECTOR2;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Audio Peak & Spectral Onset Transient Query";
    static constexpr std::string_view ui_category  = "Audio/Query";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id    = INVALID_ID; // 4B: Current spectrum/waveform buffer
    uint32_t envelope_buffer_id  = INVALID_ID; // 4B: Optional envelope follower buffer
    float peak_threshold         = 0.35f;      // 4B
    float onset_flux_threshold   = 0.12f;      // 4B -> 16B
    float envelope_decay         = 0.85f;      // 4B
    bool require_onset_rise      = true;       // 1B
    uint8_t reserved_padding[43] = {0};        // 43B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("envelope_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("peak_threshold", godot::Variant::FLOAT, 0.35f);
        add_prop("onset_flux_threshold", godot::Variant::FLOAT, 0.12f);
        add_prop("envelope_decay", godot::Variant::FLOAT, 0.85f);
        add_prop("require_onset_rise", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("envelope_buffer_id")) envelope_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["envelope_buffer_id"]));
        if (p_props.has("peak_threshold")) peak_threshold = std::max(0.0f, static_cast<float>(p_props["peak_threshold"]));
        if (p_props.has("onset_flux_threshold")) onset_flux_threshold = std::max(0.0f, static_cast<float>(p_props["onset_flux_threshold"]));
        if (p_props.has("envelope_decay")) envelope_decay = std::clamp(static_cast<float>(p_props["envelope_decay"]), 0.0f, 0.999f);
        if (p_props.has("require_onset_rise")) require_onset_rise = static_cast<bool>(p_props["require_onset_rise"]);
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        const T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* cur_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!cur_part || !cur_part->raw_base_ptr) return;

            GrantPartPOD* env_part = (envelope_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(envelope_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr) ? &p_ctx.grant->parts[1] : nullptr);

            const bool is_stereo = (cur_part->element_stride >= sizeof(godot::Vector2));
            const uint32_t stride = std::max<uint32_t>(is_stereo ? sizeof(godot::Vector2) : sizeof(float), cur_part->element_stride);
            const int64_t n_elems = std::min<int64_t>(r_selection.capacity, static_cast<int64_t>(cur_part->capacity_bytes / stride));

            float* env_f = (env_part && env_part->raw_base_ptr)
                ? reinterpret_cast<float*>(env_part->raw_base_ptr)
                : nullptr;
            const int64_t max_env = env_f ? static_cast<int64_t>(env_part->capacity_bytes / sizeof(float)) : 0;

            for (int64_t i = 0; i < n_elems; ++i) {
                float amp = 0.0f;
                if (is_stereo) {
                    const godot::Vector2& v2 = *reinterpret_cast<const godot::Vector2*>(cur_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                    amp = std::max(std::abs(v2.x), std::abs(v2.y));
                } else {
                    amp = std::abs(*reinterpret_cast<const float*>(cur_part->raw_base_ptr + static_cast<size_t>(i) * stride));
                }

                bool triggered = (amp >= peak_threshold);
                if (env_f && i < max_env) {
                    const float prev_env = env_f[i];
                    const float flux = amp - prev_env;
                    if (require_onset_rise) {
                        triggered = triggered && (flux >= onset_flux_threshold);
                    } else {
                        triggered = triggered || (flux >= onset_flux_threshold);
                    }
                    env_f[i] = std::max(amp, prev_env * envelope_decay);
                }

                const uint64_t word = static_cast<uint64_t>(i) >> 6;
                const uint64_t bit = 1ULL << (i & 63);

                if constexpr (Op == QueryOp::CULL) {
                    if (!triggered && r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                        if (r_selection.data.bitset[word] & bit) {
                            r_selection.data.bitset[word] &= ~bit;
                            r_selection.element_count = std::max<int64_t>(0, r_selection.element_count - 1);
                        }
                    }
                } else if constexpr (Op == QueryOp::ADD) {
                    if (triggered) {
                        p_ctx.queue_selection_command(target_buffer_id, i);
                        if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                            if (!(r_selection.data.bitset[word] & bit)) {
                                r_selection.data.bitset[word] |= bit;
                                r_selection.element_count++;
                            }
                        }
                    }
                }
            }
        }
    }
};

static_assert(sizeof(AudioPeakAndOnsetTransientQueryLogic<>) == 64, "AudioPeakAndOnsetTransientQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<AudioPeakAndOnsetTransientQueryLogic<>>, "AudioPeakAndOnsetTransientQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core