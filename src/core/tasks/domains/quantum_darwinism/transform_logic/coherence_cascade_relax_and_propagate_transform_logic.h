#pragma once

/**
 * coherence_cascade_relax_and_propagate_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`QD3`) for `IdeamQuantumDarwinism`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 1.5 — STAGES 3 & 4)
 * ============================================================================
 * - Traces bridge paths (`COHERENCE_BRIDGE_SLOT_POD`) inside $\mathcal{E}_{\text{box}}$
 *   from settled Pointer States toward conflicted slots.
 * - Consults the **CSS `GenerationRule` Cascade** and **`IdeamLinguistics` Nuclear vs.
 *   Peripheral Semes**:
 *     * Keeps high-precedence structural layers (`LAYER_DOMAIN`, `LAYER_HIERARCHY_PARENT`,
 *       `LAYER_PINNED_OVERRIDE`, and `nuclear_seme_mask_lo32`) locked on intermediate
 *       bridge slots so macro-coherence is preserved (e.g., `CastleTower` under
 *       `CastleGrounds` stays a `CastleTower`).
 *     * Selectively relaxes lower-precedence leaf/sibling/cardinality layers
 *       (`LAYER_CHILD_CARDINALITY`, `LAYER_SIBLING_DEPENDENCY`, `LAYER_VARIANT_DETAIL`,
 *       and `peripheral_seme_mask_lo32`) on non-pinned child/sibling slots so unmet
 *       cardinality budgets (`TowerGate [1..2]`) and required siblings (`TowerStairs`
 *       for `TowerLanding`) rebalance and resolve the conflict in-place!
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
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) CoherenceCascadeRelaxAndPropagateTransformLogic {
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

    static constexpr std::string_view display_name = "QD Coherence Cascade Relax & Pointer Propagate";
    static constexpr std::string_view ui_category  = "QuantumDarwinism";

    uint32_t target_buffer_id           = INVALID_ID; ///< `ENVIRONMENT_SLOT_POD` (`EnvironmentSlotPOD`)
    uint32_t token_buffer_id            = INVALID_ID; ///< `TOKEN_DESCRIPTOR_POD` (`TokenDescriptorPOD`)
    uint32_t rule_buffer_id             = INVALID_ID; ///< `CASCADE_RULE_POD` (`GenerationRulePOD`)
    uint32_t rule_mask_buffer_id        = INVALID_ID; ///< `RULE_TOKEN_DOMAIN_MASKS` (`BitmaskDomain256POD`)
    uint32_t child_csr_buffer_id        = INVALID_ID; ///< `CHILD_SLOT_CSR` (`uint32_t`)
    uint32_t horizon_partition_buf_id   = INVALID_ID; ///< `HORIZON_PARTITION_POD` (`HorizonSlotPartitionPOD`)
    uint32_t pointer_record_buffer_id   = INVALID_ID; ///< `POINTER_STATE_RECORD_POD` (`PointerStateRecordPOD`)
    uint32_t bridge_slot_buffer_id      = INVALID_ID; ///< `COHERENCE_BRIDGE_SLOT_POD` (`CoherenceBridgeSlotPOD`)
    uint32_t pointer_buffer_id          = INVALID_ID; ///< `POINTER_OR_CONFLICT_MASK` (`uint32_t`)
    float    pointer_redundancy_thresh  = 0.75f;
    uint8_t  allow_sibling_rebalancing  = 1;
    uint8_t  reserved_padding[23]       = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",          godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("token_buffer_id",           godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("rule_buffer_id",            godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("rule_mask_buffer_id",       godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("child_csr_buffer_id",       godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("horizon_partition_buf_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_record_buffer_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("bridge_slot_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_redundancy_thresh", godot::Variant::FLOAT, 0.75);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))          target_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("token_buffer_id"))           token_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["token_buffer_id"]));
        if (p_props.has("rule_buffer_id"))            rule_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_buffer_id"]));
        if (p_props.has("rule_mask_buffer_id"))       rule_mask_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_mask_buffer_id"]));
        if (p_props.has("child_csr_buffer_id"))       child_csr_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["child_csr_buffer_id"]));
        if (p_props.has("horizon_partition_buf_id"))  horizon_partition_buf_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["horizon_partition_buf_id"]));
        if (p_props.has("pointer_record_buffer_id"))  pointer_record_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_record_buffer_id"]));
        if (p_props.has("bridge_slot_buffer_id"))     bridge_slot_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["bridge_slot_buffer_id"]));
        if (p_props.has("pointer_buffer_id"))         pointer_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_buffer_id"]));
        if (p_props.has("pointer_redundancy_thresh")) pointer_redundancy_thresh = static_cast<float>(static_cast<double>(p_props["pointer_redundancy_thresh"]));
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

    static void relax_and_stabilize_in_place(
        EnvironmentSlotPOD *r_slots,
        uint32_t p_n_slots,
        const TokenDescriptorPOD *p_tokens,
        uint32_t p_n_tokens,
        const GenerationRulePOD *p_rules,
        uint32_t p_n_rules,
        const BitmaskDomain256POD *p_rule_masks,
        uint32_t p_n_rule_masks,
        const uint32_t *p_child_csr,
        uint32_t p_n_child_csr,
        HorizonSlotPartitionPOD *r_partitions,
        uint32_t p_n_partitions,
        const PointerStateRecordPOD *p_pointer_records,
        uint32_t p_n_pointer_records,
        CoherenceBridgeSlotPOD *r_bridges,
        uint32_t p_max_bridges,
        uint32_t *r_status_masks,
        uint32_t p_n_status_masks,
        float p_pointer_thresh,
        bool p_allow_sibling_rebalancing
    ) noexcept {
        if (!r_slots || p_n_slots == 0u || !p_tokens || p_n_tokens == 0u) {
            return;
        }
        const uint32_t n_tok = std::min<uint32_t>(256u, p_n_tokens);

        // 1. Compute base ADMIT_ARCHETYPE domain mask
        BitmaskDomain256POD domain_admitted{};
        if (p_rules && p_rule_masks) {
            for (uint32_t r = 0; r < p_n_rules; ++r) {
                if (p_rules[r].operation == GenerationRuleOp::ADMIT_ARCHETYPE &&
                    p_rules[r].target_token_mask_index < p_n_rule_masks) {
                    domain_admitted.union_with(p_rule_masks[p_rules[r].target_token_mask_index]);
                }
            }
        }
        if (domain_admitted.empty()) {
            domain_admitted.fill_first_n(n_tok);
        }

        // Helper: compute valid domain for a slot preserving LAYER_DOMAIN + LAYER_HIERARCHY_PARENT
        auto compute_coherent_domain = [&](uint32_t p_slot_id, bool p_preserve_parent_identity) -> BitmaskDomain256POD {
            const EnvironmentSlotPOD &slot = r_slots[p_slot_id];
            if (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE && !slot.domain.empty()) {
                return slot.domain;
            }
            if (p_preserve_parent_identity && slot.children_count > 0u && slot.collapsed_token_id >= 0 &&
                static_cast<uint32_t>(slot.collapsed_token_id) < n_tok) {
                BitmaskDomain256POD self_mask{};
                self_mask.set_bit(static_cast<uint32_t>(slot.collapsed_token_id));
                return self_mask;
            }

            BitmaskDomain256POD dom = domain_admitted;
            if (slot.parent_slot_id < p_n_slots && p_rules && p_rule_masks) {
                const EnvironmentSlotPOD &parent_slot = r_slots[slot.parent_slot_id];
                for (uint32_t r = 0; r < p_n_rules; ++r) {
                    const GenerationRulePOD &rule = p_rules[r];
                    if (rule.operation == GenerationRuleOp::ACCEPT_PARENTS &&
                        rule.target_token_mask_index < p_n_rule_masks) {
                        const BitmaskDomain256POD &allowed_parents = p_rule_masks[rule.target_token_mask_index];
                        for (uint32_t t = 0; t < n_tok; ++t) {
                            if (!dom.test_bit(t)) continue;
                            if (rule.selector.matches_candidate(p_tokens[t].signature) &&
                                !parent_slot.domain.overlaps(allowed_parents)) {
                                dom.clear_bit(t);
                            }
                        }
                    }
                }
                const int32_t p_tok = (parent_slot.domain.popcount() == 1u)
                    ? parent_slot.domain.first_set_state()
                    : parent_slot.collapsed_token_id;
                if (p_tok >= 0 && static_cast<uint32_t>(p_tok) < n_tok) {
                    BitmaskDomain256POD allowed_children{};
                    bool has_c_rules = false;
                    for (uint32_t r = 0; r < p_n_rules; ++r) {
                        const GenerationRulePOD &rule = p_rules[r];
                        if (rule.operation == GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY &&
                            rule.selector.matches_candidate(p_tokens[p_tok].signature) &&
                            rule.target_token_mask_index < p_n_rule_masks) {
                            has_c_rules = true;
                            allowed_children.union_with(p_rule_masks[rule.target_token_mask_index]);
                        }
                    }
                    if (has_c_rules && !allowed_children.empty()) {
                        dom.intersect_with(allowed_children);
                    }
                }
            }
            return dom.empty() ? domain_admitted : dom;
        };

        // 2. Populate CoherenceBridgeSlotPODs & relax non-pointer conflict/bridge slots
        uint32_t bridge_count = 0u;
        for (uint32_t i = 0; i < p_n_slots; ++i) {
            EnvironmentSlotPOD &slot = r_slots[i];
            const bool in_horizon = (!r_partitions || i >= p_n_partitions)
                ? (slot.has_conflict != 0u || slot.domain.empty())
                : (r_partitions[i].hop_distance != 0xFFFFu);
            if (!in_horizon) continue;

            const bool is_pointer = (slot.is_pointer_state != 0u) ||
                                    (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE && !slot.domain.empty());
            if (is_pointer) continue;

            const bool is_structural_parent = (slot.children_count > 0u && !slot.domain.empty());
            slot.domain             = compute_coherent_domain(i, is_structural_parent);
            const uint32_t pop      = slot.domain.popcount();
            slot.collapsed_token_id = (pop == 1u) ? slot.domain.first_set_state() : -1;
            slot.has_conflict       = 0u;

            if (r_partitions && i < p_n_partitions) {
                r_partitions[i].role_flags |= HORIZON_ROLE_BRIDGE_RELAXED;
            }
            if (r_bridges && bridge_count < p_max_bridges) {
                CoherenceBridgeSlotPOD b{};
                b.slot_id                 = i;
                b.source_pointer_slot_id  = (p_pointer_records && p_n_pointer_records > 0u) ? p_pointer_records[0].slot_id : i;
                b.target_conflict_slot_id = (r_partitions && i < p_n_partitions) ? r_partitions[i].nearest_conflict_slot : i;
                b.hops_from_pointer       = 1u;
                b.hops_to_conflict        = (r_partitions && i < p_n_partitions) ? r_partitions[i].hop_distance : 0u;
                b.mutable_trait_mask      = 0xFFFFFFFFu;
                b.relax_down_to           = PatternCascadeLayer::LAYER_CHILD_CARDINALITY;
                r_bridges[bridge_count++] = b;
            }

            // Rebalance non-pinned children of a conflicted parent slot (or siblings under slot.parent_slot_id)
            const uint32_t reb_p = (slot.children_count > 0u) ? i : slot.parent_slot_id;
            if (p_allow_sibling_rebalancing && p_child_csr && reb_p < p_n_slots && r_slots[reb_p].children_count > 0u &&
                (r_slots[reb_p].children_csr_offset + r_slots[reb_p].children_count) <= p_n_child_csr) {
                for (uint32_t k = 0; k < r_slots[reb_p].children_count; ++k) {
                    const uint32_t c_id = p_child_csr[r_slots[reb_p].children_csr_offset + k];
                    if (c_id >= p_n_slots) continue;
                    EnvironmentSlotPOD &c_slot = r_slots[c_id];
                    if (c_slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE && !c_slot.domain.empty()) {
                        continue;
                    }
                    c_slot.domain             = compute_coherent_domain(c_id, false);
                    const uint32_t c_pop      = c_slot.domain.popcount();
                    c_slot.collapsed_token_id = (c_pop == 1u) ? c_slot.domain.first_set_state() : -1;
                    c_slot.has_conflict       = 0u;
                    c_slot.is_pointer_state   = 0u;
                }
            }
        }

        // 3. Re-collapse relaxed slots inside E_box prioritizing required siblings & min_occurrences deficits
        uint32_t union_pointer_traits = 0u;
        if (p_pointer_records) {
            for (uint32_t p = 0; p < p_n_pointer_records; ++p) {
                union_pointer_traits |= p_pointer_records[p].preserved_trait_mask;
            }
        }

        for (uint32_t pass = 0; pass < p_n_slots + 2u; ++pass) {
            bool any_open = false;
            for (uint32_t i = 0; i < p_n_slots; ++i) {
                EnvironmentSlotPOD &slot = r_slots[i];
                if (slot.domain.popcount() <= 1u) continue;
                any_open = true;

                int32_t chosen_tok = -1;
                if (slot.parent_slot_id < p_n_slots && p_rules && p_rule_masks && p_child_csr) {
                    const EnvironmentSlotPOD &parent_slot = r_slots[slot.parent_slot_id];
                    const int32_t p_tok = (parent_slot.domain.popcount() == 1u)
                        ? parent_slot.domain.first_set_state()
                        : parent_slot.collapsed_token_id;
                    if (p_tok >= 0 && static_cast<uint32_t>(p_tok) < n_tok &&
                        (parent_slot.children_csr_offset + parent_slot.children_count) <= p_n_child_csr) {
                        for (uint32_t r = 0; r < p_n_rules; ++r) {
                            const GenerationRulePOD &rule = p_rules[r];
                            if (rule.operation != GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY) continue;
                            if (!rule.selector.matches_candidate(p_tokens[p_tok].signature)) continue;
                            if (rule.target_token_mask_index >= p_n_rule_masks) continue;
                            const int32_t c_tok = p_rule_masks[rule.target_token_mask_index].first_set_state();
                            if (c_tok < 0 || static_cast<uint32_t>(c_tok) >= n_tok) continue;
                            const uint32_t u_c_tok = static_cast<uint32_t>(c_tok);

                            uint32_t definite = 0u;
                            for (uint32_t k = 0; k < parent_slot.children_count; ++k) {
                                const uint32_t sib_id = p_child_csr[parent_slot.children_csr_offset + k];
                                if (sib_id < p_n_slots && r_slots[sib_id].domain.popcount() == 1u &&
                                    r_slots[sib_id].domain.test_bit(u_c_tok)) {
                                    ++definite;
                                }
                            }

                            // Priority A: Missing required sibling (e.g., TowerStairs required by TowerLanding)
                            if (definite > 0u && rule.required_sibling_token < n_tok &&
                                slot.domain.test_bit(rule.required_sibling_token)) {
                                uint32_t req_def = 0u;
                                for (uint32_t k = 0; k < parent_slot.children_count; ++k) {
                                    const uint32_t sib_id = p_child_csr[parent_slot.children_csr_offset + k];
                                    if (sib_id < p_n_slots && r_slots[sib_id].domain.popcount() == 1u &&
                                        r_slots[sib_id].domain.test_bit(rule.required_sibling_token)) {
                                        ++req_def;
                                    }
                                }
                                if (req_def == 0u) {
                                    chosen_tok = static_cast<int32_t>(rule.required_sibling_token);
                                    break;
                                }
                            }

                            // Priority B: Unmet min_occurrences child (e.g., TowerGate [1..2])
                            if (definite < rule.min_occurrences && slot.domain.test_bit(u_c_tok)) {
                                chosen_tok = c_tok;
                                break;
                            }
                        }
                    }
                }

                if (chosen_tok < 0) {
                    chosen_tok      = slot.domain.first_set_state();
                    uint32_t best_w = 0u;
                    for (uint32_t t = 0; t < n_tok; ++t) {
                        if (!slot.domain.test_bit(t)) continue;
                        const uint32_t overlap = static_cast<uint32_t>(
                            std::popcount((p_tokens[t].coherence_trait_mask | p_tokens[t].nuclear_seme_mask_lo32) & union_pointer_traits)
                        );
                        if (overlap > best_w) {
                            best_w     = overlap;
                            chosen_tok = static_cast<int32_t>(t);
                        }
                    }
                }

                if (chosen_tok >= 0) {
                    slot.domain.clear();
                    slot.domain.set_bit(static_cast<uint32_t>(chosen_tok));
                    slot.collapsed_token_id = chosen_tok;
                }
            }
            if (!any_open) break;
        }

        // 4. Refresh status flags & Einselection Redundancy scores
        for (uint32_t i = 0; i < p_n_slots; ++i) {
            EnvironmentSlotPOD &slot = r_slots[i];
            const uint32_t pop       = slot.domain.popcount();
            slot.collapsed_token_id  = (pop == 1u) ? slot.domain.first_set_state() : -1;
            slot.has_conflict        = (pop == 0u) ? 1u : 0u;
            slot.entropy_bucket      = (pop == 1u) ? 0u : static_cast<uint8_t>(std::min<uint32_t>(14u, pop));
            if (pop == 1u && slot.einselection_score < p_pointer_thresh) {
                slot.einselection_score = std::max(0.80f, p_pointer_thresh);
            }
            slot.is_pointer_state = (slot.einselection_score >= p_pointer_thresh && slot.has_conflict == 0u) ? 1u : 0u;

            if (r_status_masks && i < p_n_status_masks) {
                uint32_t flags = SLOT_STATUS_CLEAN;
                if (slot.is_pointer_state != 0u) flags |= SLOT_STATUS_POINTER_STATE;
                if (slot.has_conflict != 0u)     flags |= SLOT_STATUS_HAS_CONFLICT;
                if (pop == 1u)                   flags |= SLOT_STATUS_COLLAPSED;
                if (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE) flags |= SLOT_STATUS_PINNED;
                r_status_masks[i] = flags;
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t slot_cap = 0, tok_cap = 0, rule_cap = 0, mask_cap = 0, child_cap = 0;
        size_t part_cap = 0, rec_cap = 0, bridge_cap = 0, status_cap = 0;

        auto *slots            = _resolve_ptr<EnvironmentSlotPOD>(p_ctx, target_buffer_id, slot_cap);
        const auto *tokens     = _resolve_ptr<const TokenDescriptorPOD>(p_ctx, token_buffer_id, tok_cap);
        const auto *rules      = _resolve_ptr<const GenerationRulePOD>(p_ctx, rule_buffer_id, rule_cap);
        const auto *rule_masks = _resolve_ptr<const BitmaskDomain256POD>(p_ctx, rule_mask_buffer_id, mask_cap);
        const auto *child_csr  = _resolve_ptr<const uint32_t>(p_ctx, child_csr_buffer_id, child_cap);
        auto *partitions       = _resolve_ptr<HorizonSlotPartitionPOD>(p_ctx, horizon_partition_buf_id, part_cap);
        const auto *ptr_recs   = _resolve_ptr<const PointerStateRecordPOD>(p_ctx, pointer_record_buffer_id, rec_cap);
        auto *bridges          = _resolve_ptr<CoherenceBridgeSlotPOD>(p_ctx, bridge_slot_buffer_id, bridge_cap);
        auto *status_masks     = _resolve_ptr<uint32_t>(p_ctx, pointer_buffer_id, status_cap);

        relax_and_stabilize_in_place(
            slots, static_cast<uint32_t>(slot_cap),
            tokens, static_cast<uint32_t>(tok_cap),
            rules, static_cast<uint32_t>(rule_cap),
            rule_masks, static_cast<uint32_t>(mask_cap),
            child_csr, static_cast<uint32_t>(child_cap),
            partitions, static_cast<uint32_t>(part_cap),
            ptr_recs, static_cast<uint32_t>(rec_cap),
            bridges, static_cast<uint32_t>(bridge_cap),
            status_masks, static_cast<uint32_t>(status_cap),
            pointer_redundancy_thresh,
            allow_sibling_rebalancing != 0u
        );
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(CoherenceCascadeRelaxAndPropagateTransformLogic<>) == 64, "CoherenceCascadeRelaxAndPropagateTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<CoherenceCascadeRelaxAndPropagateTransformLogic<>>, "CoherenceCascadeRelaxAndPropagateTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
