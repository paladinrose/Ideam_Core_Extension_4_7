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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ShaderParameterEpsilonDirtyQueryLogic<T, T_Strategy, T_View> (S7)
 * Compares current per-instance shader parameter values against last-committed shadow
 * values, culling or adding instances whose L2 change exceeds epsilon (and optionally updating
 * the shadow buffer on dirty) so downstream S3 tasks only touch modified instance RIDs.
 */
template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SwapView
>
struct alignas(64) ShaderParameterEpsilonDirtyQueryLogic {
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
        DataType::FLOAT32 | DataType::VECTOR2 | DataType::VECTOR3 | DataType::VECTOR4 | DataType::COLOR;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Shader Parameter Epsilon Dirty Query";
    static constexpr std::string_view ui_category  = "Shader/Query";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id    = INVALID_ID; // 4B: Current parameter value buffer
    uint32_t shadow_buffer_id    = INVALID_ID; // 4B: Last-committed shadow value buffer
    float epsilon                = 1e-4f;      // 4B
    bool commit_shadow_on_dirty  = true;       // 1B
    uint8_t reserved_padding[51] = {0};        // 51B -> Total = 64B

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
        add_buf("shadow_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("epsilon", godot::Variant::FLOAT, 1e-4f);
        add_prop("commit_shadow_on_dirty", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("shadow_buffer_id")) shadow_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["shadow_buffer_id"]));
        if (p_props.has("epsilon")) epsilon = std::max(0.0f, static_cast<float>(p_props["epsilon"]));
        if (p_props.has("commit_shadow_on_dirty")) commit_shadow_on_dirty = static_cast<bool>(p_props["commit_shadow_on_dirty"]);
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
            GrantPartPOD* prev_part = (shadow_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(shadow_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!cur_part || !cur_part->raw_base_ptr || !prev_part || !prev_part->raw_base_ptr) return;

            const uint32_t stride = std::max<uint32_t>(sizeof(float), cur_part->element_stride);
            const uint32_t floats_per_elem = std::max<uint32_t>(1, stride / sizeof(float));
            const int64_t n_elems = std::min<int64_t>(
                static_cast<int64_t>(cur_part->capacity_bytes / stride),
                static_cast<int64_t>(prev_part->capacity_bytes / stride)
            );
            const float eps2 = epsilon * epsilon;

            for (int64_t i = 0; i < n_elems && i < r_selection.capacity; ++i) {
                const float* cur_f = reinterpret_cast<const float*>(cur_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                float* prev_f = reinterpret_cast<float*>(prev_part->raw_base_ptr + static_cast<size_t>(i) * stride);

                float diff2 = 0.0f;
                for (uint32_t c = 0; c < floats_per_elem; ++c) {
                    const float d = cur_f[c] - prev_f[c];
                    diff2 += d * d;
                }

                const bool is_dirty = (diff2 > eps2);
                if (is_dirty && commit_shadow_on_dirty) {
                    std::memcpy(prev_f, cur_f, floats_per_elem * sizeof(float));
                }

                if constexpr (Op == QueryOp::CULL) {
                    if (!is_dirty && r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                        const uint64_t word = static_cast<uint64_t>(i) >> 6;
                        const uint64_t bit = 1ULL << (i & 63);
                        if (r_selection.data.bitset[word] & bit) {
                            r_selection.data.bitset[word] &= ~bit;
                            r_selection.element_count = std::max<int64_t>(0, r_selection.element_count - 1);
                        }
                    }
                } else if constexpr (Op == QueryOp::ADD) {
                    if (is_dirty) {
                        p_ctx.queue_selection_command(target_buffer_id, i);
                        if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                            const uint64_t word = static_cast<uint64_t>(i) >> 6;
                            const uint64_t bit = 1ULL << (i & 63);
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

static_assert(sizeof(ShaderParameterEpsilonDirtyQueryLogic<>) == 64, "ShaderParameterEpsilonDirtyQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<ShaderParameterEpsilonDirtyQueryLogic<>>, "ShaderParameterEpsilonDirtyQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core