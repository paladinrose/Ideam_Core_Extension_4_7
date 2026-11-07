#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/aosoa_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_IDENTITY_AND_FIT_ENUMS_DEFINED
#define IDEAM_NARRATIVES_IDENTITY_AND_FIT_ENUMS_DEFINED

/**
 * Concrete Narreme archetype identifiers stored in NarremeIdentityAndTraitBuffer::narreme_type (uint8_t).
 * Pairs literal story nouns/verbs with their symbolic counterparts in Narrative:
 *   - Character (Literal) <-> Role   (Symbolic)
 *   - Prop      (Literal) <-> Symbol (Symbolic)
 *   - Location  (Literal) <-> Place  (Symbolic)
 *   - Incident  (Literal) <-> Moment (Symbolic)
 *   - Plot      (Literal) <-> Story  (Symbolic)
 */
enum NarremeType : uint8_t {
    NARREME_TYPE_CHARACTER = 0,
    NARREME_TYPE_PROP      = 1,
    NARREME_TYPE_LOCATION  = 2,
    NARREME_TYPE_INCIDENT  = 3,
    NARREME_TYPE_PLOT      = 4,
    NARREME_TYPE_NARRATIVE = 5,
    NARREME_TYPE_ANY       = 0xFFu
};

/**
 * DOD BestNarrativeFit modes (mirrors ideam::godot_ext::BestNarrativeFit in narrative.h).
 */
enum NarrativeBestFitMode : uint8_t {
    NARRATIVE_FIT_BASIC    = 0, // Base similarity only (type + 128-bit trait Hamming distance + Vector4 affinity)
    NARRATIVE_FIT_SYMBOLIC = 1, // Resolves symbolic Places/Roles/Symbols/Stories before comparing structural links
    NARRATIVE_FIT_LITERAL  = 2, // Compares direct literal Location, Possessor, Relationship, and Goal IDs
    NARRATIVE_FIT_THOROUGH = 3  // Accepts either direct literal matches OR resolved symbolic assignment matches
};

/**
 * DOD NarrativeReplacement policies (mirrors ideam::godot_ext::NarrativeReplacement in narrative.h).
 */
enum NarrativeReplacementPolicy : uint8_t {
    NARRATIVE_REPLACE_SYMBOLIC = 0, // Replaces references to the Symbolic Narreme (Role, Symbol, Place, Moment, Story)
    NARRATIVE_REPLACE_LITERAL  = 1, // Replaces references to the previously assigned Literal Narreme
    NARRATIVE_REPLACE_THOROUGH = 2  // Replaces references to both the outgoing Literal and the Symbolic Narreme
};

/**
 * Sentinel maximum discrepancy score matching Narrative::get_best_fit_* (100000) in narrative.cpp.
 * Lower discrepancy scores represent closer matches (0 = exact match).
 */
static constexpr int32_t NARRATIVE_MAX_DISCREPANCY_SCORE = 100000;

#endif // IDEAM_NARRATIVES_IDENTITY_AND_FIT_ENUMS_DEFINED

/**
 * NarremeSymbolicSimilarityTransformLogic<T> (N1)
 * Computes similarity / discrepancy scores (FIT_BASIC, FIT_SYMBOLIC, FIT_LITERAL, FIT_THOROUGH)
 * between an uncast Symbolic Narreme (Role, Symbol, Place, Moment, Story) and candidate Literal
 * Narremes across NarremeIdentityAndTraitBuffer in SIMD (AOSOAView / TiledSoAStrategy).
 *
 * Replaces virtual Narreme::score_similarity() and the per-candidate scoring loops inside:
 *   - Narrative::get_best_fit_character_for_role()
 *   - Narrative::get_best_fit_prop_for_symbol()
 *   - Narrative::get_best_fit_location_for_place()
 *   - Narrative::get_best_fit_incident_for_moment()
 *   - Narrative::get_best_fit_plot_for_story()
 */
template <typename T = int32_t>
struct alignas(64) NarremeSymbolicSimilarityTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = TiledSoAStrategy;
    using DefaultView     = AOSOAView<T, 8, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::INT32 | DataType::INT64 | DataType::VECTOR4;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Narreme Symbolic Similarity";
    static constexpr std::string_view ui_category  = "Casting";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t output_score_buffer_id         = INVALID_ID;            // 4B: Output int32_t candidate similarity/discrepancy score column
    uint32_t trait_bitmask_lo_buffer_id     = INVALID_ID;            // 4B: NarremeIdentityAndTraitBuffer::trait_bitmask_lo (uint64_t)
    uint32_t trait_bitmask_hi_buffer_id     = INVALID_ID;            // 4B: NarremeIdentityAndTraitBuffer::trait_bitmask_hi (uint64_t)
    uint32_t affinity_vector_buffer_id      = INVALID_ID;            // 4B: NarremeIdentityAndTraitBuffer::affinity_vector (godot::Vector4)
    uint32_t narreme_type_buffer_id         = INVALID_ID;            // 4B: NarremeIdentityAndTraitBuffer::narreme_type (uint8_t)
    uint32_t is_symbolic_buffer_id          = INVALID_ID;            // 4B: NarremeIdentityAndTraitBuffer::is_symbolic (uint8_t/bool)
    uint32_t location_id_buffer_id          = INVALID_ID;            // 4B: NarremeIdentityAndTraitBuffer::location_id (uint32_t)
    uint32_t possessor_id_buffer_id         = INVALID_ID;            // 4B: NarremeIdentityAndTraitBuffer::possessor_id (uint32_t)
    uint32_t symbolic_to_literal_buffer_id  = INVALID_ID;            // 4B: SymbolicAssignmentMapBuffer::symbolic_to_literal (int32_t/uint32_t)
    uint32_t structural_sig_buffer_id       = INVALID_ID;            // 4B: Optional uint64_t packed relationship/goal/possession bitmask per Narreme
    uint32_t candidate_mask_buffer_id       = INVALID_ID;            // 4B: Optional uint8_t/uint32_t exclusion mask (non-zero = masked out)
    uint32_t symbolic_narreme_idx           = INVALID_ID;            // 4B: Row index of the Symbolic Narreme in NarremeIdentityAndTraitBuffer
    uint32_t target_trait_mask_lo           = 0u;                    // 4B: Fallback low 32-bit symbolic trait mask when symbolic_narreme_idx is unbound
    float affinity_penalty_scale            = 10.0f;                 // 4B: Multiplier converting Vector4 affinity distance into int32_t penalty

    uint8_t fit_mode                        = NARRATIVE_FIT_THOROUGH;// 1B: NarrativeBestFitMode (Basic, Symbolic, Literal, Thorough)
    uint8_t target_narreme_type             = NARREME_TYPE_CHARACTER;// 1B: Expected NarremeType when symbolic_narreme_idx is unbound
    uint8_t location_mismatch_penalty       = 1u;                    // 1B: Discrepancy penalty when location does not match (+1 in Narrative)
    uint8_t possessor_mismatch_penalty      = 1u;                    // 1B: Discrepancy penalty when possessor does not match (+1 in Narrative)
    uint8_t structural_bit_penalty          = 1u;                    // 1B: Penalty per missing structural/relationship/goal bit
    bool filter_by_narreme_type             = true;                  // 1B: Assign NARRATIVE_MAX_DISCREPANCY_SCORE on NarremeType mismatch
    bool exclude_symbolic_candidates        = true;                  // 1B: Mask out rows where is_symbolic != 0 (only cast Literal Narremes)
    bool invert_score_to_similarity         = false;                 // 1B: False = discrepancy (0 is best); True = similarity (MAX - discrepancy)

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary out_prop;
        out_prop["name"] = "output_score_buffer_id";
        out_prop["type"] = godot::Variant::INT;
        out_prop["hint_string"] = "buffer_option";
        props.push_back(out_prop);

        godot::Dictionary tlo_prop;
        tlo_prop["name"] = "trait_bitmask_lo_buffer_id";
        tlo_prop["type"] = godot::Variant::INT;
        tlo_prop["hint_string"] = "buffer_option";
        props.push_back(tlo_prop);

        godot::Dictionary thi_prop;
        thi_prop["name"] = "trait_bitmask_hi_buffer_id";
        thi_prop["type"] = godot::Variant::INT;
        thi_prop["hint_string"] = "buffer_option";
        props.push_back(thi_prop);

        godot::Dictionary aff_prop;
        aff_prop["name"] = "affinity_vector_buffer_id";
        aff_prop["type"] = godot::Variant::INT;
        aff_prop["hint_string"] = "buffer_option";
        props.push_back(aff_prop);

        godot::Dictionary ntyp_prop;
        ntyp_prop["name"] = "narreme_type_buffer_id";
        ntyp_prop["type"] = godot::Variant::INT;
        ntyp_prop["hint_string"] = "buffer_option";
        props.push_back(ntyp_prop);

        godot::Dictionary sym_prop;
        sym_prop["name"] = "is_symbolic_buffer_id";
        sym_prop["type"] = godot::Variant::INT;
        sym_prop["hint_string"] = "buffer_option";
        props.push_back(sym_prop);

        godot::Dictionary loc_prop;
        loc_prop["name"] = "location_id_buffer_id";
        loc_prop["type"] = godot::Variant::INT;
        loc_prop["hint_string"] = "buffer_option";
        props.push_back(loc_prop);

        godot::Dictionary pos_prop;
        pos_prop["name"] = "possessor_id_buffer_id";
        pos_prop["type"] = godot::Variant::INT;
        pos_prop["hint_string"] = "buffer_option";
        props.push_back(pos_prop);

        godot::Dictionary map_prop;
        map_prop["name"] = "symbolic_to_literal_buffer_id";
        map_prop["type"] = godot::Variant::INT;
        map_prop["hint_string"] = "buffer_option";
        props.push_back(map_prop);

        godot::Dictionary ssig_prop;
        ssig_prop["name"] = "structural_sig_buffer_id";
        ssig_prop["type"] = godot::Variant::INT;
        ssig_prop["hint_string"] = "buffer_option";
        props.push_back(ssig_prop);

        godot::Dictionary msk_prop;
        msk_prop["name"] = "candidate_mask_buffer_id";
        msk_prop["type"] = godot::Variant::INT;
        msk_prop["hint_string"] = "buffer_option";
        props.push_back(msk_prop);

        godot::Dictionary sidx_prop;
        sidx_prop["name"] = "symbolic_narreme_idx";
        sidx_prop["type"] = godot::Variant::INT;
        sidx_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(sidx_prop);

        godot::Dictionary fit_prop;
        fit_prop["name"] = "fit_mode";
        fit_prop["type"] = godot::Variant::INT;
        fit_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        fit_prop["hint_string"] = "Fit Basic:0,Fit Symbolic:1,Fit Literal:2,Fit Thorough:3";
        props.push_back(fit_prop);

        godot::Dictionary ttype_prop;
        ttype_prop["name"] = "target_narreme_type";
        ttype_prop["type"] = godot::Variant::INT;
        ttype_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        ttype_prop["hint_string"] = "Character (Role):0,Prop (Symbol):1,Location (Place):2,Incident (Moment):3,Plot (Story):4,Narrative:5,Any:255";
        props.push_back(ttype_prop);

        godot::Dictionary ttlo_prop;
        ttlo_prop["name"] = "target_trait_mask_lo";
        ttlo_prop["type"] = godot::Variant::INT;
        ttlo_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ttlo_prop);

        godot::Dictionary asc_prop;
        asc_prop["name"] = "affinity_penalty_scale";
        asc_prop["type"] = godot::Variant::FLOAT;
        asc_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        asc_prop["hint_string"] = "0.0,1000.0,0.5";
        props.push_back(asc_prop);

        godot::Dictionary lpen_prop;
        lpen_prop["name"] = "location_mismatch_penalty";
        lpen_prop["type"] = godot::Variant::INT;
        lpen_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        lpen_prop["hint_string"] = "0,64,1";
        props.push_back(lpen_prop);

        godot::Dictionary ppen_prop;
        ppen_prop["name"] = "possessor_mismatch_penalty";
        ppen_prop["type"] = godot::Variant::INT;
        ppen_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        ppen_prop["hint_string"] = "0,64,1";
        props.push_back(ppen_prop);

        godot::Dictionary spen_prop;
        spen_prop["name"] = "structural_bit_penalty";
        spen_prop["type"] = godot::Variant::INT;
        spen_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        spen_prop["hint_string"] = "0,64,1";
        props.push_back(spen_prop);

        godot::Dictionary ftyp_prop;
        ftyp_prop["name"] = "filter_by_narreme_type";
        ftyp_prop["type"] = godot::Variant::BOOL;
        ftyp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ftyp_prop);

        godot::Dictionary exs_prop;
        exs_prop["name"] = "exclude_symbolic_candidates";
        exs_prop["type"] = godot::Variant::BOOL;
        exs_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(exs_prop);

        godot::Dictionary inv_prop;
        inv_prop["name"] = "invert_score_to_similarity";
        inv_prop["type"] = godot::Variant::BOOL;
        inv_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(inv_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (output_score_buffer_id != INVALID_ID)     return output_score_buffer_id;
        if (trait_bitmask_lo_buffer_id != INVALID_ID) return trait_bitmask_lo_buffer_id;
        return affinity_vector_buffer_id;
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        (void)p_context;
        (void)p_part;
        if constexpr (requires { p_view.strategy.elements_per_tile; p_view.strategy.tile_stride_bytes; }) {
            if (p_view.strategy.elements_per_tile == 0u) {
                p_view.strategy.elements_per_tile = 8u;
                p_view.strategy.tile_stride_bytes = static_cast<uint32_t>(8u * sizeof(T));
            }
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("output_score_buffer_id")) {
            output_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_score_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && output_score_buffer_id == INVALID_ID) {
            output_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("trait_bitmask_lo_buffer_id")) {
            trait_bitmask_lo_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["trait_bitmask_lo_buffer_id"]));
        }
        if (p_props.has("trait_bitmask_hi_buffer_id")) {
            trait_bitmask_hi_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["trait_bitmask_hi_buffer_id"]));
        }
        if (p_props.has("affinity_vector_buffer_id")) {
            affinity_vector_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["affinity_vector_buffer_id"]));
        }
        if (p_props.has("narreme_type_buffer_id")) {
            narreme_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["narreme_type_buffer_id"]));
        }
        if (p_props.has("is_symbolic_buffer_id")) {
            is_symbolic_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["is_symbolic_buffer_id"]));
        }
        if (p_props.has("location_id_buffer_id")) {
            location_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["location_id_buffer_id"]));
        }
        if (p_props.has("possessor_id_buffer_id")) {
            possessor_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["possessor_id_buffer_id"]));
        }
        if (p_props.has("symbolic_to_literal_buffer_id")) {
            symbolic_to_literal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["symbolic_to_literal_buffer_id"]));
        }
        if (p_props.has("structural_sig_buffer_id")) {
            structural_sig_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["structural_sig_buffer_id"]));
        }
        if (p_props.has("candidate_mask_buffer_id")) {
            candidate_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["candidate_mask_buffer_id"]));
        }
        if (p_props.has("symbolic_narreme_idx")) {
            symbolic_narreme_idx = static_cast<uint32_t>(static_cast<int64_t>(p_props["symbolic_narreme_idx"]));
        }
        if (p_props.has("target_trait_mask_lo")) {
            target_trait_mask_lo = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_trait_mask_lo"]));
        }
        if (p_props.has("affinity_penalty_scale")) {
            affinity_penalty_scale = static_cast<float>(p_props["affinity_penalty_scale"]);
        }
        if (p_props.has("fit_mode")) {
            fit_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["fit_mode"]), 0, 3));
        }
        if (p_props.has("target_narreme_type")) {
            target_narreme_type = static_cast<uint8_t>(static_cast<int64_t>(p_props["target_narreme_type"]));
        }
        if (p_props.has("location_mismatch_penalty")) {
            location_mismatch_penalty = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["location_mismatch_penalty"]), 0, 255));
        }
        if (p_props.has("possessor_mismatch_penalty")) {
            possessor_mismatch_penalty = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["possessor_mismatch_penalty"]), 0, 255));
        }
        if (p_props.has("structural_bit_penalty")) {
            structural_bit_penalty = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["structural_bit_penalty"]), 0, 255));
        }
        if (p_props.has("filter_by_narreme_type")) {
            filter_by_narreme_type = static_cast<bool>(p_props["filter_by_narreme_type"]);
        }
        if (p_props.has("exclude_symbolic_candidates")) {
            exclude_symbolic_candidates = static_cast<bool>(p_props["exclude_symbolic_candidates"]);
        }
        if (p_props.has("invert_score_to_similarity")) {
            invert_score_to_similarity = static_cast<bool>(p_props["invert_score_to_similarity"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t> || std::is_same_v<T, godot::Vector4>) {
            const GrantPartPOD* out_part = (output_score_buffer_id != INVALID_ID)         ? p_context.get_grant_part(output_score_buffer_id)         : nullptr;
            const GrantPartPOD* tlo_part = (trait_bitmask_lo_buffer_id != INVALID_ID)     ? p_context.get_grant_part(trait_bitmask_lo_buffer_id)     : nullptr;
            const GrantPartPOD* thi_part = (trait_bitmask_hi_buffer_id != INVALID_ID)     ? p_context.get_grant_part(trait_bitmask_hi_buffer_id)     : nullptr;
            const GrantPartPOD* aff_part = (affinity_vector_buffer_id != INVALID_ID)      ? p_context.get_grant_part(affinity_vector_buffer_id)      : nullptr;
            const GrantPartPOD* ntp_part = (narreme_type_buffer_id != INVALID_ID)         ? p_context.get_grant_part(narreme_type_buffer_id)         : nullptr;
            const GrantPartPOD* sym_part = (is_symbolic_buffer_id != INVALID_ID)          ? p_context.get_grant_part(is_symbolic_buffer_id)          : nullptr;
            const GrantPartPOD* loc_part = (location_id_buffer_id != INVALID_ID)          ? p_context.get_grant_part(location_id_buffer_id)          : nullptr;
            const GrantPartPOD* pos_part = (possessor_id_buffer_id != INVALID_ID)         ? p_context.get_grant_part(possessor_id_buffer_id)         : nullptr;
            const GrantPartPOD* map_part = (symbolic_to_literal_buffer_id != INVALID_ID)  ? p_context.get_grant_part(symbolic_to_literal_buffer_id)  : nullptr;
            const GrantPartPOD* sig_part = (structural_sig_buffer_id != INVALID_ID)       ? p_context.get_grant_part(structural_sig_buffer_id)       : nullptr;
            const GrantPartPOD* msk_part = (candidate_mask_buffer_id != INVALID_ID)       ? p_context.get_grant_part(candidate_mask_buffer_id)       : nullptr;

            const GrantPartPOD* primary_part = out_part ? out_part : (tlo_part ? tlo_part : aff_part);
            if (!primary_part || !primary_part->raw_base_ptr) return;

            int32_t* __restrict out_scores = (out_part && out_part->raw_base_ptr)
                ? reinterpret_cast<int32_t*>(out_part->raw_base_ptr)
                : nullptr;
            const uint64_t* __restrict trait_lo = (tlo_part && tlo_part->raw_base_ptr)
                ? reinterpret_cast<const uint64_t*>(tlo_part->raw_base_ptr)
                : nullptr;
            const uint64_t* __restrict trait_hi = (thi_part && thi_part->raw_base_ptr)
                ? reinterpret_cast<const uint64_t*>(thi_part->raw_base_ptr)
                : nullptr;
            const godot::Vector4* __restrict affinities = (aff_part && aff_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector4*>(aff_part->raw_base_ptr)
                : nullptr;
            const uint8_t* __restrict ntype_raw = (ntp_part && ntp_part->raw_base_ptr) ? ntp_part->raw_base_ptr : nullptr;
            const uint32_t ntype_stride         = ntp_part ? ntp_part->element_stride : 0;
            const uint8_t* __restrict sym_raw   = (sym_part && sym_part->raw_base_ptr) ? sym_part->raw_base_ptr : nullptr;
            const uint32_t sym_stride           = sym_part ? sym_part->element_stride : 0;
            const uint32_t* __restrict loc_ids  = (loc_part && loc_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(loc_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict pos_ids  = (pos_part && pos_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(pos_part->raw_base_ptr)
                : nullptr;
            const int32_t* __restrict sym_to_lit = (map_part && map_part->raw_base_ptr)
                ? reinterpret_cast<const int32_t*>(map_part->raw_base_ptr)
                : nullptr;
            const uint64_t* __restrict struct_sigs = (sig_part && sig_part->raw_base_ptr)
                ? reinterpret_cast<const uint64_t*>(sig_part->raw_base_ptr)
                : nullptr;
            const uint8_t* __restrict mask_raw  = (msk_part && msk_part->raw_base_ptr) ? msk_part->raw_base_ptr : nullptr;
            const uint32_t mask_stride          = msk_part ? msk_part->element_stride : 0;

            const int64_t out_cap = out_scores  ? static_cast<int64_t>(out_part->capacity_bytes / sizeof(int32_t))        : 0;
            const int64_t tlo_cap = trait_lo    ? static_cast<int64_t>(tlo_part->capacity_bytes / sizeof(uint64_t))       : 0;
            const int64_t thi_cap = trait_hi    ? static_cast<int64_t>(thi_part->capacity_bytes / sizeof(uint64_t))       : 0;
            const int64_t aff_cap = affinities  ? static_cast<int64_t>(aff_part->capacity_bytes / sizeof(godot::Vector4)) : 0;
            const int64_t ntp_cap = (ntype_raw && ntype_stride > 0) ? static_cast<int64_t>(ntp_part->capacity_bytes / ntype_stride) : 0;
            const int64_t sym_cap = (sym_raw && sym_stride > 0)     ? static_cast<int64_t>(sym_part->capacity_bytes / sym_stride)   : 0;
            const int64_t loc_cap = loc_ids     ? static_cast<int64_t>(loc_part->capacity_bytes / sizeof(uint32_t))       : 0;
            const int64_t pos_cap = pos_ids     ? static_cast<int64_t>(pos_part->capacity_bytes / sizeof(uint32_t))       : 0;
            const int64_t map_cap = sym_to_lit  ? static_cast<int64_t>(map_part->capacity_bytes / sizeof(int32_t))        : 0;
            const int64_t sig_cap = struct_sigs ? static_cast<int64_t>(sig_part->capacity_bytes / sizeof(uint64_t))       : 0;
            const int64_t msk_cap = (mask_raw && mask_stride > 0)   ? static_cast<int64_t>(msk_part->capacity_bytes / mask_stride)  : 0;

            int64_t elem_count = static_cast<int64_t>(
                primary_part->capacity_bytes / (primary_part->element_stride > 0 ? primary_part->element_stride : sizeof(T))
            );
            if (out_part) elem_count = std::min<int64_t>(elem_count, out_cap);
            if (elem_count <= 0) return;

            auto read_narreme_type = [&](int64_t idx) noexcept -> uint8_t {
                if (!ntype_raw || idx < 0 || idx >= ntp_cap) return target_narreme_type;
                if (ntype_stride == sizeof(int32_t)) {
                    return static_cast<uint8_t>(reinterpret_cast<const int32_t*>(ntype_raw)[idx]);
                }
                return ntype_raw[idx];
            };

            auto read_is_symbolic = [&](int64_t idx) noexcept -> bool {
                if (!sym_raw || idx < 0 || idx >= sym_cap) return false;
                if (sym_stride == sizeof(int32_t)) {
                    return reinterpret_cast<const int32_t*>(sym_raw)[idx] != 0;
                }
                return sym_raw[idx] != 0u;
            };

            auto is_candidate_masked = [&](int64_t idx) noexcept -> bool {
                if (!mask_raw || idx < 0 || idx >= msk_cap) return false;
                if (mask_stride == sizeof(uint32_t)) {
                    return reinterpret_cast<const uint32_t*>(mask_raw)[idx] != 0u;
                }
                if (mask_stride == sizeof(uint64_t)) {
                    return reinterpret_cast<const uint64_t*>(mask_raw)[idx] != 0ULL;
                }
                return mask_raw[idx] != 0u;
            };

            auto resolve_symbolic_id = [&](uint32_t sym_id) noexcept -> uint32_t {
                if (!sym_to_lit || sym_id == INVALID_ID) return INVALID_ID;
                const int64_t sidx = static_cast<int64_t>(sym_id);
                if (sidx < 0 || sidx >= map_cap) return INVALID_ID;
                const int32_t mapped = sym_to_lit[sidx];
                return (mapped >= 0) ? static_cast<uint32_t>(mapped) : INVALID_ID;
            };

            // Extract target Symbolic Narreme attributes (from symbolic_narreme_idx row or inline defaults)
            const int64_t s_idx      = (symbolic_narreme_idx != INVALID_ID) ? static_cast<int64_t>(symbolic_narreme_idx) : -1;
            const uint8_t sym_type   = (s_idx >= 0 && s_idx < ntp_cap) ? read_narreme_type(s_idx) : target_narreme_type;
            const uint64_t sym_tlo   = (s_idx >= 0 && s_idx < tlo_cap) ? trait_lo[s_idx]          : static_cast<uint64_t>(target_trait_mask_lo);
            const uint64_t sym_thi   = (s_idx >= 0 && s_idx < thi_cap) ? trait_hi[s_idx]          : 0ULL;
            const godot::Vector4 sym_aff = (s_idx >= 0 && s_idx < aff_cap)
                ? affinities[s_idx]
                : godot::Vector4{0.0f, 0.0f, 0.0f, 0.0f};
            const bool has_sym_aff   = (affinities != nullptr) && (s_idx >= 0 && s_idx < aff_cap);
            const uint32_t sym_loc   = (s_idx >= 0 && s_idx < loc_cap) ? loc_ids[s_idx]           : INVALID_ID;
            const uint32_t sym_pos   = (s_idx >= 0 && s_idx < pos_cap) ? pos_ids[s_idx]           : INVALID_ID;
            const uint64_t sym_sig   = (s_idx >= 0 && s_idx < sig_cap) ? struct_sigs[s_idx]       : 0ULL;

            // Pre-resolve symbolic Place (for sym_loc) and symbolic Role (for sym_pos) once outside the hot loop
            const uint32_t resolved_sym_loc = resolve_symbolic_id(sym_loc);
            const uint32_t resolved_sym_pos = resolve_symbolic_id(sym_pos);
            const float aff_scale           = std::max(0.0f, affinity_penalty_scale);

            auto finalize_score = [&](int32_t discrepancy) noexcept -> int32_t {
                const int32_t clamped = std::clamp<int32_t>(discrepancy, 0, NARRATIVE_MAX_DISCREPANCY_SCORE);
                return invert_score_to_similarity ? (NARRATIVE_MAX_DISCREPANCY_SCORE - clamped) : clamped;
            };

            auto compute_candidate_score = [&](int64_t idx) noexcept -> int32_t {
                if (idx == s_idx || is_candidate_masked(idx)) {
                    return finalize_score(NARRATIVE_MAX_DISCREPANCY_SCORE);
                }
                if (exclude_symbolic_candidates && sym_raw && read_is_symbolic(idx)) {
                    return finalize_score(NARRATIVE_MAX_DISCREPANCY_SCORE);
                }
                if (filter_by_narreme_type && ntype_raw && sym_type != NARREME_TYPE_ANY) {
                    if (read_narreme_type(idx) != sym_type) {
                        return finalize_score(NARRATIVE_MAX_DISCREPANCY_SCORE);
                    }
                }

                int32_t discrepancy = 0;

                // 1. 128-bit Trait Bitmask SIMD Hamming Distance: popcount(cand_lo ^ sym_lo) + popcount(cand_hi ^ sym_hi)
                if (trait_lo && idx < tlo_cap) {
                    discrepancy += std::popcount(trait_lo[idx] ^ sym_tlo);
                }
                if (trait_hi && idx < thi_cap) {
                    discrepancy += std::popcount(trait_hi[idx] ^ sym_thi);
                }

                // 2. Vector4 Affinity Discrepancy: Euclidean distance scaled by affinity_penalty_scale
                if (has_sym_aff && idx < aff_cap && aff_scale > 0.0f) {
                    const godot::Vector4 c_aff = affinities[idx];
                    const float dx = c_aff.x - sym_aff.x;
                    const float dy = c_aff.y - sym_aff.y;
                    const float dz = c_aff.z - sym_aff.z;
                    const float dw = c_aff.w - sym_aff.w;
                    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz + dw * dw);
                    discrepancy += static_cast<int32_t>(std::lround(dist * aff_scale));
                }

                // 3. Structural Fit Evaluation (FIT_SYMBOLIC, FIT_LITERAL, FIT_THOROUGH)
                // Mirrors Narrative::get_best_fit_character_for_role / get_best_fit_prop_for_symbol in narrative.cpp
                if (fit_mode != NARRATIVE_FIT_BASIC) {
                    // 3a. Location / Place co-presence check
                    if (loc_ids && idx < loc_cap && location_mismatch_penalty > 0u) {
                        const uint32_t cand_loc = loc_ids[idx];
                        if (fit_mode == NARRATIVE_FIT_LITERAL) {
                            if (sym_loc != cand_loc) {
                                discrepancy += static_cast<int32_t>(location_mismatch_penalty);
                            }
                        } else if (fit_mode == NARRATIVE_FIT_SYMBOLIC) {
                            if (sym_loc != INVALID_ID) {
                                if (resolved_sym_loc != INVALID_ID && resolved_sym_loc != cand_loc) {
                                    discrepancy += static_cast<int32_t>(location_mismatch_penalty);
                                }
                            } else if (cand_loc != INVALID_ID) {
                                discrepancy += static_cast<int32_t>(location_mismatch_penalty);
                            }
                        } else if (fit_mode == NARRATIVE_FIT_THOROUGH) {
                            if (sym_loc != cand_loc) {
                                if (resolved_sym_loc != INVALID_ID && resolved_sym_loc != cand_loc) {
                                    discrepancy += static_cast<int32_t>(location_mismatch_penalty);
                                }
                            }
                        }
                    }

                    // 3b. Possessor / Role ownership check (Props & Symbols)
                    if (pos_ids && idx < pos_cap && possessor_mismatch_penalty > 0u) {
                        const uint32_t cand_pos = pos_ids[idx];
                        if (fit_mode == NARRATIVE_FIT_LITERAL) {
                            if (sym_pos != cand_pos) {
                                discrepancy += static_cast<int32_t>(possessor_mismatch_penalty);
                            }
                        } else if (fit_mode == NARRATIVE_FIT_SYMBOLIC) {
                            if (sym_pos != INVALID_ID && resolved_sym_pos != INVALID_ID && resolved_sym_pos != cand_pos) {
                                discrepancy += static_cast<int32_t>(possessor_mismatch_penalty);
                            }
                        } else if (fit_mode == NARRATIVE_FIT_THOROUGH) {
                            if (sym_pos != cand_pos && resolved_sym_pos != INVALID_ID && resolved_sym_pos != cand_pos) {
                                discrepancy += static_cast<int32_t>(possessor_mismatch_penalty);
                            }
                        }
                    }

                    // 3c. Missing Relationship / Possession / Goal / Event structural signature bits
                    if (struct_sigs && idx < sig_cap && sym_sig != 0ULL && structural_bit_penalty > 0u) {
                        const uint64_t missing_bits = sym_sig & ~struct_sigs[idx];
                        discrepancy += std::popcount(missing_bits) * static_cast<int32_t>(structural_bit_penalty);
                    }
                }

                return finalize_score(discrepancy);
            };

            const MemoryBufferSelectionPOD& sel = primary_part->selection;

            // Fast-path SIMD lane execution when T == int32_t and bound to AOSOAView on a contiguous RANGE selection
            if constexpr (std::is_same_v<T, int32_t> && requires { p_view.lane_count(); p_view.get_lane(0); T_View::lane_width; }) {
                if (out_part == primary_part && sel.mode == SelectionMode::RANGE && sel.start_index == 0) {
                    constexpr size_t L_WIDTH = T_View::lane_width;
                    const size_t lanes       = p_view.lane_count();
                    const int64_t active_cnt = (sel.element_count > 0) ? std::min<int64_t>(sel.element_count, elem_count) : elem_count;
                    const size_t full_lanes  = std::min<size_t>(lanes, static_cast<size_t>(active_cnt) / L_WIDTH);

                    for (size_t l = 0; l < full_lanes; ++l) {
                        int32_t* __restrict lane_ptr = p_view.get_lane(l);
                        const int64_t base_idx = static_cast<int64_t>(l * L_WIDTH);
                        for (size_t k = 0; k < L_WIDTH; ++k) {
                            lane_ptr[k] = compute_candidate_score(base_idx + static_cast<int64_t>(k));
                        }
                    }
                    const int64_t tail_start = static_cast<int64_t>(full_lanes * L_WIDTH);
                    for (int64_t i = tail_start; i < active_cnt; ++i) {
                        out_scores[i] = compute_candidate_score(i);
                    }
                    return;
                }
            }

            // Selection-aware execution across DENSE, SPARSE, or RANGE selections
            auto eval_at = [&](int64_t idx) noexcept {
                const int32_t score = compute_candidate_score(idx);
                if (out_scores && idx < out_cap) {
                    out_scores[idx] = score;
                } else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t>) {
                    reinterpret_cast<T*>(primary_part->raw_base_ptr)[idx] = static_cast<T>(score);
                }
            };
            _iterate_selection(sel, elem_count, eval_at);
        }
    }

private:
    template <typename Fn>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _iterate_selection(const MemoryBufferSelectionPOD& p_sel, int64_t p_cap, Fn&& p_fn) noexcept {
        if (p_sel.mode == SelectionMode::DENSE && p_sel.data.bitset) {
            const uint64_t* bitset = p_sel.data.bitset;
            const int64_t cap = std::min<int64_t>(p_sel.capacity, p_cap);
            for (int64_t i = 0; i < cap; ++i) {
                if (bitset[i >> 6] & (1ULL << (i & 63))) {
                    p_fn(i);
                }
            }
        } else if (p_sel.mode == SelectionMode::SPARSE && p_sel.data.indices) {
            for (int64_t i = 0; i < p_sel.element_count; ++i) {
                const int64_t idx = p_sel.data.indices[i];
                if (idx >= 0 && idx < p_cap) {
                    p_fn(idx);
                }
            }
        } else {
            const int64_t start = std::max<int64_t>(0, p_sel.start_index);
            const int64_t count = (p_sel.element_count > 0) ? p_sel.element_count : p_cap;
            const int64_t end   = std::min<int64_t>(start + count, p_cap);
            for (int64_t i = start; i < end; ++i) {
                p_fn(i);
            }
        }
    }
};

static_assert(sizeof(NarremeSymbolicSimilarityTransformLogic<>) == 64, "NarremeSymbolicSimilarityTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core