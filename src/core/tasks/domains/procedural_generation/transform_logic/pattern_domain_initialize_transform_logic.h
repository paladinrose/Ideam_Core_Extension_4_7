#pragma once

/**
 * pattern_domain_initialize_transform_logic.h
 * `PCG3`: 64-byte cache-aligned `TransformLogic` kernel for `IdeamProceduralGeneration`
 * (`src/core/tasks/domains/procedural_generation/transform_logic/`).
 *
 * Initializes `EnvironmentSlotPOD::domain` (`BitmaskDomain256POD`) across all
 * slots from compiled `ADMIT_ARCHETYPE` rules (`CASCADE_RULE_POD` +
 * `RULE_TOKEN_DOMAIN_MASKS`), computes initial `SLOT_ENTROPY_FLOATS`, and
 * initializes `POINTER_OR_CONFLICT_MASK` (`PointerOrConflictSlotFlags`).
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
struct alignas(64) PatternDomainInitializeTransformLogic {
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

    static constexpr std::string_view display_name = "PCG3: ProceduralPattern Domain & Hierarchy Initialize";
    static constexpr std::string_view ui_category  = "ProceduralGeneration/Domain";

    // --- 64-Byte Cache-Line Member Layout ---
    uint32_t target_buffer_id            = INVALID_ID; ///< Offset 0  (4B): `ENVIRONMENT_SLOT_POD`
    uint32_t rule_buffer_id              = INVALID_ID; ///< Offset 4  (4B): `CASCADE_RULE_POD`
    uint32_t pointer_buffer_id           = INVALID_ID; ///< Offset 8  (4B): `POINTER_OR_CONFLICT_MASK`
    uint32_t token_descriptor_buffer_id  = INVALID_ID; ///< Offset 12 (4B): `TOKEN_DESCRIPTOR_POD`
    uint32_t rule_domain_masks_buffer_id = INVALID_ID; ///< Offset 16 (4B): `RULE_TOKEN_DOMAIN_MASKS`
    uint32_t slot_entropy_buffer_id      = INVALID_ID; ///< Offset 20 (4B): `SLOT_ENTROPY_FLOATS`
    uint32_t fallback_token_count        = 16;         ///< Offset 24 (4B): Default token count if no token buffer bound
    uint8_t  preserve_pinned_slots       = 1;          ///< Offset 28 (1B): Keep pre-collapsed / pinned slots intact
    uint8_t  reserved_padding[35]        = {0};        ///< Offset 29 (35B) -> Total = 64 Bytes

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",            godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("rule_buffer_id",              godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_buffer_id",           godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("token_descriptor_buffer_id",  godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("rule_domain_masks_buffer_id", godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("slot_entropy_buffer_id",      godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("fallback_token_count",        godot::Variant::INT,  static_cast<int64_t>(16));
        add_prop("preserve_pinned_slots",       godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))            target_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("rule_buffer_id"))              rule_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_buffer_id"]));
        if (p_props.has("pointer_buffer_id"))           pointer_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_buffer_id"]));
        if (p_props.has("token_descriptor_buffer_id"))  token_descriptor_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["token_descriptor_buffer_id"]));
        if (p_props.has("rule_domain_masks_buffer_id")) rule_domain_masks_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rule_domain_masks_buffer_id"]));
        if (p_props.has("slot_entropy_buffer_id"))      slot_entropy_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["slot_entropy_buffer_id"]));
        if (p_props.has("fallback_token_count"))        fallback_token_count        = static_cast<uint32_t>(static_cast<int64_t>(p_props["fallback_token_count"]));
        if (p_props.has("preserve_pinned_slots"))       preserve_pinned_slots       = static_cast<bool>(p_props["preserve_pinned_slots"]) ? 1u : 0u;
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
        size_t slot_cap = 0, rule_cap = 0, ptr_cap = 0, tok_cap = 0, mask_cap = 0, ent_cap = 0;
        auto *slots            = _resolve_ptr<EnvironmentSlotPOD>(p_ctx, target_buffer_id, slot_cap);
        const auto *rules      = _resolve_ptr<const GenerationRulePOD>(p_ctx, rule_buffer_id, rule_cap);
        auto *ptr_masks        = _resolve_ptr<uint32_t>(p_ctx, pointer_buffer_id, ptr_cap);
        const auto *tokens     = _resolve_ptr<const TokenDescriptorPOD>(p_ctx, token_descriptor_buffer_id, tok_cap);
        const auto *rule_masks = _resolve_ptr<const BitmaskDomain256POD>(p_ctx, rule_domain_masks_buffer_id, mask_cap);
        auto *entropy_out      = _resolve_ptr<float>(p_ctx, slot_entropy_buffer_id, ent_cap);

        if (!slots || slot_cap == 0) return;

        // Build union of ADMIT_ARCHETYPE masks
        BitmaskDomain256POD admitted_domain{};
        bool has_admit_rule = false;
        if (rules && rule_cap > 0) {
            for (size_t r = 0; r < rule_cap; ++r) {
                if (rules[r].operation != GenerationRuleOp::ADMIT_ARCHETYPE) continue;
                has_admit_rule = true;
                if (rule_masks && rules[r].target_token_mask_index < mask_cap &&
                    !rule_masks[rules[r].target_token_mask_index].empty()) {
                    admitted_domain.union_with(rule_masks[rules[r].target_token_mask_index]);
                } else if (tokens && tok_cap > 0) {
                    for (size_t t = 0; t < tok_cap && t < 256u; ++t) {
                        if (rules[r].selector.matches_candidate(tokens[t].signature)) {
                            admitted_domain.set_bit(static_cast<uint32_t>(t));
                        }
                    }
                }
            }
        }
        if (!has_admit_rule) {
            const uint32_t n_tok = (tok_cap > 0) ? static_cast<uint32_t>(tok_cap) : fallback_token_count;
            admitted_domain.fill_first_n(std::min<uint32_t>(256u, n_tok));
        }

        for (size_t s = 0; s < slot_cap; ++s) {
            EnvironmentSlotPOD &slot = slots[s];
            const bool is_pinned = (preserve_pinned_slots != 0u &&
                                    (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE ||
                                     slot.collapsed_token_id >= 0));
            if (is_pinned && slot.collapsed_token_id >= 0) {
                slot.domain.clear();
                slot.domain.set_bit(static_cast<uint32_t>(slot.collapsed_token_id));
            } else {
                slot.domain = admitted_domain;
            }

            const uint32_t pop = slot.domain.popcount();
            slot.collapsed_token_id = (pop == 1u) ? slot.domain.first_set_state() : -1;
            slot.has_conflict       = (pop == 0u) ? 1u : 0u;
            slot.entropy_bucket     = static_cast<uint8_t>(std::min<uint32_t>(15u, pop));

            if (entropy_out && s < ent_cap) {
                entropy_out[s] = slot.domain.shannon_entropy();
            }
            if (ptr_masks && s < ptr_cap) {
                uint32_t flags = SLOT_STATUS_CLEAN;
                if (slot.is_pointer_state != 0u) flags |= SLOT_STATUS_POINTER_STATE;
                if (slot.has_conflict != 0u)     flags |= SLOT_STATUS_HAS_CONFLICT;
                if (pop == 1u)                   flags |= SLOT_STATUS_COLLAPSED;
                if (is_pinned)                   flags |= SLOT_STATUS_PINNED;
                ptr_masks[s] = flags;
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(PatternDomainInitializeTransformLogic<>) == 64, "PatternDomainInitializeTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<PatternDomainInitializeTransformLogic<>>, "PatternDomainInitializeTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
