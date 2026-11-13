#pragma once

/**
 * sememe_and_lexicon_morphology_transform_logic.h
 * IdeamMinds Transform Task (`MND12` — Wave 5 Semantics & Lexicon Morphology):
 *
 * ============================================================================
 * ARCHITECTURAL SYNERGY WITH `IdeamLinguistics` & `IdeamCommunications`
 * ============================================================================
 * Bridges `MindDOD` (`AgentMindResource` & Akashic Tome `InformationMatrixResource`)
 * with `IdeamLinguistics` (`SememeDescriptorPOD`, `LexemePOD`, `PhonemePOD` 16-bit
 * articulatory feature masks) and `IdeamCommunications` (`SignalPacketPOD`).
 * When a listener decodes a noise-degraded phoneme stream via
 * `PhoneticHammingLexemeDecodeTransformLogic`, misheard words naturally enter
 * the listener's Reference Catalogue as Honest Misreferences!
 * Ports `Sememe.cs` (`SememicForm`) and `UnityLanguageAsset.cs` (`GetSemanticID`,
 * `UnityLexicon`, `UnityLexeme`, `UnityMorpheme`):
 *   Resolves denotative (`PRIMARY_DENOTATION`, `SECONDARY_DENOTATION`) vs. connotative
 *   (`ANALOGOUS_CONNOTATION`, `EMOTIVE_CONNOTATION`, `EVALUATIVE_CONNOTATION`) `SememicForm`
 *   tags from belief `emotional_charge` and `BeliefCatalogueEntryPod::sememe_id`, and maps
 *   propositions/beliefs into packed `[SememeID | SememicForm | LexemeMorphemeHash]` token
 *   streams in `MIND_SEMEME_TOKEN_STREAM`.
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = uint32_t>
struct alignas(64) SememeAndLexiconMorphologyTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Sememe & Lexicon Morphology";
    static constexpr std::string_view ui_category  = "Minds/Semantics";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                    = INVALID_ID; // 4B
    uint32_t sememe_token_stream_buffer_id       = INVALID_ID; // 4B: uint32_t output token stream
    uint32_t agent_belief_catalogue_entry_buffer_id = INVALID_ID; // 4B: BeliefCatalogueEntryPod (16B AoS)
    uint32_t belief_catalogued_fact_id_buffer_id = INVALID_ID; // 4B: uint32_t catalogued_fact_id
    uint32_t belief_emotional_charge_buffer_id   = INVALID_ID; // 4B: float emotional_charge
    uint32_t belief_flags_buffer_id              = INVALID_ID; // 4B: uint8_t FactFlagBits
    float emotive_connotation_threshold          = 0.60f;      // 4B: Above this emotional_charge -> EMOTIVE_CONNOTATION
    float evaluative_connotation_threshold       = 0.35f;      // 4B: Above this charge -> EVALUATIVE_CONNOTATION
    uint16_t dialect_lect_id                     = 0u;         // 2B: UnityLanguageLect dialect ID offset
    uint8_t reserved_padding[30]                 = {0};        // 30B -> Total = 64B

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
        add_buf("sememe_token_stream_buffer_id");
        add_buf("agent_belief_catalogue_entry_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("belief_emotional_charge_buffer_id");
        add_buf("belief_flags_buffer_id");

        godot::Dictionary emo;
        emo["name"]        = "emotive_connotation_threshold";
        emo["type"]        = godot::Variant::FLOAT;
        emo["hint"]        = godot::PROPERTY_HINT_RANGE;
        emo["hint_string"] = "0.0,1.0,0.01";
        props.push_back(emo);

        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                       target_buffer_id                       = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("sememe_token_stream_buffer_id"))          sememe_token_stream_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_token_stream_buffer_id"]));
        if (p_props.has("agent_belief_catalogue_entry_buffer_id")) agent_belief_catalogue_entry_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_belief_catalogue_entry_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id"))    belief_catalogued_fact_id_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("belief_emotional_charge_buffer_id"))      belief_emotional_charge_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_emotional_charge_buffer_id"]));
        if (p_props.has("belief_flags_buffer_id"))                 belief_flags_buffer_id                 = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_flags_buffer_id"]));
        if (p_props.has("emotive_connotation_threshold"))          emotive_connotation_threshold          = static_cast<float>(static_cast<double>(p_props["emotive_connotation_threshold"]));
        if (p_props.has("evaluative_connotation_threshold"))       evaluative_connotation_threshold       = static_cast<float>(static_cast<double>(p_props["evaluative_connotation_threshold"]));
        if (p_props.has("dialect_lect_id"))                        dialect_lect_id                        = static_cast<uint16_t>(static_cast<int64_t>(p_props["dialect_lect_id"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)              return target_buffer_id;
        if (sememe_token_stream_buffer_id != INVALID_ID) return sememe_token_stream_buffer_id;
        return belief_catalogued_fact_id_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *tok_part  = p_context.get_grant_part(sememe_token_stream_buffer_id);
        const GrantPartPOD *cat_part  = p_context.get_grant_part(agent_belief_catalogue_entry_buffer_id);
        const GrantPartPOD *cfid_part = p_context.get_grant_part(belief_catalogued_fact_id_buffer_id);
        const GrantPartPOD *emo_part  = p_context.get_grant_part(belief_emotional_charge_buffer_id);
        const GrantPartPOD *flg_part  = p_context.get_grant_part(belief_flags_buffer_id);

        if (!tok_part || !tok_part->raw_base_ptr) return;

        auto *tokens          = reinterpret_cast<uint32_t *>(tok_part->raw_base_ptr);
        const int64_t tok_cap = static_cast<int64_t>(tok_part->capacity_bytes / sizeof(uint32_t));
        auto *cat_entries     = (cat_part && cat_part->raw_base_ptr)   ? reinterpret_cast<BeliefCatalogueEntryPod *>(cat_part->raw_base_ptr) : nullptr;
        const auto *cat_fids  = (cfid_part && cfid_part->raw_base_ptr) ? reinterpret_cast<const uint32_t *>(cfid_part->raw_base_ptr)         : nullptr;
        const auto *emos      = (emo_part && emo_part->raw_base_ptr)   ? reinterpret_cast<const float *>(emo_part->raw_base_ptr)            : nullptr;
        const auto *flags     = (flg_part && flg_part->raw_base_ptr)   ? reinterpret_cast<const uint8_t *>(flg_part->raw_base_ptr)          : nullptr;

        const int64_t b_cap   = cat_fids ? static_cast<int64_t>(cfid_part->capacity_bytes / sizeof(uint32_t)) : tok_cap;
        const int64_t count   = std::min(b_cap, tok_cap);

        for (int64_t i = 0; i < count; ++i) {
            const uint32_t fid   = cat_fids ? cat_fids[i] : static_cast<uint32_t>(i + 1);
            const float charge   = emos ? std::abs(emos[i]) : 0.0f;
            const uint8_t flg    = flags ? flags[i] : 0u;

            SememicForm form = SememicForm::PRIMARY_DENOTATION;
            if ((flg & FACT_FLAG_MISREFERENCED) != 0u) {
                form = SememicForm::ANALOGOUS_CONNOTATION;
            } else if (charge >= emotive_connotation_threshold) {
                form = SememicForm::EMOTIVE_CONNOTATION;
            } else if (charge >= evaluative_connotation_threshold) {
                form = SememicForm::EVALUATIVE_CONNOTATION;
            }

            const uint16_t sememe_id = static_cast<uint16_t>((fid & 0xFFFFu) ^ dialect_lect_id);
            if (cat_entries) {
                cat_entries[i].sememe_id = sememe_id;
            }

            // Pack [31..24: SememicForm | 23..16: dialect_lect_id | 15..0: sememe_id]
            tokens[i] = (static_cast<uint32_t>(form) << 24u) |
                        (static_cast<uint32_t>(dialect_lect_id & 0xFFu) << 16u) |
                        static_cast<uint32_t>(sememe_id);
        }
    }
};

static_assert(sizeof(SememeAndLexiconMorphologyTransformLogic<>) == 64, "SememeAndLexiconMorphologyTransformLogic must be 64 bytes!");

} // namespace ideam::core
