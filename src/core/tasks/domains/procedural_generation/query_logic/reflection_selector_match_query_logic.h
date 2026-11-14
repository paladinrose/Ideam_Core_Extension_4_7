#pragma once

/**
 * reflection_selector_match_query_logic.h
 * `PCG2`: 64-byte cache-aligned `QueryLogic` kernel for `IdeamProceduralGeneration`
 * (`src/core/tasks/domains/procedural_generation/query_logic/`).
 *
 * Evaluates CSS-like `ReflectionSignaturePOD` selectors (`class_name_hash`,
 * `object_name_hash`, `name_prefix_hash`, `name_suffix_hash`, `group_or_tag_mask`,
 * `property_trait_mask`, `method_trait_mask`, `signal_trait_mask`) over
 * `TOKEN_DESCRIPTOR_POD` buffers, populating `r_selection.data.indices` and
 * compiling `RULE_TOKEN_DOMAIN_MASKS` (`BitmaskDomain256POD`) for each rule.
 */

#include "../../../../procedural_generation/procedural_environment.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
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
struct alignas(64) ReflectionSelectorMatchQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "PCG2: Reflection Selector Match (CSS Rule Filter)";
    static constexpr std::string_view ui_category  = "ProceduralGeneration/Query";

    // --- 64-Byte Cache-Line Member Layout ---
    uint32_t target_buffer_id           = INVALID_ID; ///< Offset 0  (4B): `TOKEN_DESCRIPTOR_POD`
    uint32_t rule_buffer_id             = INVALID_ID; ///< Offset 4  (4B): `CASCADE_RULE_POD`
    uint32_t rule_domain_masks_buffer_id= INVALID_ID; ///< Offset 8  (4B): `RULE_TOKEN_DOMAIN_MASKS`
    uint32_t required_class_hash        = 0;          ///< Offset 12 (4B): Optional explicit class hash filter
    uint32_t required_suffix_hash       = 0;          ///< Offset 16 (4B): Optional explicit suffix hash filter (`"_cell"`)
    uint32_t required_seme_mask_lo32    = 0;          ///< Offset 20 (4B): Optional nuclear seme bitmask filter
    float    threshold                  = 0.75f;      ///< Offset 24 (4B): Minimum base_frequency_weight threshold
    uint8_t  reserved_padding[36]       = {0};        ///< Offset 28 (36B) -> Total = 64 Bytes

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",            godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("rule_buffer_id",              godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("rule_domain_masks_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("required_class_hash",         godot::Variant::INT,   static_cast<int64_t>(0));
        add_prop("required_suffix_hash",        godot::Variant::INT,   static_cast<int64_t>(0));
        add_prop("required_seme_mask_lo32",     godot::Variant::INT,   static_cast<int64_t>(0));
        add_prop("threshold",                   godot::Variant::FLOAT, 0.75);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))            target_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("rule_buffer_id"))              rule_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_buffer_id"]));
        if (p_props.has("rule_domain_masks_buffer_id")) rule_domain_masks_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_domain_masks_buffer_id"]));
        if (p_props.has("required_class_hash"))         required_class_hash         = static_cast<uint32_t>(static_cast<int64_t>(p_props["required_class_hash"]));
        if (p_props.has("required_suffix_hash"))        required_suffix_hash        = static_cast<uint32_t>(static_cast<int64_t>(p_props["required_suffix_hash"]));
        if (p_props.has("required_seme_mask_lo32"))     required_seme_mask_lo32     = static_cast<uint32_t>(static_cast<int64_t>(p_props["required_seme_mask_lo32"]));
        if (p_props.has("threshold"))                   threshold                   = static_cast<float>(static_cast<double>(p_props["threshold"]));
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
        size_t tok_cap = 0, rule_cap = 0, mask_cap = 0;
        const auto *tokens     = _resolve_ptr<const TokenDescriptorPOD>(p_ctx, target_buffer_id, tok_cap);
        const auto *rules      = _resolve_ptr<const GenerationRulePOD>(p_ctx, rule_buffer_id, rule_cap);
        auto *rule_masks       = _resolve_ptr<BitmaskDomain256POD>(p_ctx, rule_domain_masks_buffer_id, mask_cap);

        if (!tokens || tok_cap == 0) return;

        // Populate ADMIT_ARCHETYPE rule domain masks if rule_masks is bound and empty
        if (rules && rule_masks) {
            const size_t n_rules = std::min(rule_cap, mask_cap);
            for (size_t r = 0; r < n_rules; ++r) {
                if (rules[r].operation == GenerationRuleOp::ADMIT_ARCHETYPE && rule_masks[r].empty()) {
                    for (size_t t = 0; t < tok_cap && t < 256u; ++t) {
                        if (rules[r].selector.matches_candidate(tokens[t].signature)) {
                            rule_masks[r].set_bit(static_cast<uint32_t>(t));
                        }
                    }
                }
            }
        }

        GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(target_buffer_id));
        int64_t matched_count = 0;

        for (size_t t = 0; t < tok_cap; ++t) {
            const TokenDescriptorPOD &tok = tokens[t];
            if (required_class_hash != 0u && tok.signature.class_name_hash != required_class_hash) continue;
            if (required_suffix_hash != 0u && tok.signature.name_suffix_hash != required_suffix_hash) continue;
            if ((tok.nuclear_seme_mask_lo32 & required_seme_mask_lo32) != required_seme_mask_lo32) continue;

            bool rule_matched = true;
            if (rules && rule_cap > 0) {
                rule_matched = false;
                for (size_t r = 0; r < rule_cap; ++r) {
                    if (rules[r].selector.matches_candidate(tok.signature)) {
                        rule_matched = true;
                        break;
                    }
                }
            }
            if (!rule_matched) continue;

            if (target_part && target_part->selection.mode == SelectionMode::SPARSE && target_part->selection.data.indices) {
                target_part->selection.data.indices[matched_count] = static_cast<int64_t>(t);
            }
            ++matched_count;
        }

        if (target_part && target_part->selection.mode == SelectionMode::SPARSE) {
            target_part->selection.element_count = matched_count;
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

static_assert(sizeof(ReflectionSelectorMatchQueryLogic<>) == 64, "ReflectionSelectorMatchQueryLogic must be 64 bytes!");
static_assert(IsQueryLogic<ReflectionSelectorMatchQueryLogic<>>, "ReflectionSelectorMatchQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core
