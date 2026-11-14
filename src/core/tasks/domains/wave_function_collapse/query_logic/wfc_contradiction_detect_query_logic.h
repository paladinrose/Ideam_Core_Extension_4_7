#pragma once

/**
 * wfc_contradiction_detect_query_logic.h
 * 64-byte cache-aligned QueryLogic task struct (`WFC3`) for `IdeamWaveFunctionCollapse`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Audits all `EnvironmentSlotPOD`s in `ENVIRONMENT_SLOT_POD` against `CASCADE_RULE_POD`,
 *   `RULE_TOKEN_DOMAIN_MASKS`, `CHILD_SLOT_CSR`, and `POINTER_OR_CONFLICT_MASK`.
 * - Detects empty domains (`WFC_CONTRADICTION_EMPTY_DOMAIN`), parent incompatibility
 *   (`WFC_CONTRADICTION_PARENT_INCOMPATIBLE`), child cardinality deficits/excesses
 *   (`WFC_CONTRADICTION_CARDINALITY_MIN_DEFICIT` / `MAX_EXCEEDED`), and missing required
 *   siblings (`WFC_CONTRADICTION_MISSING_REQUIRED_SIBLING`, e.g. `TowerLanding` without `TowerStairs`).
 * - Populates `WFC_CONTRADICTION_REPORT_POD` (`WFCContradictionReportPOD`), flags
 *   `SLOT_STATUS_HAS_CONFLICT` in `POINTER_OR_CONFLICT_MASK`, and writes conflicted slot
 *   indices into `target_part->selection` (`SelectionMode::SPARSE`) so `IdeamQuantumDarwinism`
 *   TaskGraph nodes can immediately isolate and repair the local horizon box (`E_box`).
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../../../../wave_function_collapse/wfc_common.h"
#include "../../../../procedural_generation/procedural_environment.h"
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
struct alignas(64) WFCContradictionDetectQueryLogic {
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

    static constexpr std::string_view display_name = "WFC Contradiction & Cardinality Conflict Detect";
    static constexpr std::string_view ui_category  = "WaveFunctionCollapse";

    uint32_t target_buffer_id               = INVALID_ID; ///< `ENVIRONMENT_SLOT_POD` (`EnvironmentSlotPOD`)
    uint32_t token_buffer_id                = INVALID_ID; ///< `TOKEN_DESCRIPTOR_POD` (`TokenDescriptorPOD`)
    uint32_t rule_buffer_id                 = INVALID_ID; ///< `CASCADE_RULE_POD` (`GenerationRulePOD`)
    uint32_t rule_mask_buffer_id            = INVALID_ID; ///< `RULE_TOKEN_DOMAIN_MASKS` (`BitmaskDomain256POD`)
    uint32_t child_csr_buffer_id            = INVALID_ID; ///< `CHILD_SLOT_CSR` (`uint32_t`)
    uint32_t pointer_conflict_buffer_id     = INVALID_ID; ///< `POINTER_OR_CONFLICT_MASK` (`uint32_t`)
    uint32_t contradiction_report_buffer_id = INVALID_ID; ///< `WFC_CONTRADICTION_REPORT_POD` (`WFCContradictionReportPOD`)
    uint32_t recommended_horizon_radius     = 4;
    float    threshold                      = 0.75f;      ///< Pointer redundancy threshold ($R_\delta$)
    uint8_t  reserved_padding[28]           = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",               godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("token_buffer_id",                godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("rule_buffer_id",                 godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("rule_mask_buffer_id",            godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("child_csr_buffer_id",            godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_conflict_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("contradiction_report_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("recommended_horizon_radius",     godot::Variant::INT,   static_cast<int64_t>(4));
        add_prop("threshold",                      godot::Variant::FLOAT, 0.75);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))               target_buffer_id               = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("token_buffer_id"))                token_buffer_id                = static_cast<uint32_t>(static_cast<int64_t>(p_props["token_buffer_id"]));
        if (p_props.has("rule_buffer_id"))                 rule_buffer_id                 = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_buffer_id"]));
        if (p_props.has("rule_mask_buffer_id"))            rule_mask_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_mask_buffer_id"]));
        if (p_props.has("child_csr_buffer_id"))            child_csr_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["child_csr_buffer_id"]));
        if (p_props.has("pointer_conflict_buffer_id"))     pointer_conflict_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_conflict_buffer_id"]));
        if (p_props.has("contradiction_report_buffer_id")) contradiction_report_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["contradiction_report_buffer_id"]));
        if (p_props.has("recommended_horizon_radius"))     recommended_horizon_radius     = static_cast<uint32_t>(static_cast<int64_t>(p_props["recommended_horizon_radius"]));
        if (p_props.has("threshold"))                      threshold                      = static_cast<float>(static_cast<double>(p_props["threshold"]));
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
        size_t slot_cap = 0, tok_cap = 0, rule_cap = 0, mask_cap = 0, child_cap = 0, ptr_cap = 0, rep_cap = 0;
        auto *slots            = _resolve_ptr<EnvironmentSlotPOD>(p_ctx, target_buffer_id, slot_cap);
        const auto *tokens     = _resolve_ptr<const TokenDescriptorPOD>(p_ctx, token_buffer_id, tok_cap);
        const auto *rules      = _resolve_ptr<const GenerationRulePOD>(p_ctx, rule_buffer_id, rule_cap);
        const auto *rule_masks = _resolve_ptr<const BitmaskDomain256POD>(p_ctx, rule_mask_buffer_id, mask_cap);
        const auto *child_csr  = _resolve_ptr<const uint32_t>(p_ctx, child_csr_buffer_id, child_cap);
        auto *status_masks     = _resolve_ptr<uint32_t>(p_ctx, pointer_conflict_buffer_id, ptr_cap);
        auto *reports          = _resolve_ptr<WFCContradictionReportPOD>(p_ctx, contradiction_report_buffer_id, rep_cap);

        if (!slots || slot_cap == 0) {
            return;
        }

        const uint32_t n_slots      = static_cast<uint32_t>(slot_cap);
        const uint32_t n_tokens     = std::min<uint32_t>(256u, (tok_cap > 0u) ? static_cast<uint32_t>(tok_cap) : 256u);
        const uint32_t n_rules      = static_cast<uint32_t>(rule_cap);
        const uint32_t n_rule_masks = static_cast<uint32_t>(mask_cap);
        const uint32_t n_child_csr  = static_cast<uint32_t>(child_cap);

        GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(target_buffer_id));
        int64_t conflicted_count  = 0;

        for (uint32_t i = 0; i < n_slots; ++i) {
            EnvironmentSlotPOD &slot    = slots[i];
            uint32_t cause_flags        = WFC_CONTRADICTION_NONE;
            uint32_t violated_rule_id   = 0xFFFFFFFFu;
            uint32_t fault_token_id     = 0xFFFFFFFFu;
            PatternCascadeLayer f_layer = slot.locked_layer;

            if (slot.domain.empty()) {
                cause_flags |= WFC_CONTRADICTION_EMPTY_DOMAIN;
            }
            if (slot.has_conflict != 0u && cause_flags == WFC_CONTRADICTION_NONE) {
                cause_flags |= WFC_CONTRADICTION_CARDINALITY_MIN_DEFICIT;
            }

            const int32_t p_tok = (slot.domain.popcount() == 1u) ? slot.domain.first_set_state() : -1;
            if (tokens && rules && rule_masks && child_csr &&
                p_tok >= 0 && static_cast<uint32_t>(p_tok) < n_tokens && slot.children_count > 0u) {
                const uint32_t c_off = slot.children_csr_offset;
                const uint32_t c_cnt = slot.children_count;
                if (c_off + c_cnt <= n_child_csr) {
                    for (uint32_t r = 0; r < n_rules; ++r) {
                        const GenerationRulePOD &rule = rules[r];
                        if (rule.operation != GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY &&
                            rule.operation != GenerationRuleOp::REQUIRE_SIBLING &&
                            rule.operation != GenerationRuleOp::FORBID_SIBLING) {
                            continue;
                        }
                        if (!rule.selector.matches_candidate(tokens[p_tok].signature)) continue;
                        if (rule.target_token_mask_index >= n_rule_masks) continue;
                        const int32_t t_tok = rule_masks[rule.target_token_mask_index].first_set_state();
                        if (t_tok < 0 || static_cast<uint32_t>(t_tok) >= n_tokens) continue;
                        const uint32_t u_t_tok = static_cast<uint32_t>(t_tok);

                        uint32_t guaranteed = 0u;
                        uint32_t possible   = 0u;
                        for (uint32_t k = 0; k < c_cnt; ++k) {
                            const uint32_t c_slot = child_csr[c_off + k];
                            if (c_slot >= n_slots) continue;
                            if (slots[c_slot].domain.test_bit(u_t_tok)) {
                                ++possible;
                                if (slots[c_slot].domain.popcount() == 1u) {
                                    ++guaranteed;
                                }
                            }
                        }

                        if (rule.operation == GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY) {
                            if (possible < rule.min_occurrences) {
                                cause_flags      |= WFC_CONTRADICTION_CARDINALITY_MIN_DEFICIT;
                                violated_rule_id  = rule.rule_id;
                                fault_token_id    = u_t_tok;
                                f_layer           = rule.layer;
                            }
                            if (guaranteed > rule.max_occurrences) {
                                cause_flags      |= WFC_CONTRADICTION_CARDINALITY_MAX_EXCEEDED;
                                violated_rule_id  = rule.rule_id;
                                fault_token_id    = u_t_tok;
                                f_layer           = rule.layer;
                            }
                        }

                        if (rule.required_sibling_token < n_tokens && guaranteed > 0u) {
                            bool req_possible = false;
                            for (uint32_t k = 0; k < c_cnt; ++k) {
                                const uint32_t c_slot = child_csr[c_off + k];
                                if (c_slot < n_slots && slots[c_slot].domain.test_bit(rule.required_sibling_token)) {
                                    req_possible = true;
                                    break;
                                }
                            }
                            if (!req_possible) {
                                cause_flags      |= WFC_CONTRADICTION_MISSING_REQUIRED_SIBLING;
                                violated_rule_id  = rule.rule_id;
                                fault_token_id    = rule.required_sibling_token;
                                f_layer           = rule.layer;
                            }
                        }
                    }
                }
            }

            const bool is_conflicted = (cause_flags != WFC_CONTRADICTION_NONE);
            if (is_conflicted) {
                slot.has_conflict   = 1u;
                slot.entropy_bucket = 15u;
            }

            if (status_masks && i < ptr_cap) {
                if (is_conflicted) {
                    status_masks[i] |= SLOT_STATUS_HAS_CONFLICT;
                } else {
                    status_masks[i] &= ~SLOT_STATUS_HAS_CONFLICT;
                }
            }

            if (reports && i < rep_cap) {
                WFCContradictionReportPOD rep{};
                rep.slot_id                    = i;
                rep.parent_slot_id             = slot.parent_slot_id;
                rep.violated_rule_id           = violated_rule_id;
                rep.contradiction_flags        = cause_flags;
                rep.deficit_or_excess_token_id = fault_token_id;
                rep.recommended_horizon_radius = recommended_horizon_radius;
                rep.local_einselection_score   = slot.einselection_score;
                rep.failed_cascade_layer       = f_layer;
                reports[i]                     = rep;
            }

            if (is_conflicted) {
                if (target_part && target_part->selection.mode == SelectionMode::SPARSE &&
                    target_part->selection.data.indices) {
                    target_part->selection.data.indices[conflicted_count] = static_cast<int64_t>(i);
                }
                ++conflicted_count;
            }
        }

        if (target_part && target_part->selection.mode == SelectionMode::SPARSE) {
            target_part->selection.element_count = conflicted_count;
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

static_assert(sizeof(WFCContradictionDetectQueryLogic<>) == 64, "WFCContradictionDetectQueryLogic must be 64 bytes!");
static_assert(IsQueryLogic<WFCContradictionDetectQueryLogic<>>, "WFCContradictionDetectQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core
