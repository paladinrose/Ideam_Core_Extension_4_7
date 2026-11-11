#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * A10: AudioPeakAndOnsetTransientQueryLogic
 * Evaluates positive spectral flux / envelope onset transients (cur - prev >= onset_threshold)
 * and absolute peak crossings across DOD audio spectrum or waveform buffers, waking or sleeping
 * reactive indices in r_selection.
 */
struct AudioPeakAndOnsetTransientQueryLogic {
    static constexpr std::string_view ui_name = "Audio Peak & Spectral Onset Transient Query";
    static constexpr std::string_view ui_category = "Audio/Query";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SWAP,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::FLOAT32,
        DataType::VECTOR2
    };
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::WAKE,
        QueryOp::SLEEP,
        QueryOp::INVERT
    };

    float peak_threshold = 0.35f;
    float onset_flux_threshold = 0.12f;
    float envelope_decay = 0.85f;
    bool require_onset_rise = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("peak_threshold", godot::Variant::FLOAT, peak_threshold);
        add_prop("onset_flux_threshold", godot::Variant::FLOAT, onset_flux_threshold);
        add_prop("envelope_decay", godot::Variant::FLOAT, envelope_decay);
        add_prop("require_onset_rise", godot::Variant::BOOL, require_onset_rise);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("peak_threshold")) peak_threshold = std::max(0.0f, static_cast<float>(p_props["peak_threshold"]));
        if (p_props.has("onset_flux_threshold")) onset_flux_threshold = std::max(0.0f, static_cast<float>(p_props["onset_flux_threshold"]));
        if (p_props.has("envelope_decay")) envelope_decay = std::clamp(static_cast<float>(p_props["envelope_decay"]), 0.0f, 0.999f);
        if (p_props.has("require_onset_rise")) require_onset_rise = static_cast<bool>(p_props["require_onset_rise"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline void apply_selection_op(MemoryBufferSelectionPOD& r_sel, int64_t idx, QueryOp p_op) noexcept {
        if (idx < 0 || idx >= r_sel.capacity) return;
        if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
            const uint64_t word = static_cast<uint64_t>(idx) >> 6;
            const uint64_t bit = 1ULL << (idx & 63);
            const bool was_set = (r_sel.data.bitset[word] & bit) != 0;
            if (p_op == QueryOp::WAKE && !was_set) {
                r_sel.data.bitset[word] |= bit;
                r_sel.element_count++;
            } else if (p_op == QueryOp::SLEEP && was_set) {
                r_sel.data.bitset[word] &= ~bit;
                r_sel.element_count = std::max<int64_t>(0, r_sel.element_count - 1);
            } else if (p_op == QueryOp::INVERT) {
                r_sel.data.bitset[word] ^= bit;
                r_sel.element_count += was_set ? -1 : 1;
            }
        } else if (r_sel.mode == SelectionMode::SPARSE && r_sel.data.indices) {
            if (p_op == QueryOp::WAKE && r_sel.element_count < r_sel.capacity) {
                r_sel.data.indices[r_sel.element_count++] = idx;
            }
        }
    }

    void execute_with_op(const TaskContextPOD& p_ctx, QueryOp p_op) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& cur_part = p_ctx.grant->parts[0];
        if (!cur_part.raw_base_ptr) return;

        GrantPartPOD* env_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        MemoryBufferSelectionPOD& sel = cur_part.selection;
        const bool is_stereo = (cur_part.element_stride >= sizeof(godot::Vector2));
        const uint32_t stride = std::max<uint32_t>(is_stereo ? sizeof(godot::Vector2) : sizeof(float), cur_part.element_stride);
        const int64_t n_elems = static_cast<int64_t>(cur_part.capacity_bytes / stride);

        float* env_f = env_part ? reinterpret_cast<float*>(env_part->raw_base_ptr) : nullptr;
        const int64_t max_env = env_part ? static_cast<int64_t>(env_part->capacity_bytes / sizeof(float)) : 0;

        for (int64_t i = 0; i < n_elems; ++i) {
            float amp = 0.0f;
            if (is_stereo) {
                const godot::Vector2& v2 = *reinterpret_cast<const godot::Vector2*>(cur_part.raw_base_ptr + static_cast<size_t>(i) * stride);
                amp = std::max(std::abs(v2.x), std::abs(v2.y));
            } else {
                amp = std::abs(*reinterpret_cast<const float*>(cur_part.raw_base_ptr + static_cast<size_t>(i) * stride));
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

            if (triggered) {
                apply_selection_op(sel, i, p_op);
            }
        }
    }

    void execute(const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, QueryOp p_op, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, p_op);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }
};

} // namespace ideam::core