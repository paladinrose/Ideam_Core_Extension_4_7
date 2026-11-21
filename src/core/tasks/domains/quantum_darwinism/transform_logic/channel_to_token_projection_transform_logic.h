#pragma once

/**
 * channel_to_token_projection_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`QD5`) for `IdeamQuantumDarwinism`.
 *
 * Projects settled multi-channel `SlotChannelStatePOD` values/masks back onto
 * `EnvironmentSlotPOD::domain` (`TokenDescriptorPOD` reflection + seme matching),
 * updates `einselection_score`, and refreshes `POINTER_OR_CONFLICT_MASK`.
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../../quantum_darwinism/quantum_darwinism_common.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) ChannelToTokenProjectionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32;

    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "QD Channel-to-Token Reflection Projection";
    static constexpr std::string_view ui_category  = "QuantumDarwinism";

    uint32_t target_buffer_id           = INVALID_ID; ///< `ENVIRONMENT_SLOT_POD`
    uint32_t token_buffer_id            = INVALID_ID; ///< `TOKEN_DESCRIPTOR_POD`
    uint32_t channel_spec_buffer_id     = INVALID_ID; ///< `PROPERTY_CHANNEL_SPEC_POD`
    uint32_t slot_channel_buffer_id     = INVALID_ID; ///< `SLOT_CHANNEL_STATE_POD`
    uint32_t pointer_conflict_buffer_id = INVALID_ID; ///< `POINTER_OR_CONFLICT_MASK`
    float    pointer_redundancy_thresh  = 0.75f;
    uint8_t  collapse_to_best_token     = 1;
    uint8_t  reserved_padding[39]       = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",           godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("token_buffer_id",            godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("channel_spec_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("slot_channel_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_conflict_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_redundancy_thresh",  godot::Variant::FLOAT, 0.75);
        add_prop("collapse_to_best_token",     godot::Variant::BOOL,  true);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))           target_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("token_buffer_id"))            token_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["token_buffer_id"]));
        if (p_props.has("channel_spec_buffer_id"))     channel_spec_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["channel_spec_buffer_id"]));
        if (p_props.has("slot_channel_buffer_id"))     slot_channel_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["slot_channel_buffer_id"]));
        if (p_props.has("pointer_conflict_buffer_id")) pointer_conflict_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_conflict_buffer_id"]));
        if (p_props.has("pointer_redundancy_thresh"))  pointer_redundancy_thresh  = static_cast<float>(static_cast<double>(p_props["pointer_redundancy_thresh"]));
        if (p_props.has("collapse_to_best_token"))     collapse_to_best_token     = static_cast<bool>(p_props["collapse_to_best_token"]) ? 1u : 0u;
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_Elem>
    [[nodiscard]] static T_Elem *_resolve_ptr(const TaskContextPOD &p_ctx, uint32_t p_buf_id, size_t &r_cap) noexcept {
        r_cap = 0u;
        if (p_buf_id == INVALID_ID) return nullptr;
        if (const GrantPartPOD *part = p_ctx.get_grant_part(p_buf_id)) {
            if (part->raw_base_ptr) {
                const size_t cap = part->capacity_bytes / sizeof(T_Elem);
                r_cap = (part->selection.element_count > 0)
                    ? std::min<size_t>(static_cast<size_t>(part->selection.element_count), cap)
                    : cap;
                return reinterpret_cast<T_Elem *>(part->raw_base_ptr);
            }
        }
        if (p_ctx.manager) {
            if (MemoryBufferPOD *buf = p_ctx.manager->get_buffer(p_buf_id)) {
                if (buf->master_block_ptr) {
                    const size_t cap = static_cast<size_t>(std::max<int64_t>(0, buf->max_elements));
                    r_cap = (buf->current_count > 0)
                        ? std::min<size_t>(static_cast<size_t>(buf->current_count), cap)
                        : cap;
                    return reinterpret_cast<T_Elem *>(buf->master_block_ptr + buf->memory_offset);
                }
            }
        }
        return nullptr;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t slot_cap = 0, tok_cap = 0, spec_cap = 0, state_cap = 0, status_cap = 0;
        auto *slots        = _resolve_ptr<EnvironmentSlotPOD>(p_ctx, target_buffer_id, slot_cap);
        const auto *tokens = _resolve_ptr<const TokenDescriptorPOD>(p_ctx, token_buffer_id, tok_cap);
        const auto *specs  = _resolve_ptr<const PropertyChannelSpecPOD>(p_ctx, channel_spec_buffer_id, spec_cap);
        const auto *states = _resolve_ptr<const SlotChannelStatePOD>(p_ctx, slot_channel_buffer_id, state_cap);
        auto *status_masks = _resolve_ptr<uint32_t>(p_ctx, pointer_conflict_buffer_id, status_cap);

        if (!slots || !specs || !states || slot_cap == 0u || spec_cap == 0u) return;

        const uint32_t n_slots    = static_cast<uint32_t>(slot_cap);
        const uint32_t n_tokens   = static_cast<uint32_t>(std::min<size_t>(256u, tok_cap));
        const uint32_t n_channels = static_cast<uint32_t>(spec_cap);
        if (state_cap < static_cast<size_t>(n_slots) * n_channels) return;

        for (uint32_t s = 0u; s < n_slots; ++s) {
            EnvironmentSlotPOD &slot = slots[s];
            float mean_r = 0.0f;
            for (uint32_t c = 0u; c < n_channels; ++c) {
                mean_r += states[static_cast<size_t>(s) * n_channels + c].redundancy_r_delta;
            }
            mean_r /= static_cast<float>(n_channels);
            slot.einselection_score = std::clamp(mean_r, 0.0f, 1.0f);
            slot.is_pointer_state   = (slot.einselection_score >= pointer_redundancy_thresh) ? 1u : 0u;

            if (collapse_to_best_token != 0u && tokens && n_tokens > 0u &&
                !(slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE && slot.domain.popcount() == 1u)) {
                BitmaskDomain256POD cand = slot.domain;
                if (cand.empty()) cand.fill_first_n(n_tokens);

                int32_t best_tok = cand.first_set_state();
                float best_sc    = -1e30f;

                for (uint32_t t = 0u; t < n_tokens; ++t) {
                    if (!cand.test_bit(t)) continue;
                    const TokenDescriptorPOD &tok = tokens[t];
                    float sc = std::max(0.01f, tok.base_frequency_weight) * 0.1f;

                    for (uint32_t c = 0u; c < n_channels; ++c) {
                        const PropertyChannelSpecPOD &spec = specs[c];
                        const SlotChannelStatePOD &st = states[static_cast<size_t>(s) * n_channels + c];
                        if (st.redundancy_r_delta <= 1e-4f) continue;
                        const float w = st.redundancy_r_delta * (st.is_locked ? 1.5f : 0.75f);

                        if (spec.is_discrete_mask != 0u) {
                            uint32_t tb = 0u;
                            switch (spec.aspect) {
                                case QDChannelAspect::ASPECT_GROUP_OR_TAG:       tb = tok.signature.group_or_tag_mask; break;
                                case QDChannelAspect::ASPECT_NUCLEAR_SEME:       tb = tok.nuclear_seme_mask_lo32; break;
                                case QDChannelAspect::ASPECT_PERIPHERAL_SEME:    tb = tok.peripheral_seme_mask_lo32; break;
                                case QDChannelAspect::ASPECT_METHOD_TRAIT:       tb = tok.signature.method_trait_mask; break;
                                case QDChannelAspect::ASPECT_SIGNAL_TRAIT:       tb = tok.signature.signal_trait_mask; break;
                                case QDChannelAspect::ASPECT_REFLECTED_PROPERTY: tb = tok.signature.property_trait_mask; break;
                                default:                                         tb = (t < 32u) ? (1u << t) : 0u; break;
                            }
                            if ((tb & st.candidate_state_mask) != 0u) {
                                sc += w * static_cast<float>(std::popcount(tb & st.candidate_state_mask));
                            }
                        } else {
                            const float denom   = static_cast<float>(std::max<uint32_t>(1u, n_tokens - 1u));
                            const float t_norm  = static_cast<float>(t) / denom;
                            const float c_norm  = (st.value - spec.value_min) / std::max(1e-5f, spec.value_max - spec.value_min);
                            sc += w * (1.0f - std::abs(t_norm - c_norm));
                        }
                    }

                    if (sc > best_sc) {
                        best_sc  = sc;
                        best_tok = static_cast<int32_t>(t);
                    }
                }

                if (best_tok >= 0) {
                    slot.domain.clear();
                    slot.domain.set_bit(static_cast<uint32_t>(best_tok));
                    slot.collapsed_token_id = best_tok;
                    slot.has_conflict       = 0u;
                    slot.entropy_bucket     = 0u;
                }
            }

            if (status_masks && s < status_cap) {
                uint32_t flags = SLOT_STATUS_CLEAN;
                if (slot.is_pointer_state != 0u) flags |= SLOT_STATUS_POINTER_STATE;
                if (slot.has_conflict != 0u)     flags |= SLOT_STATUS_HAS_CONFLICT;
                if (slot.domain.popcount() == 1u) flags |= SLOT_STATUS_COLLAPSED;
                if (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE) flags |= SLOT_STATUS_PINNED;
                status_masks[s] = flags;
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(ChannelToTokenProjectionTransformLogic<>) == 64, "ChannelToTokenProjectionTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<ChannelToTokenProjectionTransformLogic<>>, "ChannelToTokenProjectionTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
