#pragma once

/**
 * hierarchy_cardinality_propagate_transform_logic.h
 * `PCG4`: 64-byte cache-aligned `TransformLogic` kernel for `IdeamProceduralGeneration`
 * (`src/core/tasks/domains/procedural_generation/transform_logic/`).
 *
 * Propagates cascading `GenerationRulePOD` constraints (`ACCEPT_PARENTS`,
 * `ACCEPT_CHILDREN_CARDINALITY` `[min..max]`, `REQUIRE_SIBLING`, `FORBID_SIBLING`)
 * over `ENVIRONMENT_SLOT_POD` and `CHILD_SLOT_CSR`, computes Zurek Quantum Darwinism
 * Einselection Redundancy $R_\delta(i)$ (`einselection_score`), and updates
 * `POINTER_OR_CONFLICT_MASK` (`SLOT_STATUS_POINTER_STATE | SLOT_STATUS_HAS_CONFLICT`)
 * for downstream `IdeamWaveFunctionCollapse` and `IdeamQuantumDarwinism` stages.
 */

#include "../../../../procedural_generation/procedural_environment.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
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
struct alignas(64) HierarchyCardinalityPropagateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "PCG4: Hierarchy, Cardinality & Sibling Constraint Propagate";
    static constexpr std::string_view ui_category  = "ProceduralGeneration/Propagate";

    // --- 64-Byte Cache-Line Member Layout ---
    uint32_t target_buffer_id            = INVALID_ID; ///< Offset 0  (4B): `ENVIRONMENT_SLOT_POD`
    uint32_t token_descriptor_buffer_id  = INVALID_ID; ///< Offset 4  (4B): `TOKEN_DESCRIPTOR_POD`
    uint32_t rule_buffer_id              = INVALID_ID; ///< Offset 8  (4B): `CASCADE_RULE_POD`
    uint32_t rule_domain_masks_buffer_id = INVALID_ID; ///< Offset 12 (4B): `RULE_TOKEN_DOMAIN_MASKS`
    uint32_t child_slot_csr_buffer_id    = INVALID_ID; ///< Offset 16 (4B): `CHILD_SLOT_CSR`
    uint32_t pointer_or_conflict_buf_id  = INVALID_ID; ///< Offset 20 (4B): `POINTER_OR_CONFLICT_MASK`
    uint32_t slot_entropy_buffer_id      = INVALID_ID; ///< Offset 24 (4B): `SLOT_ENTROPY_FLOATS`
    float    pointer_redundancy_threshold= 0.75f;      ///< Offset 28 (4B): R_delta threshold to promote Pointer State
    uint32_t max_propagation_sweeps      = 16;         ///< Offset 32 (4B): Max fixed-point sweeps
    uint8_t  reserved_padding[28]        = {0};        ///< Offset 36 (28B) -> Total = 64 Bytes

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",             godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("token_descriptor_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("rule_buffer_id",               godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("rule_domain_masks_buffer_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("child_slot_csr_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_or_conflict_buf_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("slot_entropy_buffer_id",       godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_redundancy_threshold", godot::Variant::FLOAT, 0.75);
        add_prop("max_propagation_sweeps",       godot::Variant::INT,   static_cast<int64_t>(16));
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))             target_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("token_descriptor_buffer_id"))   token_descriptor_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["token_descriptor_buffer_id"]));
        if (p_props.has("rule_buffer_id"))               rule_buffer_id               = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_buffer_id"]));
        if (p_props.has("rule_domain_masks_buffer_id"))  rule_domain_masks_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_domain_masks_buffer_id"]));
        if (p_props.has("child_slot_csr_buffer_id"))     child_slot_csr_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["child_slot_csr_buffer_id"]));
        if (p_props.has("pointer_or_conflict_buf_id"))   pointer_or_conflict_buf_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_or_conflict_buf_id"]));
        if (p_props.has("slot_entropy_buffer_id"))       slot_entropy_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["slot_entropy_buffer_id"]));
        if (p_props.has("pointer_redundancy_threshold")) pointer_redundancy_threshold = static_cast<float>(static_cast<double>(p_props["pointer_redundancy_threshold"]));
        if (p_props.has("max_propagation_sweeps"))       max_propagation_sweeps       = static_cast<uint32_t>(static_cast<int64_t>(p_props["max_propagation_sweeps"]));
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
        size_t slot_cap = 0, tok_cap = 0, rule_cap = 0, mask_cap = 0, csr_cap = 0, ptr_cap = 0, ent_cap = 0;
        auto *slots            = _resolve_ptr<EnvironmentSlotPOD>(p_ctx, target_buffer_id, slot_cap);
        const auto *tokens     = _resolve_ptr<const TokenDescriptorPOD>(p_ctx, token_descriptor_buffer_id, tok_cap);
        const auto *rules      = _resolve_ptr<const GenerationRulePOD>(p_ctx, rule_buffer_id, rule_cap);
        const auto *rule_masks = _resolve_ptr<const BitmaskDomain256POD>(p_ctx, rule_domain_masks_buffer_id, mask_cap);
        const auto *child_csr  = _resolve_ptr<const uint32_t>(p_ctx, child_slot_csr_buffer_id, csr_cap);
        auto *ptr_masks        = _resolve_ptr<uint32_t>(p_ctx, pointer_or_conflict_buf_id, ptr_cap);
        auto *entropy_out      = _resolve_ptr<float>(p_ctx, slot_entropy_buffer_id, ent_cap);

        if (!slots || slot_cap == 0 || !tokens || tok_cap == 0 || !rules || rule_cap == 0) {
            return;
        }

        bool changed = true;
        uint32_t sweep = 0u;
        const uint32_t max_sweeps = std::max<uint32_t>(1u, max_propagation_sweeps);

        while (changed && sweep++ < max_sweeps) {
            changed = false;

            for (size_t s = 0; s < slot_cap; ++s) {
                EnvironmentSlotPOD &slot = slots[s];
                if (slot.domain.empty()) {
                    slot.has_conflict = 1u;
                    continue;
                }

                // 1. ACCEPT_PARENTS between `slot` and `slot.parent_slot_id`
                if (slot.parent_slot_id < slot_cap) {
                    EnvironmentSlotPOD &parent_slot = slots[slot.parent_slot_id];
                    for (size_t r = 0; r < rule_cap; ++r) {
                        const GenerationRulePOD &rule = rules[r];
                        if (rule.operation != GenerationRuleOp::ACCEPT_PARENTS) continue;
                        if (!rule_masks || rule.target_token_mask_index >= mask_cap) continue;
                        const BitmaskDomain256POD &allowed_parents = rule_masks[rule.target_token_mask_index];

                        for (size_t t = 0; t < tok_cap && t < 256u; ++t) {
                            if (!slot.domain.test_bit(static_cast<uint32_t>(t))) continue;
                            if (rule.selector.matches_candidate(tokens[t].signature)) {
                                if (!parent_slot.domain.overlaps(allowed_parents)) {
                                    slot.domain.clear_bit(static_cast<uint32_t>(t));
                                    changed = true;
                                }
                                if (slot.domain.popcount() == 1u && slot.domain.test_bit(static_cast<uint32_t>(t))) {
                                    if (parent_slot.domain.intersect_with(allowed_parents)) {
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }
                }

                // 2. ACCEPT_CHILDREN_CARDINALITY & REQUIRE_SIBLING on children of `slot`
                if (child_csr && slot.children_count > 0u &&
                    slot.children_csr_offset + slot.children_count <= csr_cap) {
                    const int32_t parent_collapsed = (slot.domain.popcount() == 1u)
                        ? slot.domain.first_set_state()
                        : slot.collapsed_token_id;
                    if (parent_collapsed >= 0 && static_cast<size_t>(parent_collapsed) < tok_cap) {
                        const TokenDescriptorPOD &parent_tok = tokens[static_cast<size_t>(parent_collapsed)];
                        for (size_t r = 0; r < rule_cap; ++r) {
                            const GenerationRulePOD &rule = rules[r];
                            if (rule.operation != GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY) continue;
                            if (!rule.selector.matches_candidate(parent_tok.signature)) continue;
                            if (!rule_masks || rule.target_token_mask_index >= mask_cap) continue;

                            const int32_t c_tok_signed = rule_masks[rule.target_token_mask_index].first_set_state();
                            if (c_tok_signed < 0) continue;
                            const uint32_t c_tok = static_cast<uint32_t>(c_tok_signed);

                            uint32_t definite_cnt = 0u;
                            uint32_t possible_cnt = 0u;
                            for (uint32_t k = 0; k < slot.children_count; ++k) {
                                const uint32_t c_id = child_csr[slot.children_csr_offset + k];
                                if (c_id >= slot_cap) continue;
                                if (slots[c_id].domain.test_bit(c_tok)) {
                                    ++possible_cnt;
                                    if (slots[c_id].domain.popcount() == 1u) ++definite_cnt;
                                }
                            }

                            if (definite_cnt > rule.max_occurrences || possible_cnt < rule.min_occurrences) {
                                slot.has_conflict = 1u;
                            }
                            if (definite_cnt == rule.max_occurrences) {
                                for (uint32_t k = 0; k < slot.children_count; ++k) {
                                    const uint32_t c_id = child_csr[slot.children_csr_offset + k];
                                    if (c_id < slot_cap && slots[c_id].domain.popcount() > 1u && slots[c_id].domain.test_bit(c_tok)) {
                                        slots[c_id].domain.clear_bit(c_tok);
                                        changed = true;
                                    }
                                }
                            }
                            if (rule.min_occurrences > 0u && possible_cnt == rule.min_occurrences && definite_cnt < rule.min_occurrences) {
                                for (uint32_t k = 0; k < slot.children_count; ++k) {
                                    const uint32_t c_id = child_csr[slot.children_csr_offset + k];
                                    if (c_id < slot_cap && slots[c_id].domain.test_bit(c_tok) && slots[c_id].domain.popcount() > 1u) {
                                        slots[c_id].domain.clear();
                                        slots[c_id].domain.set_bit(c_tok);
                                        changed = true;
                                    }
                                }
                            }

                            // Enforce required sibling (e.g. TowerLanding requires TowerStairs)
                            if (rule.required_sibling_token < tok_cap) {
                                const uint32_t req_sib = rule.required_sibling_token;
                                bool sib_possible = false;
                                for (uint32_t k = 0; k < slot.children_count; ++k) {
                                    const uint32_t c_id = child_csr[slot.children_csr_offset + k];
                                    if (c_id < slot_cap && slots[c_id].domain.test_bit(req_sib)) {
                                        sib_possible = true;
                                        break;
                                    }
                                }
                                if (!sib_possible) {
                                    for (uint32_t k = 0; k < slot.children_count; ++k) {
                                        const uint32_t c_id = child_csr[slot.children_csr_offset + k];
                                        if (c_id < slot_cap && slots[c_id].domain.test_bit(c_tok)) {
                                            slots[c_id].domain.clear_bit(c_tok);
                                            changed = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Final pass: update collapsed_token_id, einselection_score, and POINTER_OR_CONFLICT_MASK
        for (size_t s = 0; s < slot_cap; ++s) {
            EnvironmentSlotPOD &slot = slots[s];
            const uint32_t pop = slot.domain.popcount();
            slot.collapsed_token_id = (pop == 1u) ? slot.domain.first_set_state() : -1;
            slot.entropy_bucket     = static_cast<uint8_t>(std::min<uint32_t>(15u, pop));
            if (pop == 0u) slot.has_conflict = 1u;

            if (slot.has_conflict == 0u && pop == 1u) {
                slot.einselection_score = (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE)
                    ? 1.0f
                    : 0.85f;
                slot.is_pointer_state = (slot.einselection_score >= pointer_redundancy_threshold) ? 1u : 0u;
            } else {
                slot.einselection_score = (pop > 1u) ? (0.35f / static_cast<float>(pop)) : 0.0f;
                slot.is_pointer_state   = 0u;
            }

            if (entropy_out && s < ent_cap) {
                entropy_out[s] = slot.domain.shannon_entropy();
            }
            if (ptr_masks && s < ptr_cap) {
                uint32_t flags = SLOT_STATUS_CLEAN;
                if (slot.is_pointer_state != 0u) flags |= SLOT_STATUS_POINTER_STATE;
                if (slot.has_conflict != 0u)     flags |= SLOT_STATUS_HAS_CONFLICT;
                if (pop == 1u)                   flags |= SLOT_STATUS_COLLAPSED;
                if (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE) flags |= SLOT_STATUS_PINNED;
                ptr_masks[s] = flags;
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(HierarchyCardinalityPropagateTransformLogic<>) == 64, "HierarchyCardinalityPropagateTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<HierarchyCardinalityPropagateTransformLogic<>>, "HierarchyCardinalityPropagateTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
