#pragma once

/**
 * cascade_rule_compile_metadata_logic.h
 * `PCG1`: 64-byte cache-aligned `MetadataLogic` kernel for `IdeamProceduralGeneration`
 * (`src/core/tasks/domains/procedural_generation/metadata_logic/`).
 *
 * Computes CSS selector specificity (`!important > PatternCascadeLayer > priority >
 * ID/Name match > Prefix/Suffix > Property/Method/Signal signature > Group/Sememe tag >
 * Class type`) for `GenerationRulePOD` buffers, sorts them in cascade order, binds
 * `TokenDescriptorPOD::coherence_trait_mask` + `TOKEN_COHERENCE_MASKS` for
 * `IdeamQuantumDarwinism`, and stamps `PatternCascadeLayer` into `r_selection.lod_levels`.
 */

#include "../../../../procedural_generation/procedural_environment.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) CascadeRuleCompileMetadataLogic {
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

    static constexpr std::string_view display_name = "PCG1: Cascade Rule Specificity & Layer Compile";
    static constexpr std::string_view ui_category  = "ProceduralGeneration/Cascade";

    // --- 64-Byte Cache-Line Member Layout ---
    uint32_t target_buffer_id          = INVALID_ID; ///< Offset 0  (4B): `CASCADE_RULE_POD`
    uint32_t metadata_buffer_id        = INVALID_ID; ///< Offset 4  (4B): Legacy alias / `CASCADE_RULE_POD`
    uint32_t token_descriptor_buffer_id= INVALID_ID; ///< Offset 8  (4B): `TOKEN_DESCRIPTOR_POD`
    uint32_t token_coherence_buffer_id = INVALID_ID; ///< Offset 12 (4B): `TOKEN_COHERENCE_MASKS`
    uint32_t max_horizon_radius        = 4;          ///< Offset 16 (4B): Quantum Darwinism horizon hop radius
    uint8_t  sort_rules_in_place       = 1;          ///< Offset 20 (1B): 1 = sort rules by CSS cascade precedence
    uint8_t  reserved_padding[43]      = {0};        ///< Offset 21 (43B) -> Total = 64 Bytes

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",           godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("token_descriptor_buffer_id", godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("token_coherence_buffer_id",  godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("max_horizon_radius",         godot::Variant::INT,  static_cast<int64_t>(4));
        add_prop("sort_rules_in_place",        godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))           target_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("metadata_buffer_id"))         metadata_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["metadata_buffer_id"]));
        if (p_props.has("token_descriptor_buffer_id")) token_descriptor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["token_descriptor_buffer_id"]));
        if (p_props.has("token_coherence_buffer_id"))  token_coherence_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["token_coherence_buffer_id"]));
        if (p_props.has("max_horizon_radius"))         max_horizon_radius         = static_cast<uint32_t>(static_cast<int64_t>(p_props["max_horizon_radius"]));
        if (p_props.has("sort_rules_in_place"))        sort_rules_in_place        = static_cast<bool>(p_props["sort_rules_in_place"]) ? 1u : 0u;
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
        const uint32_t rule_buf = (metadata_buffer_id != INVALID_ID) ? metadata_buffer_id : target_buffer_id;
        size_t rule_cap = 0, tok_cap = 0, coh_cap = 0;
        auto *rules         = _resolve_ptr<GenerationRulePOD>(p_ctx, rule_buf, rule_cap);
        auto *tokens        = _resolve_ptr<TokenDescriptorPOD>(p_ctx, token_descriptor_buffer_id, tok_cap);
        auto *coh_masks_out = _resolve_ptr<uint32_t>(p_ctx, token_coherence_buffer_id, coh_cap);

        if (rules && rule_cap > 0) {
            for (size_t i = 0; i < rule_cap; ++i) {
                rules[i].specificity = CascadeRuleEngine::compute_generation_specificity(rules[i]);
            }
            if (sort_rules_in_place != 0u) {
                CascadeRuleEngine::sort_generation_rules_by_cascade(std::span<GenerationRulePOD>(rules, rule_cap));
            }
        }

        if (tokens && tok_cap > 0) {
            for (size_t t = 0; t < tok_cap; ++t) {
                TokenDescriptorPOD &tok = tokens[t];
                uint32_t mask = tok.coherence_trait_mask | tok.nuclear_seme_mask_lo32 | tok.signature.group_or_tag_mask;
                if (rules) {
                    for (size_t r = 0; r < rule_cap; ++r) {
                        if (rules[r].selector.matches_candidate(tok.signature)) {
                            mask |= (1u << static_cast<uint32_t>(rules[r].layer));
                        }
                    }
                }
                tok.coherence_trait_mask = mask;
                if (coh_masks_out && t < coh_cap) {
                    coh_masks_out[t] = mask;
                }
            }
        }

        if (GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(target_buffer_id))) {
            MemoryBufferSelectionPOD &sel = target_part->selection;
            if (sel.lod_levels != nullptr && rules != nullptr) {
                const size_t n = std::min<size_t>(rule_cap, static_cast<size_t>(std::max<int64_t>(0, sel.element_count)));
                for (size_t i = 0; i < n; ++i) {
                    sel.lod_levels[i] = static_cast<uint8_t>(rules[i].layer);
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_ctx,
        T_ActiveView &p_view
    ) const noexcept {
        (void)r_selection;
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_metadata(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(CascadeRuleCompileMetadataLogic<>) == 64, "CascadeRuleCompileMetadataLogic must be 64 bytes!");
static_assert(IsMetadataLogic<CascadeRuleCompileMetadataLogic<>>, "CascadeRuleCompileMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
