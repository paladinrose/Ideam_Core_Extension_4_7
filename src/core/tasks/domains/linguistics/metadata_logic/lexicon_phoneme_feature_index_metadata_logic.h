#pragma once

/**
 * lexicon_phoneme_feature_index_metadata_logic.h
 * IdeamLinguistics Metadata Task (`LNG1` — Wave 1 Lexicon, UPOS & PanPhon Feature Indexing):
 *
 * Synchronizes SOA columns (`LING_SEMEME_NUCLEAR_SYMBOLIC_WORD0`, `LING_SEMEME_MRL_BINARY_LO64`,
 * `LING_SEMEME_MRL_BINARY_HI64`, `LING_SEMEME_COHERENCE_MASK32`, `LING_SEMEME_UPOS_AND_DOMAIN`)
 * from `SememeDescriptorPOD` / `LinguisticProfilePOD` rows and partitions `LexemePOD` /
 * `SememeDescriptorPOD` selections into LOD priority tiers (`r_selection.lod_levels`) based on
 * dialect `active_lect_id`, Universal Dependencies `UniversalPOS` (`UPOS`), and PanPhon
 * articulatory feature density.
 */

#include "../../../../linguistics/linguistics_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
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
struct alignas(64) LexiconPhonemeFeatureIndexMetadataLogic {
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

    static constexpr std::string_view display_name = "Lexicon Phoneme Feature & Lect Partition";
    static constexpr std::string_view ui_category  = "Linguistics/Indexing";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                      = INVALID_ID; // 4B
    uint32_t sememe_descriptor_pod_buffer_id       = INVALID_ID; // 4B: SememeDescriptorPOD / LinguisticProfilePOD (64B AoS)
    uint32_t sememe_nuclear_symbolic_word0_buffer_id = INVALID_ID; // 4B: uint64_t SOA symbolic ontology word 0
    uint32_t sememe_mrl_binary_lo64_buffer_id      = INVALID_ID; // 4B: uint64_t SOA MRL-128 binary low 64 bits
    uint32_t sememe_mrl_binary_hi64_buffer_id      = INVALID_ID; // 4B: uint64_t SOA MRL-128 binary high 64 bits
    uint32_t sememe_coherence_mask32_buffer_id     = INVALID_ID; // 4B: uint32_t SOA 32-bit WFC/Quantum Darwinism mask
    uint32_t sememe_upos_and_domain_buffer_id      = INVALID_ID; // 4B: uint32_t SOA [UPOS | SememicForm | DomainID]
    uint32_t lexeme_pod_buffer_id                  = INVALID_ID; // 4B: LexemePOD (32B AoS)
    uint32_t active_lect_id                        = 0u;         // 4B: Preferred dialect / sociolect ID
    uint16_t allowed_upos_mask                     = 0xFFFFu;    // 2B: Bitmask of UniversalPOS categories
    bool sync_soa_columns_from_pod                 = true;       // 1B: Extract SOA columns from 64B AoS descriptor
    bool write_lod_tiers_to_selection              = true;       // 1B: Stamp LOD tiers (1..4) into selection lod_levels
    uint8_t reserved_padding[24]                   = {0};        // 24B -> Total = 64B

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
        add_buf("sememe_nuclear_symbolic_word0_buffer_id");
        add_buf("sememe_mrl_binary_lo64_buffer_id");
        add_buf("sememe_mrl_binary_hi64_buffer_id");
        add_buf("sememe_coherence_mask32_buffer_id");
        add_buf("sememe_upos_and_domain_buffer_id");
        add_buf("lexeme_pod_buffer_id");

        godot::Dictionary lect;
        lect["name"] = "active_lect_id";
        lect["type"] = godot::Variant::INT;
        props.push_back(lect);

        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                return target_buffer_id;
        if (sememe_descriptor_pod_buffer_id != INVALID_ID) return sememe_descriptor_pod_buffer_id;
        return sememe_coherence_mask32_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                        target_buffer_id                        = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("sememe_descriptor_pod_buffer_id"))         sememe_descriptor_pod_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_descriptor_pod_buffer_id"]));
        if (p_props.has("sememe_nuclear_symbolic_word0_buffer_id")) sememe_nuclear_symbolic_word0_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_nuclear_symbolic_word0_buffer_id"]));
        if (p_props.has("sememe_mrl_binary_lo64_buffer_id"))        sememe_mrl_binary_lo64_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_mrl_binary_lo64_buffer_id"]));
        if (p_props.has("sememe_mrl_binary_hi64_buffer_id"))        sememe_mrl_binary_hi64_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_mrl_binary_hi64_buffer_id"]));
        if (p_props.has("sememe_coherence_mask32_buffer_id"))       sememe_coherence_mask32_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_coherence_mask32_buffer_id"]));
        if (p_props.has("sememe_upos_and_domain_buffer_id"))        sememe_upos_and_domain_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_upos_and_domain_buffer_id"]));
        if (p_props.has("lexeme_pod_buffer_id"))                    lexeme_pod_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["lexeme_pod_buffer_id"]));
        if (p_props.has("active_lect_id"))                          active_lect_id                          = static_cast<uint32_t>(static_cast<int64_t>(p_props["active_lect_id"]));
        if (p_props.has("allowed_upos_mask"))                       allowed_upos_mask                       = static_cast<uint16_t>(static_cast<int64_t>(p_props["allowed_upos_mask"]));
        if (p_props.has("sync_soa_columns_from_pod"))               sync_soa_columns_from_pod               = static_cast<bool>(p_props["sync_soa_columns_from_pod"]);
        if (p_props.has("write_lod_tiers_to_selection"))            write_lod_tiers_to_selection            = static_cast<bool>(p_props["write_lod_tiers_to_selection"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *pod_part  = p_ctx.get_grant_part(sememe_descriptor_pod_buffer_id);
        const GrantPartPOD *sym_part  = p_ctx.get_grant_part(sememe_nuclear_symbolic_word0_buffer_id);
        const GrantPartPOD *mrl0_part = p_ctx.get_grant_part(sememe_mrl_binary_lo64_buffer_id);
        const GrantPartPOD *mrl1_part = p_ctx.get_grant_part(sememe_mrl_binary_hi64_buffer_id);
        const GrantPartPOD *coh_part  = p_ctx.get_grant_part(sememe_coherence_mask32_buffer_id);
        const GrantPartPOD *upos_part = p_ctx.get_grant_part(sememe_upos_and_domain_buffer_id);
        const GrantPartPOD *lex_part  = p_ctx.get_grant_part(lexeme_pod_buffer_id);

        if (pod_part && pod_part->raw_base_ptr) {
            const auto *pods   = reinterpret_cast<const SememeDescriptorPOD *>(pod_part->raw_base_ptr);
            const size_t count = pod_part->capacity_bytes / sizeof(SememeDescriptorPOD);

            auto *sym_col  = (sym_part  && sym_part->raw_base_ptr)  ? reinterpret_cast<uint64_t *>(sym_part->raw_base_ptr)  : nullptr;
            auto *mrl0_col = (mrl0_part && mrl0_part->raw_base_ptr) ? reinterpret_cast<uint64_t *>(mrl0_part->raw_base_ptr) : nullptr;
            auto *mrl1_col = (mrl1_part && mrl1_part->raw_base_ptr) ? reinterpret_cast<uint64_t *>(mrl1_part->raw_base_ptr) : nullptr;
            auto *coh_col  = (coh_part  && coh_part->raw_base_ptr)  ? reinterpret_cast<uint32_t *>(coh_part->raw_base_ptr)  : nullptr;
            auto *upos_col = (upos_part && upos_part->raw_base_ptr) ? reinterpret_cast<uint32_t *>(upos_part->raw_base_ptr) : nullptr;

            const size_t sym_cap  = sym_col  ? (sym_part->capacity_bytes  / sizeof(uint64_t)) : 0u;
            const size_t mrl0_cap = mrl0_col ? (mrl0_part->capacity_bytes / sizeof(uint64_t)) : 0u;
            const size_t mrl1_cap = mrl1_col ? (mrl1_part->capacity_bytes / sizeof(uint64_t)) : 0u;
            const size_t coh_cap  = coh_col  ? (coh_part->capacity_bytes  / sizeof(uint32_t)) : 0u;
            const size_t upos_cap = upos_col ? (upos_part->capacity_bytes / sizeof(uint32_t)) : 0u;

            if (sync_soa_columns_from_pod) {
                for (size_t i = 0; i < count; ++i) {
                    const SememeDescriptorPOD &d = pods[i];
                    if (sym_col  && i < sym_cap)  sym_col[i]  = d.nuclear_seme_words[0];
                    if (mrl0_col && i < mrl0_cap) mrl0_col[i] = d.nuclear_seme_words[1];
                    if (mrl1_col && i < mrl1_cap) mrl1_col[i] = d.nuclear_seme_words[2];
                    if (coh_col  && i < coh_cap)  coh_col[i]  = d.compute_coherence_trait_mask32();
                    if (upos_col && i < upos_cap) {
                        upos_col[i] = static_cast<uint32_t>(d.taxeme_category) |
                                      (static_cast<uint32_t>(d.sememic_form) << 8u) |
                                      (static_cast<uint32_t>(d.primary_domain_id) << 16u);
                    }
                }
            }
        }

        // Optional LOD tier stamping on target selection
        const GrantPartPOD *tgt_part = p_ctx.get_grant_part(get_target_buffer_id());
        if (write_lod_tiers_to_selection && tgt_part && tgt_part->selection.lod_levels) {
            const int64_t sel_cap = tgt_part->selection.capacity;
            const auto *lex_pods  = (lex_part && lex_part->raw_base_ptr)
                ? reinterpret_cast<const LexemePOD *>(lex_part->raw_base_ptr)
                : nullptr;
            const int64_t lex_cap = lex_pods
                ? static_cast<int64_t>(lex_part->capacity_bytes / sizeof(LexemePOD))
                : 0;

            for (int64_t i = 0; i < sel_cap; ++i) {
                uint8_t tier = 1u;
                if (lex_pods && i < lex_cap) {
                    const LexemePOD &lx = lex_pods[i];
                    const bool lect_match = (lx.lect_id == static_cast<uint16_t>(active_lect_id));
                    const uint16_t cat_bit = (static_cast<uint8_t>(lx.category) < 16u)
                        ? static_cast<uint16_t>(1u << static_cast<uint8_t>(lx.category))
                        : 1u;
                    const bool upos_match = (allowed_upos_mask & cat_bit) != 0u;
                    if (lect_match && upos_match) tier = 0u;      // Tier 0: Exact dialect & UPOS match
                    else if (upos_match)          tier = 1u;      // Tier 1: Fallback dialect, valid UPOS
                    else                          tier = 2u;      // Tier 2: Filtered UPOS
                }
                tgt_part->selection.lod_levels[i] = tier;
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute(
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

static_assert(sizeof(LexiconPhonemeFeatureIndexMetadataLogic<>) == 64, "LexiconPhonemeFeatureIndexMetadataLogic must be 64 bytes!");
static_assert(IsMetadataLogic<LexiconPhonemeFeatureIndexMetadataLogic<>>, "LexiconPhonemeFeatureIndexMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
