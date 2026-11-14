#pragma once

/**
 * pointer_state_einselection_query_logic.h
 * 64-byte cache-aligned QueryLogic task struct (`QD2`) for `IdeamQuantumDarwinism`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 1.5 — STAGE 2)
 * ============================================================================
 * - Evaluates Einselection Redundancy $R_\delta(i)$ across all slots inside $\mathcal{E}_{\text{box}}$
 *   from:
 *     1. Mutual-information witness agreement across resolved parent, children, siblings,
 *        and spatial neighbors.
 *     2. Highest CSS `GenerationRulePOD::specificity` satisfied by the slot's token.
 *     3. Designer-pinned landmark status (`LAYER_PINNED_OVERRIDE`).
 * - Promotes perimeter anchors and interior slots with $R_\delta(i) \ge \tau_{\text{pointer}}$
 *   (`threshold`) to immutable **Pointer States** (`PointerStateRecordPOD`), sets
 *   `SLOT_STATUS_POINTER_STATE` in `POINTER_OR_CONFLICT_MASK`, and writes Pointer State
 *   slot indices into `target_part->selection` (`SelectionMode::SPARSE`).
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../../../../quantum_darwinism/quantum_darwinism_common.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) PointerStateEinselectionQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32;

    static constexpr bool supports_cull            = true;
    static constexpr bool supports_addition        = true;
    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "QD Pointer State Einselection & Redundancy Query";
    static constexpr std::string_view ui_category  = "QuantumDarwinism";

    uint32_t target_buffer_id           = INVALID_ID; ///< `ENVIRONMENT_SLOT_POD` (`EnvironmentSlotPOD`)
    uint32_t token_buffer_id            = INVALID_ID; ///< `TOKEN_DESCRIPTOR_POD` (`TokenDescriptorPOD`)
    uint32_t rule_buffer_id             = INVALID_ID; ///< `CASCADE_RULE_POD` (`GenerationRulePOD`)
    uint32_t child_csr_buffer_id        = INVALID_ID; ///< `CHILD_SLOT_CSR` (`uint32_t`)
    uint32_t horizon_partition_buf_id   = INVALID_ID; ///< `HORIZON_PARTITION_POD` (`HorizonSlotPartitionPOD`)
    uint32_t pointer_record_buffer_id   = INVALID_ID; ///< `POINTER_STATE_RECORD_POD` (`PointerStateRecordPOD`)
    uint32_t pointer_conflict_buffer_id = INVALID_ID; ///< `POINTER_OR_CONFLICT_MASK` (`uint32_t`)
    float    threshold                  = 0.75f;      ///< $\tau_{\text{pointer}}$ redundancy threshold
    uint8_t  promote_perimeter_anchors  = 1;
    uint8_t  reserved_padding[31]       = {0};

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
        add_prop("rule_buffer_id",             godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("child_csr_buffer_id",        godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("horizon_partition_buf_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_record_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_conflict_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("threshold",                  godot::Variant::FLOAT, 0.75);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))           target_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("token_buffer_id"))            token_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["token_buffer_id"]));
        if (p_props.has("rule_buffer_id"))             rule_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_buffer_id"]));
        if (p_props.has("child_csr_buffer_id"))        child_csr_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["child_csr_buffer_id"]));
        if (p_props.has("horizon_partition_buf_id"))   horizon_partition_buf_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["horizon_partition_buf_id"]));
        if (p_props.has("pointer_record_buffer_id"))   pointer_record_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_record_buffer_id"]));
        if (p_props.has("pointer_conflict_buffer_id")) pointer_conflict_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_conflict_buffer_id"]));
        if (p_props.has("threshold"))                  threshold                  = static_cast<float>(static_cast<double>(p_props["threshold"]));
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

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t slot_cap = 0, tok_cap = 0, rule_cap = 0, child_cap = 0, part_cap = 0, rec_cap = 0, status_cap = 0;
        auto *slots            = _resolve_ptr<EnvironmentSlotPOD>(p_ctx, target_buffer_id, slot_cap);
        const auto *tokens     = _resolve_ptr<const TokenDescriptorPOD>(p_ctx, token_buffer_id, tok_cap);
        const auto *rules      = _resolve_ptr<const GenerationRulePOD>(p_ctx, rule_buffer_id, rule_cap);
        const auto *child_csr  = _resolve_ptr<const uint32_t>(p_ctx, child_csr_buffer_id, child_cap);
        auto *partitions       = _resolve_ptr<HorizonSlotPartitionPOD>(p_ctx, horizon_partition_buf_id, part_cap);
        auto *pointer_records  = _resolve_ptr<PointerStateRecordPOD>(p_ctx, pointer_record_buffer_id, rec_cap);
        auto *status_masks     = _resolve_ptr<uint32_t>(p_ctx, pointer_conflict_buffer_id, status_cap);

        if (!slots || slot_cap == 0) {
            return;
        }

        const uint32_t n_slots     = static_cast<uint32_t>(slot_cap);
        const uint32_t n_tokens    = static_cast<uint32_t>(tok_cap);
        const uint32_t n_rules     = static_cast<uint32_t>(rule_cap);
        const uint32_t n_child_csr = static_cast<uint32_t>(child_cap);

        GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(target_buffer_id));
        int64_t pointer_count     = 0;

        for (uint32_t i = 0; i < n_slots; ++i) {
            EnvironmentSlotPOD &slot = slots[i];
            if (pointer_records && i < rec_cap) {
                pointer_records[i] = PointerStateRecordPOD{};
            }

            if (slot.has_conflict != 0u || slot.domain.popcount() != 1u) {
                slot.is_pointer_state = 0u;
                if (status_masks && i < status_cap) {
                    status_masks[i] &= ~SLOT_STATUS_POINTER_STATE;
                }
                continue;
            }

            const int32_t tok_id    = slot.domain.first_set_state();
            slot.collapsed_token_id = tok_id;

            // Compute highest satisfied CSS rule specificity
            uint32_t max_spec = 0u;
            uint32_t preserved_traits = 0u;
            if (tokens && tok_id >= 0 && static_cast<uint32_t>(tok_id) < n_tokens) {
                const TokenDescriptorPOD &tok = tokens[tok_id];
                preserved_traits = tok.coherence_trait_mask | tok.nuclear_seme_mask_lo32;
                if (rules) {
                    for (uint32_t r = 0; r < n_rules; ++r) {
                        if (rules[r].selector.matches_candidate(tok.signature)) {
                            max_spec = std::max<uint32_t>(max_spec, rules[r].specificity);
                        }
                    }
                }
            }

            // Compute witness agreement across parent and children
            uint32_t witnesses = 1u;
            uint32_t agreed    = 1u;
            if (slot.parent_slot_id < n_slots) {
                ++witnesses;
                if (slots[slot.parent_slot_id].has_conflict == 0u &&
                    slots[slot.parent_slot_id].domain.popcount() == 1u) {
                    ++agreed;
                }
            }
            if (child_csr && slot.children_count > 0u &&
                (slot.children_csr_offset + slot.children_count) <= n_child_csr) {
                for (uint32_t k = 0; k < slot.children_count; ++k) {
                    const uint32_t c_id = child_csr[slot.children_csr_offset + k];
                    if (c_id < n_slots) {
                        ++witnesses;
                        if (slots[c_id].has_conflict == 0u && slots[c_id].domain.popcount() == 1u) {
                            ++agreed;
                        }
                    }
                }
            }

            const float witness_ratio = static_cast<float>(agreed) / static_cast<float>(witnesses);
            const float spec_norm     = std::clamp(static_cast<float>(max_spec) / 1500.0f, 0.1f, 1.0f);
            const float pinned_bonus  = (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE) ? 0.35f : 0.0f;
            slot.einselection_score   = std::clamp(0.55f * witness_ratio + 0.45f * spec_norm + pinned_bonus, 0.0f, 1.0f);

            const bool in_horizon = (!partitions || i >= part_cap)
                ? true
                : (partitions[i].hop_distance != 0xFFFFu && partitions[i].hop_distance > 0u);
            const bool is_perimeter = (partitions && i < part_cap) &&
                ((partitions[i].role_flags & HORIZON_ROLE_PERIMETER) != 0u) &&
                (promote_perimeter_anchors != 0u);
            const bool is_pinned = (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE);

            const bool selected_as_pointer = in_horizon &&
                (is_pinned || is_perimeter || slot.einselection_score >= threshold);

            slot.is_pointer_state = selected_as_pointer ? 1u : 0u;
            if (partitions && i < part_cap && selected_as_pointer) {
                partitions[i].role_flags |= HORIZON_ROLE_POINTER_STATE;
            }
            if (status_masks && i < status_cap) {
                if (selected_as_pointer) {
                    status_masks[i] |= SLOT_STATUS_POINTER_STATE;
                } else {
                    status_masks[i] &= ~SLOT_STATUS_POINTER_STATE;
                }
            }

            if (selected_as_pointer) {
                if (pointer_records && static_cast<size_t>(pointer_count) < rec_cap) {
                    PointerStateRecordPOD rec{};
                    rec.slot_id                  = i;
                    rec.settled_token_id         = static_cast<uint32_t>(std::max(0, tok_id));
                    rec.einselection_redundancy  = slot.einselection_score;
                    rec.highest_satisfied_spec   = max_spec;
                    rec.preserved_trait_mask     = preserved_traits;
                    rec.hop_distance_to_conflict = (partitions && i < part_cap) ? partitions[i].hop_distance : 1u;
                    pointer_records[pointer_count] = rec;
                }
                if (target_part && target_part->selection.mode == SelectionMode::SPARSE &&
                    target_part->selection.data.indices) {
                    target_part->selection.data.indices[pointer_count] = static_cast<int64_t>(i);
                }
                ++pointer_count;
            }
        }

        if (target_part && target_part->selection.mode == SelectionMode::SPARSE) {
            target_part->selection.element_count = pointer_count;
        }
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_ctx,
        const T_ActiveView &p_view
    ) const noexcept {
        (void)r_selection;
        T_ActiveView &mut_view = const_cast<T_ActiveView &>(p_view);
        execute<Op, T_ActiveView, T_ActiveStrategy>(p_ctx, mut_view);
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_query(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<Op, T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(PointerStateEinselectionQueryLogic<>) == 64, "PointerStateEinselectionQueryLogic must be 64 bytes!");
static_assert(IsQueryLogic<PointerStateEinselectionQueryLogic<>>, "PointerStateEinselectionQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core
