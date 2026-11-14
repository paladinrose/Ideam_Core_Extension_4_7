#pragma once

/**
 * wfc_forward_collapse_and_propagate_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`WFC2`) for `IdeamWaveFunctionCollapse`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Operates on the shared `ProceduralEnvironment` DOD tables (`ENVIRONMENT_SLOT_POD`,
 *   `TOKEN_DESCRIPTOR_POD`, `CASCADE_RULE_POD`, `RULE_TOKEN_DOMAIN_MASKS`, `CHILD_SLOT_CSR`,
 *   `SPATIAL_NEIGHBOR_CSR`, and `WFC_RULE_TENSOR_POD`).
 * - Runs minimum-entropy slot collapse and propagates both spatial/port adjacency and
 *   cascading `GenerationRulePOD` hierarchy constraints (`CastleGrounds -> CastleTower`,
 *   child cardinality `[min..max]`, and sibling dependencies like `TowerLanding requires TowerStairs`).
 * - Flags any contradiction slots in `POINTER_OR_CONFLICT_MASK` (`SLOT_STATUS_HAS_CONFLICT`)
 *   and updates Quantum Darwinism Einselection Redundancy (`einselection_score`, $R_\delta$)
 *   so `IdeamQuantumDarwinism` TaskGraph nodes can stabilize the local environment horizon box.
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../../wave_function_collapse/wfc_common.h"
#include "../../../../procedural_generation/procedural_environment.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) WFCForwardCollapseAndPropagateTransformLogic {
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

    static constexpr std::string_view display_name = "WFC Forward Collapse & Hierarchy/Port Propagate";
    static constexpr std::string_view ui_category  = "WaveFunctionCollapse";

    // --- 64-Byte Cache-Line Member Layout (13 x 4B = 52B + 12B padding = 64B) ---
    uint32_t target_buffer_id           = INVALID_ID; ///< `ENVIRONMENT_SLOT_POD` (`EnvironmentSlotPOD`)
    uint32_t token_buffer_id            = INVALID_ID; ///< `TOKEN_DESCRIPTOR_POD` (`TokenDescriptorPOD`)
    uint32_t rule_buffer_id             = INVALID_ID; ///< `CASCADE_RULE_POD` (`GenerationRulePOD`)
    uint32_t rule_mask_buffer_id        = INVALID_ID; ///< `RULE_TOKEN_DOMAIN_MASKS` (`BitmaskDomain256POD`)
    uint32_t child_csr_buffer_id        = INVALID_ID; ///< `CHILD_SLOT_CSR` (`uint32_t`)
    uint32_t spatial_csr_buffer_id      = INVALID_ID; ///< `SPATIAL_NEIGHBOR_CSR` (`SpatialNeighborEdgePOD`)
    uint32_t wfc_rule_tensor_buffer_id  = INVALID_ID; ///< `WFC_RULE_TENSOR_POD` (`WFCRuleEntryPOD`)
    uint32_t pointer_buffer_id          = INVALID_ID; ///< `POINTER_OR_CONFLICT_MASK` (`uint32_t`)
    uint32_t slot_entropy_buffer_id     = INVALID_ID; ///< `SLOT_ENTROPY_FLOATS` (`float`)
    uint32_t max_collapse_steps         = 64;
    uint32_t master_seed_lo             = 1337u;
    float    pointer_redundancy_thresh  = 0.75f;
    uint8_t  reserved_padding[16]       = {0};

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
        add_prop("spatial_csr_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("wfc_rule_tensor_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("slot_entropy_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("max_collapse_steps",        godot::Variant::INT,   static_cast<int64_t>(64));
        add_prop("master_seed_lo",            godot::Variant::INT,   static_cast<int64_t>(1337));
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
        if (p_props.has("spatial_csr_buffer_id"))     spatial_csr_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["spatial_csr_buffer_id"]));
        if (p_props.has("wfc_rule_tensor_buffer_id")) wfc_rule_tensor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["wfc_rule_tensor_buffer_id"]));
        if (p_props.has("pointer_buffer_id"))         pointer_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_buffer_id"]));
        if (p_props.has("slot_entropy_buffer_id"))    slot_entropy_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["slot_entropy_buffer_id"]));
        if (p_props.has("max_collapse_steps"))        max_collapse_steps        = static_cast<uint32_t>(static_cast<int64_t>(p_props["max_collapse_steps"]));
        if (p_props.has("master_seed_lo"))            master_seed_lo            = static_cast<uint32_t>(static_cast<int64_t>(p_props["master_seed_lo"]));
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

    static bool propagate_all_constraints_in_place(
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
        const SpatialNeighborEdgePOD *p_spatial_csr,
        uint32_t p_n_spatial_csr,
        const WFCRuleEntryPOD *p_wfc_rules,
        uint32_t p_n_wfc_rules
    ) noexcept {
        bool any_conflict = false;
        bool changed      = true;
        uint32_t sweep    = 0u;

        while (changed && sweep < 16u) {
            changed = false;
            ++sweep;

            // 1. Directional Port / Socket WFC Rule Tensor Propagation across SPATIAL_NEIGHBOR_CSR
            if (p_spatial_csr && p_wfc_rules && p_n_wfc_rules > 0u) {
                for (uint32_t u = 0; u < p_n_slots; ++u) {
                    if (r_slots[u].domain.empty()) {
                        r_slots[u].has_conflict = 1u;
                        any_conflict = true;
                        continue;
                    }
                    const uint32_t s_off = r_slots[u].spatial_csr_offset;
                    const uint32_t s_cnt = r_slots[u].spatial_neighbor_count;
                    for (uint32_t k = 0; k < s_cnt && (s_off + k) < p_n_spatial_csr; ++k) {
                        const SpatialNeighborEdgePOD &edge = p_spatial_csr[s_off + k];
                        if (edge.neighbor_slot_id >= p_n_slots) continue;

                        BitmaskDomain256POD union_allowed{};
                        bool has_port_rules = false;
                        for (uint32_t r = 0; r < p_n_wfc_rules; ++r) {
                            const WFCRuleEntryPOD &wrule = p_wfc_rules[r];
                            if (wrule.port_id == edge.port_or_direction_id &&
                                r_slots[u].domain.test_bit(wrule.source_state)) {
                                has_port_rules = true;
                                union_allowed.union_with(wrule.allowed_mask);
                            }
                        }
                        if (has_port_rules) {
                            EnvironmentSlotPOD &nbr = r_slots[edge.neighbor_slot_id];
                            if (nbr.domain.intersect_with(union_allowed)) {
                                changed = true;
                                const uint32_t pop = nbr.domain.popcount();
                                if (pop == 0u) {
                                    nbr.has_conflict = 1u;
                                    any_conflict     = true;
                                } else if (pop == 1u) {
                                    nbr.collapsed_token_id = nbr.domain.first_set_state();
                                }
                            }
                        }
                    }
                }
            }

            // 2. Cascading GenerationRulePOD Hierarchy (`ACCEPT_PARENTS`), Child Cardinality (`[min..max]`),
            //    and Sibling Dependencies (`REQUIRE_SIBLING` / `FORBID_SIBLING`)
            if (p_tokens && p_rules && p_rule_masks) {
                for (uint32_t r = 0; r < p_n_rules; ++r) {
                    const GenerationRulePOD &rule = p_rules[r];
                    if (rule.target_token_mask_index >= p_n_rule_masks) continue;
                    const BitmaskDomain256POD &target_mask = p_rule_masks[rule.target_token_mask_index];

                    if (rule.operation == GenerationRuleOp::ACCEPT_PARENTS) {
                        for (uint32_t i = 0; i < p_n_slots; ++i) {
                            EnvironmentSlotPOD &slot = r_slots[i];
                            if (slot.parent_slot_id >= p_n_slots || slot.domain.empty()) continue;
                            const EnvironmentSlotPOD &parent_slot = r_slots[slot.parent_slot_id];

                            for (uint32_t t = 0; t < p_n_tokens; ++t) {
                                if (!slot.domain.test_bit(t)) continue;
                                if (!rule.selector.matches_candidate(p_tokens[t].signature)) continue;
                                if (!parent_slot.domain.overlaps(target_mask)) {
                                    slot.domain.clear_bit(t);
                                    changed = true;
                                }
                            }
                            const uint32_t pop = slot.domain.popcount();
                            if (pop == 0u) {
                                slot.has_conflict = 1u;
                                any_conflict      = true;
                            } else if (pop == 1u) {
                                slot.collapsed_token_id = slot.domain.first_set_state();
                                if (parent_slot.domain.popcount() != 1u) {
                                    const int32_t c_tok = slot.collapsed_token_id;
                                    if (c_tok >= 0 && static_cast<uint32_t>(c_tok) < p_n_tokens &&
                                        rule.selector.matches_candidate(p_tokens[c_tok].signature)) {
                                        EnvironmentSlotPOD &mut_parent = r_slots[slot.parent_slot_id];
                                        if (mut_parent.domain.intersect_with(target_mask)) {
                                            changed = true;
                                            if (mut_parent.domain.empty()) {
                                                mut_parent.has_conflict = 1u;
                                                any_conflict            = true;
                                            } else if (mut_parent.domain.popcount() == 1u) {
                                                mut_parent.collapsed_token_id = mut_parent.domain.first_set_state();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else if (rule.operation == GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY && p_child_csr) {
                        const int32_t child_tok = target_mask.first_set_state();
                        if (child_tok < 0 || static_cast<uint32_t>(child_tok) >= p_n_tokens) continue;
                        const uint32_t u_child_tok = static_cast<uint32_t>(child_tok);

                        for (uint32_t p = 0; p < p_n_slots; ++p) {
                            EnvironmentSlotPOD &parent_slot = r_slots[p];
                            if (parent_slot.children_count == 0u || parent_slot.domain.popcount() != 1u) continue;
                            const int32_t p_tok = parent_slot.domain.first_set_state();
                            if (p_tok < 0 || static_cast<uint32_t>(p_tok) >= p_n_tokens) continue;
                            if (!rule.selector.matches_candidate(p_tokens[p_tok].signature)) continue;

                            const uint32_t c_off = parent_slot.children_csr_offset;
                            const uint32_t c_cnt = parent_slot.children_count;
                            if (c_off + c_cnt > p_n_child_csr) continue;

                            uint32_t guaranteed = 0u;
                            uint32_t possible   = 0u;
                            for (uint32_t k = 0; k < c_cnt; ++k) {
                                const uint32_t c_slot = p_child_csr[c_off + k];
                                if (c_slot >= p_n_slots) continue;
                                if (r_slots[c_slot].domain.test_bit(u_child_tok)) {
                                    ++possible;
                                    if (r_slots[c_slot].domain.popcount() == 1u) {
                                        ++guaranteed;
                                    }
                                }
                            }

                            if (guaranteed > rule.max_occurrences || possible < rule.min_occurrences) {
                                parent_slot.has_conflict = 1u;
                                any_conflict             = true;
                            }

                            // Prune u_child_tok from remaining uncollapsed children when max_occurrences is reached
                            if (guaranteed == rule.max_occurrences) {
                                for (uint32_t k = 0; k < c_cnt; ++k) {
                                    const uint32_t c_slot = p_child_csr[c_off + k];
                                    if (c_slot >= p_n_slots) continue;
                                    EnvironmentSlotPOD &cs = r_slots[c_slot];
                                    if (cs.domain.popcount() > 1u && cs.domain.test_bit(u_child_tok)) {
                                        cs.domain.clear_bit(u_child_tok);
                                        changed = true;
                                        if (cs.domain.popcount() == 1u) {
                                            cs.collapsed_token_id = cs.domain.first_set_state();
                                        }
                                    }
                                }
                            }

                            // Force u_child_tok when remaining possible slots equal min_occurrences
                            if (rule.min_occurrences > 0u && possible == rule.min_occurrences && guaranteed < rule.min_occurrences) {
                                for (uint32_t k = 0; k < c_cnt; ++k) {
                                    const uint32_t c_slot = p_child_csr[c_off + k];
                                    if (c_slot >= p_n_slots) continue;
                                    EnvironmentSlotPOD &cs = r_slots[c_slot];
                                    if (cs.domain.test_bit(u_child_tok) && cs.domain.popcount() > 1u) {
                                        cs.domain.clear();
                                        cs.domain.set_bit(u_child_tok);
                                        cs.collapsed_token_id = static_cast<int32_t>(u_child_tok);
                                        changed = true;
                                    }
                                }
                            }

                            // Sibling co-occurrence dependency (e.g., TowerLanding requires TowerStairs)
                            if (rule.required_sibling_token < p_n_tokens) {
                                const uint32_t req_tok = rule.required_sibling_token;
                                uint32_t req_possible  = 0u;
                                uint32_t last_req_slot = 0xFFFFFFFFu;
                                for (uint32_t k = 0; k < c_cnt; ++k) {
                                    const uint32_t c_slot = p_child_csr[c_off + k];
                                    if (c_slot < p_n_slots && r_slots[c_slot].domain.test_bit(req_tok)) {
                                        ++req_possible;
                                        last_req_slot = c_slot;
                                    }
                                }
                                if (req_possible == 0u) {
                                    for (uint32_t k = 0; k < c_cnt; ++k) {
                                        const uint32_t c_slot = p_child_csr[c_off + k];
                                        if (c_slot >= p_n_slots) continue;
                                        EnvironmentSlotPOD &cs = r_slots[c_slot];
                                        if (cs.domain.test_bit(u_child_tok)) {
                                            cs.domain.clear_bit(u_child_tok);
                                            changed = true;
                                            if (cs.domain.empty()) {
                                                cs.has_conflict = 1u;
                                                any_conflict    = true;
                                            } else if (cs.domain.popcount() == 1u) {
                                                cs.collapsed_token_id = cs.domain.first_set_state();
                                            }
                                        }
                                    }
                                } else if (guaranteed > 0u && req_possible == 1u && last_req_slot < p_n_slots) {
                                    EnvironmentSlotPOD &req_cs = r_slots[last_req_slot];
                                    if (req_cs.domain.popcount() > 1u) {
                                        req_cs.domain.clear();
                                        req_cs.domain.set_bit(req_tok);
                                        req_cs.collapsed_token_id = static_cast<int32_t>(req_tok);
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return !any_conflict;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t slot_cap = 0, tok_cap = 0, rule_cap = 0, mask_cap = 0;
        size_t child_cap = 0, spatial_cap = 0, wfc_cap = 0, ptr_cap = 0, ent_cap = 0;

        auto *slots             = _resolve_ptr<EnvironmentSlotPOD>(p_ctx, target_buffer_id, slot_cap);
        const auto *tokens      = _resolve_ptr<const TokenDescriptorPOD>(p_ctx, token_buffer_id, tok_cap);
        const auto *rules       = _resolve_ptr<const GenerationRulePOD>(p_ctx, rule_buffer_id, rule_cap);
        const auto *rule_masks  = _resolve_ptr<const BitmaskDomain256POD>(p_ctx, rule_mask_buffer_id, mask_cap);
        const auto *child_csr   = _resolve_ptr<const uint32_t>(p_ctx, child_csr_buffer_id, child_cap);
        const auto *spatial_csr = _resolve_ptr<const SpatialNeighborEdgePOD>(p_ctx, spatial_csr_buffer_id, spatial_cap);
        const auto *wfc_rules   = _resolve_ptr<const WFCRuleEntryPOD>(p_ctx, wfc_rule_tensor_buffer_id, wfc_cap);
        auto *status_masks      = _resolve_ptr<uint32_t>(p_ctx, pointer_buffer_id, ptr_cap);
        auto *entropies         = _resolve_ptr<float>(p_ctx, slot_entropy_buffer_id, ent_cap);

        if (!slots || slot_cap == 0) {
            return;
        }

        const uint32_t n_slots       = static_cast<uint32_t>(slot_cap);
        const uint32_t n_tokens      = std::min<uint32_t>(256u, (tok_cap > 0u) ? static_cast<uint32_t>(tok_cap) : 256u);
        const uint32_t n_rules       = static_cast<uint32_t>(rule_cap);
        const uint32_t n_rule_masks  = static_cast<uint32_t>(mask_cap);
        const uint32_t n_child_csr   = static_cast<uint32_t>(child_cap);
        const uint32_t n_spatial_csr = static_cast<uint32_t>(spatial_cap);
        const uint32_t n_wfc_rules   = static_cast<uint32_t>(wfc_cap);

        SeedStreamPOD rng(static_cast<uint64_t>(master_seed_lo ? master_seed_lo : 1337u));

        // Initial propagation before collapsing
        propagate_all_constraints_in_place(
            slots, n_slots, tokens, n_tokens, rules, n_rules,
            rule_masks, n_rule_masks, child_csr, n_child_csr,
            spatial_csr, n_spatial_csr, wfc_rules, n_wfc_rules
        );

        const uint32_t steps_limit = (max_collapse_steps > 0u) ? max_collapse_steps : n_slots;
        for (uint32_t step = 0; step < steps_limit; ++step) {
            int32_t best_slot = -1;
            float best_score  = std::numeric_limits<float>::max();
            bool has_conflict = false;

            for (uint32_t i = 0; i < n_slots; ++i) {
                const uint32_t pop = slots[i].domain.popcount();
                if (pop == 0u || slots[i].has_conflict != 0u) {
                    slots[i].has_conflict = 1u;
                    has_conflict = true;
                    break;
                }
                if (pop == 1u) continue;

                const float parent_bias = (slots[i].parent_slot_id == 0xFFFFFFFFu) ? 0.0f : 8.0f;
                const float score       = slots[i].domain.shannon_entropy() + parent_bias + rng.next_unit_float() * 1e-4f;
                if (score < best_score) {
                    best_score = score;
                    best_slot  = static_cast<int32_t>(i);
                }
            }

            if (has_conflict || best_slot < 0) {
                break;
            }

            EnvironmentSlotPOD &chosen_slot = slots[static_cast<uint32_t>(best_slot)];
            float total_w = 0.0f;
            for (uint32_t t = 0; t < n_tokens; ++t) {
                if (chosen_slot.domain.test_bit(t)) {
                    const float w = (tokens && t < tok_cap && tokens[t].base_frequency_weight > 0.0f)
                        ? tokens[t].base_frequency_weight
                        : 1.0f;
                    total_w += w;
                }
            }
            const float pick = rng.next_unit_float() * total_w;
            float accum      = 0.0f;
            int32_t picked_t = chosen_slot.domain.first_set_state();
            for (uint32_t t = 0; t < n_tokens; ++t) {
                if (!chosen_slot.domain.test_bit(t)) continue;
                const float w = (tokens && t < tok_cap && tokens[t].base_frequency_weight > 0.0f)
                    ? tokens[t].base_frequency_weight
                    : 1.0f;
                accum += w;
                if (pick <= accum) {
                    picked_t = static_cast<int32_t>(t);
                    break;
                }
            }

            if (picked_t < 0) {
                chosen_slot.has_conflict = 1u;
                break;
            }
            chosen_slot.domain.clear();
            chosen_slot.domain.set_bit(static_cast<uint32_t>(picked_t));
            chosen_slot.collapsed_token_id = picked_t;

            if (!propagate_all_constraints_in_place(
                    slots, n_slots, tokens, n_tokens, rules, n_rules,
                    rule_masks, n_rule_masks, child_csr, n_child_csr,
                    spatial_csr, n_spatial_csr, wfc_rules, n_wfc_rules
                )) {
                break;
            }
        }

        for (uint32_t i = 0; i < n_slots; ++i) {
            EnvironmentSlotPOD &slot = slots[i];
            const uint32_t pop       = slot.domain.popcount();
            const float ent          = slot.domain.shannon_entropy();
            if (entropies && i < ent_cap) {
                entropies[i] = ent;
            }

            if (pop == 0u || slot.has_conflict != 0u) {
                slot.has_conflict       = 1u;
                slot.einselection_score = 0.0f;
                slot.is_pointer_state   = 0u;
                slot.entropy_bucket     = 15u;
            } else if (pop == 1u) {
                slot.collapsed_token_id = slot.domain.first_set_state();
                slot.entropy_bucket     = 0u;
                uint32_t witnesses      = 1u;
                uint32_t agreed         = 1u;
                if (slot.parent_slot_id < n_slots) {
                    ++witnesses;
                    if (slots[slot.parent_slot_id].domain.popcount() == 1u &&
                        slots[slot.parent_slot_id].has_conflict == 0u) {
                        ++agreed;
                    }
                }
                if (child_csr && slot.children_count > 0u &&
                    (slot.children_csr_offset + slot.children_count) <= n_child_csr) {
                    for (uint32_t k = 0; k < slot.children_count; ++k) {
                        const uint32_t c_id = child_csr[slot.children_csr_offset + k];
                        if (c_id < n_slots) {
                            ++witnesses;
                            if (slots[c_id].domain.popcount() == 1u && slots[c_id].has_conflict == 0u) {
                                ++agreed;
                            }
                        }
                    }
                }
                const float r_delta   = static_cast<float>(agreed) / static_cast<float>(witnesses);
                slot.einselection_score = (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE)
                    ? 1.0f
                    : std::clamp(r_delta, 0.0f, 1.0f);
                slot.is_pointer_state = (slot.einselection_score >= pointer_redundancy_thresh) ? 1u : 0u;
            } else {
                slot.einselection_score = 1.0f / static_cast<float>(pop);
                slot.is_pointer_state   = 0u;
            }

            if (status_masks && i < ptr_cap) {
                uint32_t flags = SLOT_STATUS_CLEAN;
                if (slot.is_pointer_state != 0u) flags |= SLOT_STATUS_POINTER_STATE;
                if (slot.has_conflict != 0u)     flags |= SLOT_STATUS_HAS_CONFLICT;
                if (pop == 1u)                   flags |= SLOT_STATUS_COLLAPSED;
                if (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE) flags |= SLOT_STATUS_PINNED;
                status_masks[i] = flags;
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(WFCForwardCollapseAndPropagateTransformLogic<>) == 64, "WFCForwardCollapseAndPropagateTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<WFCForwardCollapseAndPropagateTransformLogic<>>, "WFCForwardCollapseAndPropagateTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
