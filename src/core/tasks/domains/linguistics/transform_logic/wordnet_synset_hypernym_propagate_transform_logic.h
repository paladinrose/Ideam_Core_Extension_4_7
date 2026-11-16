#pragma once

/**
 * wordnet_synset_hypernym_propagate_transform_logic.h
 * IdeamLinguistics Transform Task (`LNG2` — Wave 1 WordNet Synset Ontology Propagation):
 *
 * Iterates directed `SynsetRelationPOD` edges (`WN_HYPERNYM`, `WN_PART_HOLONYM`, `WN_ENTAILMENT`)
 * across `max_propagation_hops` passes, propagating symbolic nuclear ontology invariants
 * (`nuclear_seme_words[0]`) from hypernym (`target_sememe_id`) to hyponym (`source_sememe_id`)
 * in contiguous DOD memory and refreshing `LING_SEMEME_NUCLEAR_SYMBOLIC_WORD0` and
 * `LING_SEMEME_COHERENCE_MASK32`.
 *
 * Ensures that if `sword` is a hyponym of `weapon` -> `item` -> `prop`, then in DOD memory
 * `sword` automatically inherits the `weapon`, `item`, and `prop` symbolic ontology bits!
 */

#include "../../../../linguistics/linguistics_common.h"
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
struct alignas(64) WordNetSynsetHypernymPropagateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "WordNet Synset Hypernym Propagate";
    static constexpr std::string_view ui_category  = "Linguistics/Ontology";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                        = INVALID_ID; // 4B
    uint32_t sememe_descriptor_pod_buffer_id         = INVALID_ID; // 4B: SememeDescriptorPOD (64B AoS)
    uint32_t synset_relation_pod_buffer_id           = INVALID_ID; // 4B: SynsetRelationPOD (16B AoS)
    uint32_t sememe_nuclear_symbolic_word0_buffer_id = INVALID_ID; // 4B: uint64_t SOA symbolic word 0
    uint32_t sememe_coherence_mask32_buffer_id       = INVALID_ID; // 4B: uint32_t SOA coherence mask
    uint8_t  max_propagation_hops                    = 4u;         // 1B: Transitive hypernym depth
    bool     propagate_holonyms_to_peripheral        = true;       // 1B: Propagate holonym nuclear bits into meronym peripheral_seme_words[0]
    uint8_t  reserved_padding[42]                    = {0};        // 42B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char *p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("sememe_descriptor_pod_buffer_id");
        add_buf("synset_relation_pod_buffer_id");
        add_buf("sememe_nuclear_symbolic_word0_buffer_id");
        add_buf("sememe_coherence_mask32_buffer_id");

        godot::Dictionary hops;
        hops["name"]        = "max_propagation_hops";
        hops["type"]        = godot::Variant::INT;
        hops["hint"]        = godot::PROPERTY_HINT_RANGE;
        hops["hint_string"] = "1,12,1";
        props.push_back(hops);

        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                return target_buffer_id;
        if (sememe_descriptor_pod_buffer_id != INVALID_ID) return sememe_descriptor_pod_buffer_id;
        return synset_relation_pod_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                        target_buffer_id                        = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("sememe_descriptor_pod_buffer_id"))         sememe_descriptor_pod_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_descriptor_pod_buffer_id"]));
        if (p_props.has("synset_relation_pod_buffer_id"))           synset_relation_pod_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["synset_relation_pod_buffer_id"]));
        if (p_props.has("sememe_nuclear_symbolic_word0_buffer_id")) sememe_nuclear_symbolic_word0_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_nuclear_symbolic_word0_buffer_id"]));
        if (p_props.has("sememe_coherence_mask32_buffer_id"))       sememe_coherence_mask32_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_coherence_mask32_buffer_id"]));
        if (p_props.has("max_propagation_hops"))                    max_propagation_hops                    = static_cast<uint8_t>(std::clamp<int64_t>(p_props["max_propagation_hops"], 1, 16));
        if (p_props.has("propagate_holonyms_to_peripheral"))        propagate_holonyms_to_peripheral        = static_cast<bool>(p_props["propagate_holonyms_to_peripheral"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *sem_part = p_ctx.get_grant_part(sememe_descriptor_pod_buffer_id);
        const GrantPartPOD *rel_part = p_ctx.get_grant_part(synset_relation_pod_buffer_id);
        const GrantPartPOD *sym_part = p_ctx.get_grant_part(sememe_nuclear_symbolic_word0_buffer_id);
        const GrantPartPOD *coh_part = p_ctx.get_grant_part(sememe_coherence_mask32_buffer_id);

        if (!sem_part || !sem_part->raw_base_ptr || !rel_part || !rel_part->raw_base_ptr) {
            return;
        }

        auto *sememes         = reinterpret_cast<SememeDescriptorPOD *>(sem_part->raw_base_ptr);
        const size_t sem_cap  = sem_part->capacity_bytes / sizeof(SememeDescriptorPOD);
        const auto *rels      = reinterpret_cast<const SynsetRelationPOD *>(rel_part->raw_base_ptr);
        const size_t rel_cap  = (rel_part->selection.element_count > 0)
            ? std::min<size_t>(static_cast<size_t>(rel_part->selection.element_count), rel_part->capacity_bytes / sizeof(SynsetRelationPOD))
            : (rel_part->capacity_bytes / sizeof(SynsetRelationPOD));

        const uint8_t hops = std::max<uint8_t>(1u, max_propagation_hops);
        for (uint8_t h = 0; h < hops; ++h) {
            bool any_changed = false;
            for (size_t r = 0; r < rel_cap; ++r) {
                const SynsetRelationPOD &edge = rels[r];
                if (edge.source_sememe_id >= sem_cap || edge.target_sememe_id >= sem_cap) {
                    continue;
                }
                if (edge.relation == WordNetRelationType::WN_HYPERNYM ||
                    edge.relation == WordNetRelationType::WN_ENTAILMENT) {
                    const uint64_t parent_bits = sememes[edge.target_sememe_id].nuclear_seme_words[0];
                    const uint64_t before      = sememes[edge.source_sememe_id].nuclear_seme_words[0];
                    const uint64_t after       = before | parent_bits;
                    if (after != before) {
                        sememes[edge.source_sememe_id].nuclear_seme_words[0] = after;
                        any_changed = true;
                    }
                } else if (propagate_holonyms_to_peripheral &&
                           edge.relation == WordNetRelationType::WN_PART_HOLONYM) {
                    const uint64_t whole_bits = sememes[edge.target_sememe_id].nuclear_seme_words[0];
                    sememes[edge.source_sememe_id].peripheral_seme_words[0] |= whole_bits;
                }
            }
            if (!any_changed) break;
        }

        // Synchronize updated symbolic word0 & 32-bit coherence masks back to SOA columns
        auto *sym_col = (sym_part && sym_part->raw_base_ptr) ? reinterpret_cast<uint64_t *>(sym_part->raw_base_ptr) : nullptr;
        auto *coh_col = (coh_part && coh_part->raw_base_ptr) ? reinterpret_cast<uint32_t *>(coh_part->raw_base_ptr) : nullptr;
        const size_t sym_cap = sym_col ? (sym_part->capacity_bytes / sizeof(uint64_t)) : 0u;
        const size_t coh_cap = coh_col ? (coh_part->capacity_bytes / sizeof(uint32_t)) : 0u;

        for (size_t i = 0; i < sem_cap; ++i) {
            if (sym_col && i < sym_cap) sym_col[i] = sememes[i].nuclear_seme_words[0];
            if (coh_col && i < coh_cap) coh_col[i] = sememes[i].compute_coherence_trait_mask32();
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(WordNetSynsetHypernymPropagateTransformLogic<>) == 64, "WordNetSynsetHypernymPropagateTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<WordNetSynsetHypernymPropagateTransformLogic<>>, "WordNetSynsetHypernymPropagateTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
