#pragma once

/**
 * semantic_frame_and_slm_mask_transform_logic.h
 * IdeamLinguistics Transform Task (`LNG7` — Wave 4 FrameNet Role Validation & Constrained SLM Mask):
 *
 * 1. Evaluates staged `FrameClauseInstancePOD` rows `(frame_id, agent_sememe_id, patient_sememe_id,
 *    instrument_sememe_id)` against `SemanticFramePOD` selectional restrictions
 *    (`required_agent_seme_mask32`, `required_patient_seme_mask32`, `required_instrument_mask32`),
 *    computing `role_fit_score` and optionally inflicting `consequent_state_seme_mask32` onto the
 *    Patient `SememeDescriptorPOD` when `role_fit_score >= role_fit_threshold`.
 * 2. Compiles `LING_SLM_VOCAB_LOGIT_MASK` (allowed BPE token IDs for `XGrammar` / `llguidance` /
 *    `llama.cpp` constrained decoding) from `LexemePOD` rows matching `active_lect_id`,
 *    `allowed_upos_mask`, and `LING_SEMEME_AFFINITY_SCORE`.
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

/**
 * FrameClauseInstancePOD (32 Bytes, alignas(16))
 * Represents a parsed or staged predicate-argument clause `(Frame, Agent, Patient, Instrument)`
 * evaluated in batch by `SemanticFrameAndSLMMaskTransformLogic`.
 */
struct alignas(16) FrameClauseInstancePOD {
    uint32_t frame_id             = 0;          // 4B: Index in LING_SEMANTIC_FRAME_POD
    uint32_t agent_sememe_id      = 0xFFFFFFFFu;// 4B: ARG0 Agent index in LING_SEMEME_DESCRIPTOR_POD
    uint32_t patient_sememe_id    = 0xFFFFFFFFu;// 4B: ARG1 Patient/Theme index in LING_SEMEME_DESCRIPTOR_POD
    uint32_t instrument_sememe_id = 0xFFFFFFFFu;// 4B: ARG2 Instrument index in LING_SEMEME_DESCRIPTOR_POD
    float    role_fit_score       = 0.0f;       // 4B: Output [0.0 .. 1.0] selectional fit score
    uint32_t consequent_applied   = 0;          // 4B: 1 if consequent_state_seme_mask32 was applied to Patient
    uint32_t location_sememe_id   = 0xFFFFFFFFu;// 4B: Optional ARG_LOC location sememe index
    uint32_t reserved             = 0;          // 4B -> 32B total
};
static_assert(sizeof(FrameClauseInstancePOD) == 32, "FrameClauseInstancePOD must be 32 bytes.");

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SemanticFrameAndSLMMaskTransformLogic {
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

    static constexpr std::string_view display_name = "Semantic Frame Role Fit & SLM Logit Mask";
    static constexpr std::string_view ui_category  = "Linguistics/Semantics";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                = INVALID_ID; // 4B
    uint32_t frame_instance_pod_buffer_id    = INVALID_ID; // 4B: FrameClauseInstancePOD (32B AoS)
    uint32_t semantic_frame_pod_buffer_id    = INVALID_ID; // 4B: SemanticFramePOD (32B AoS)
    uint32_t sememe_descriptor_pod_buffer_id = INVALID_ID; // 4B: SememeDescriptorPOD (64B AoS)
    uint32_t lexeme_pod_buffer_id            = INVALID_ID; // 4B: LexemePOD (32B AoS)
    uint32_t sememe_affinity_score_buffer_id = INVALID_ID; // 4B: float affinity scores from Wave 2
    uint32_t slm_vocab_logit_mask_buffer_id  = INVALID_ID; // 4B: uint32_t output allowed BPE token IDs
    float    role_fit_threshold              = 0.65f;      // 4B: Minimum fit score to apply consequent state semes
    float    min_slm_affinity_floor          = 0.15f;      // 4B: Minimum sememe affinity to allow BPE token in SLM mask
    uint16_t active_lect_id                  = 0u;         // 2B: Preferred dialect ID for SLM vocabulary
    uint16_t allowed_upos_mask               = 0xFFFFu;    // 2B: Allowed next-token UniversalPOS mask
    bool     apply_consequent_state_to_patient = true;     // 1B: Mutate Patient nuclear_seme_words[0] on valid frame execution
    uint8_t  reserved_padding[23]            = {0};        // 23B -> Total = 64B

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
        add_buf("frame_instance_pod_buffer_id");
        add_buf("semantic_frame_pod_buffer_id");
        add_buf("sememe_descriptor_pod_buffer_id");
        add_buf("lexeme_pod_buffer_id");
        add_buf("sememe_affinity_score_buffer_id");
        add_buf("slm_vocab_logit_mask_buffer_id");

        godot::Dictionary thr;
        thr["name"]        = "role_fit_threshold";
        thr["type"]        = godot::Variant::FLOAT;
        thr["hint"]        = godot::PROPERTY_HINT_RANGE;
        thr["hint_string"] = "0.0,1.0,0.01";
        props.push_back(thr);

        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)               return target_buffer_id;
        if (frame_instance_pod_buffer_id != INVALID_ID)   return frame_instance_pod_buffer_id;
        return slm_vocab_logit_mask_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                 target_buffer_id                  = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("frame_instance_pod_buffer_id"))     frame_instance_pod_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["frame_instance_pod_buffer_id"]));
        if (p_props.has("semantic_frame_pod_buffer_id"))     semantic_frame_pod_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["semantic_frame_pod_buffer_id"]));
        if (p_props.has("sememe_descriptor_pod_buffer_id"))  sememe_descriptor_pod_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_descriptor_pod_buffer_id"]));
        if (p_props.has("lexeme_pod_buffer_id"))             lexeme_pod_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["lexeme_pod_buffer_id"]));
        if (p_props.has("sememe_affinity_score_buffer_id"))  sememe_affinity_score_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_affinity_score_buffer_id"]));
        if (p_props.has("slm_vocab_logit_mask_buffer_id"))   slm_vocab_logit_mask_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["slm_vocab_logit_mask_buffer_id"]));
        if (p_props.has("role_fit_threshold"))               role_fit_threshold                = static_cast<float>(static_cast<double>(p_props["role_fit_threshold"]));
        if (p_props.has("min_slm_affinity_floor"))           min_slm_affinity_floor            = static_cast<float>(static_cast<double>(p_props["min_slm_affinity_floor"]));
        if (p_props.has("active_lect_id"))                   active_lect_id                    = static_cast<uint16_t>(static_cast<int64_t>(p_props["active_lect_id"]));
        if (p_props.has("allowed_upos_mask"))                allowed_upos_mask                 = static_cast<uint16_t>(static_cast<int64_t>(p_props["allowed_upos_mask"]));
        if (p_props.has("apply_consequent_state_to_patient"))apply_consequent_state_to_patient = static_cast<bool>(p_props["apply_consequent_state_to_patient"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *inst_part  = p_ctx.get_grant_part(frame_instance_pod_buffer_id);
        const GrantPartPOD *frame_part = p_ctx.get_grant_part(semantic_frame_pod_buffer_id);
        const GrantPartPOD *sem_part   = p_ctx.get_grant_part(sememe_descriptor_pod_buffer_id);
        const GrantPartPOD *lex_part   = p_ctx.get_grant_part(lexeme_pod_buffer_id);
        const GrantPartPOD *aff_part   = p_ctx.get_grant_part(sememe_affinity_score_buffer_id);
        const GrantPartPOD *slm_part   = p_ctx.get_grant_part(slm_vocab_logit_mask_buffer_id);

        auto fold64_to_32 = [](uint64_t w) noexcept -> uint32_t {
            return static_cast<uint32_t>(w & 0xFFFFFFFFULL) | static_cast<uint32_t>(w >> 32u);
        };

        // 1. Evaluate FrameNet Clause Instances
        if (inst_part && inst_part->raw_base_ptr &&
            frame_part && frame_part->raw_base_ptr &&
            sem_part && sem_part->raw_base_ptr) {
            auto *instances        = reinterpret_cast<FrameClauseInstancePOD *>(inst_part->raw_base_ptr);
            const size_t inst_cap  = (inst_part->selection.element_count > 0)
                ? std::min<size_t>(static_cast<size_t>(inst_part->selection.element_count), inst_part->capacity_bytes / sizeof(FrameClauseInstancePOD))
                : (inst_part->capacity_bytes / sizeof(FrameClauseInstancePOD));
            const auto *frames     = reinterpret_cast<const SemanticFramePOD *>(frame_part->raw_base_ptr);
            const size_t frame_cap = frame_part->capacity_bytes / sizeof(SemanticFramePOD);
            auto *sememes          = reinterpret_cast<SememeDescriptorPOD *>(sem_part->raw_base_ptr);
            const size_t sem_cap   = sem_part->capacity_bytes / sizeof(SememeDescriptorPOD);

            for (size_t i = 0; i < inst_cap; ++i) {
                FrameClauseInstancePOD &cl = instances[i];
                if (cl.frame_id >= frame_cap) continue;
                const SemanticFramePOD &fr = frames[cl.frame_id];

                float score = 0.0f;
                int checks  = 0;

                if (fr.required_agent_seme_mask32 != 0u) {
                    ++checks;
                    if (cl.agent_sememe_id < sem_cap) {
                        const uint32_t ag_m = fold64_to_32(sememes[cl.agent_sememe_id].nuclear_seme_words[0]);
                        if ((ag_m & fr.required_agent_seme_mask32) != 0u) score += 1.0f;
                    }
                }
                if (fr.required_patient_seme_mask32 != 0u) {
                    ++checks;
                    if (cl.patient_sememe_id < sem_cap) {
                        const uint32_t pt_m = fold64_to_32(sememes[cl.patient_sememe_id].nuclear_seme_words[0]);
                        if ((pt_m & fr.required_patient_seme_mask32) != 0u) score += 1.0f;
                    }
                }
                if (fr.required_instrument_mask32 != 0u && cl.instrument_sememe_id < sem_cap) {
                    ++checks;
                    const uint32_t in_m = fold64_to_32(sememes[cl.instrument_sememe_id].nuclear_seme_words[0]);
                    if ((in_m & fr.required_instrument_mask32) != 0u) score += 1.0f;
                }

                cl.role_fit_score = (checks > 0) ? (score / static_cast<float>(checks)) : 1.0f;
                if (apply_consequent_state_to_patient &&
                    cl.role_fit_score >= role_fit_threshold &&
                    cl.patient_sememe_id < sem_cap &&
                    fr.consequent_state_seme_mask32 != 0u) {
                    sememes[cl.patient_sememe_id].nuclear_seme_words[0] |=
                        static_cast<uint64_t>(fr.consequent_state_seme_mask32);
                    cl.consequent_applied = 1u;
                }
            }
        }

        // 2. Compile Constrained SLM Vocabulary Mask (`LING_SLM_VOCAB_LOGIT_MASK`)
        if (slm_part && slm_part->raw_base_ptr && lex_part && lex_part->raw_base_ptr) {
            auto *slm_mask        = reinterpret_cast<uint32_t *>(slm_part->raw_base_ptr);
            const size_t slm_cap  = slm_part->capacity_bytes / sizeof(uint32_t);
            const auto *lexemes   = reinterpret_cast<const LexemePOD *>(lex_part->raw_base_ptr);
            const size_t lex_cap  = (lex_part->selection.element_count > 0)
                ? std::min<size_t>(static_cast<size_t>(lex_part->selection.element_count), lex_part->capacity_bytes / sizeof(LexemePOD))
                : (lex_part->capacity_bytes / sizeof(LexemePOD));
            const auto *aff_col   = (aff_part && aff_part->raw_base_ptr)
                ? reinterpret_cast<const float *>(aff_part->raw_base_ptr)
                : nullptr;
            const size_t aff_cap  = aff_col ? (aff_part->capacity_bytes / sizeof(float)) : 0u;

            size_t write_idx = 0u;
            for (size_t l = 0; l < lex_cap && write_idx < slm_cap; ++l) {
                const LexemePOD &lx = lexemes[l];
                if (lx.slm_bpe_token_id == 0xFFFFFFFFu) continue;
                if (active_lect_id != 0u && lx.lect_id != active_lect_id && lx.lect_id != 0u) continue;

                const uint16_t cat_bit = (static_cast<uint8_t>(lx.category) < 16u)
                    ? static_cast<uint16_t>(1u << static_cast<uint8_t>(lx.category))
                    : 1u;
                if ((allowed_upos_mask & cat_bit) == 0u) continue;

                if (aff_col && lx.sememe_id < aff_cap) {
                    if (aff_col[lx.sememe_id] < min_slm_affinity_floor) continue;
                }
                slm_mask[write_idx++] = lx.slm_bpe_token_id;
            }
            for (size_t k = write_idx; k < slm_cap; ++k) {
                slm_mask[k] = 0xFFFFFFFFu;
            }
            const_cast<GrantPartPOD *>(slm_part)->selection.element_count = static_cast<int64_t>(write_idx);
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(SemanticFrameAndSLMMaskTransformLogic<>) == 64, "SemanticFrameAndSLMMaskTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<SemanticFrameAndSLMMaskTransformLogic<>>, "SemanticFrameAndSLMMaskTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
