#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../transform_logic/symbolic_reference_resolve_transform_logic.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_RELATIONSHIP_GRAPH_QUERY_ENUMS_DEFINED
#define IDEAM_NARRATIVES_RELATIONSHIP_GRAPH_QUERY_ENUMS_DEFINED

/**
 * Graph traversal query modes for RelationshipGraphTraversalQueryLogic (N14),
 * mirroring Narreme::has_relationship_with(), get_relationships_of_type(),
 * and get_related_narremes() in src/godot/narratives/narreme.cpp.
 */
enum RelationshipTraversalQueryMode : uint8_t {
    REL_QUERY_OUTGOING_TO_TARGET       = 0, // Candidate row i has an outgoing CSR edge to target_narreme_id matching token & affinity
    REL_QUERY_INCOMING_FROM_SOURCE     = 1, // source_narreme_id has an outgoing CSR edge to candidate row i (or within max_hops)
    REL_QUERY_CONNECTED_TO_SELECTION   = 2, // Candidate row i has an edge to/from any narreme selected in source_selection_buffer_id
    REL_QUERY_HAS_ANY_MATCHING_EDGE    = 3, // Candidate row i has >= min_matching_degree outgoing edges matching token & affinity
    REL_QUERY_MUTUAL_RECIPROCAL_EDGE   = 4  // Candidate row i and source_narreme_id have reciprocal edges to each other
};

#endif // IDEAM_NARRATIVES_RELATIONSHIP_GRAPH_QUERY_ENUMS_DEFINED

/**
 * RelationshipGraphTraversalQueryLogic<T> (N14)
 * Traverses NarrativeRelationshipCSRBuffer (row_offsets + RelationshipEdgePOD) to select or
 * cull Characters/Narremes connected by a specific interned relationship_title_token,
 * affinity weight range, and edge flag filter to a source narreme or active selection.
 */
template <typename T = int32_t>
struct alignas(64) RelationshipGraphTraversalQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = BridgeView<T, T, 1, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Relationship Graph Traversal";
    static constexpr std::string_view ui_category  = "Relationships";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                  = INVALID_ID;                       // 4B: Primary Narreme selection buffer to filter/wake
    uint32_t relationship_row_offsets_buffer_id= INVALID_ID;                       // 4B: NarrativeRelationshipCSRBuffer::row_offsets (uint32_t, N+1)
    uint32_t relationship_edges_buffer_id      = INVALID_ID;                       // 4B: NarrativeRelationshipCSRBuffer::edges (RelationshipEdgePOD)
    uint32_t source_selection_buffer_id        = INVALID_ID;                       // 4B: Optional secondary buffer whose selection seeds traversal
    uint32_t narreme_type_buffer_id            = INVALID_ID;                       // 4B: Optional NarremeIdentitySoABuffer::narreme_type (uint8_t/uint32_t)
    uint32_t required_relationship_token       = 0u;                               // 4B: Interned StringName token ID for relationship title (0 = any)
    uint32_t source_or_target_narreme_id       = INVALID_ID;                       // 4B: Anchor Narreme ID for modes 0, 1, and 4
    int16_t min_affinity_weight                = -32768;                           // 2B: Minimum RelationshipEdgePOD::affinity_weight
    int16_t max_affinity_weight                = 32767;                            // 2B: Maximum RelationshipEdgePOD::affinity_weight
    uint16_t required_edge_flags               = 0u;                               // 2B: Required bitmask in RelationshipEdgePOD::flags (0 = any)
    uint16_t min_matching_degree               = 1u;                               // 2B: Minimum matching edge count for REL_QUERY_HAS_ANY_MATCHING_EDGE

    uint8_t traversal_mode                     = REL_QUERY_OUTGOING_TO_TARGET;     // 1B: RelationshipTraversalQueryMode (0..4)
    uint8_t max_traversal_hops                 = 1u;                               // 1B: Max BFS hops for INCOMING_FROM_SOURCE (1..4)
    uint8_t allowed_narreme_type_mask          = 0x3Fu;                            // 1B: Allowed NarremeType bitmask (bits 0..5)
    bool exclude_anchor_self                   = true;                             // 1B: Exclude source_or_target_narreme_id from matching itself
    bool negate_result                         = false;                            // 1B: Invert final match result
    uint8_t reserved_padding[23]               = {0};                              // 23B -> Total = 64B

    template <typename T_View>
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context) const noexcept {
        if constexpr (requires { p_view.bind_secondary(nullptr); }) {
            const uint32_t sec_id = (source_selection_buffer_id != INVALID_ID)
                ? source_selection_buffer_id
                : ((relationship_row_offsets_buffer_id != INVALID_ID) ? relationship_row_offsets_buffer_id : get_target_buffer_id());
            if (const GrantPartPOD* sec_part = p_context.get_grant_part(sec_id)) {
                p_view.bind_secondary(sec_part);
            }
        }
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary off_prop;
        off_prop["name"] = "relationship_row_offsets_buffer_id";
        off_prop["type"] = godot::Variant::INT;
        off_prop["hint_string"] = "buffer_option";
        props.push_back(off_prop);

        godot::Dictionary edg_prop;
        edg_prop["name"] = "relationship_edges_buffer_id";
        edg_prop["type"] = godot::Variant::INT;
        edg_prop["hint_string"] = "buffer_option";
        props.push_back(edg_prop);

        godot::Dictionary ssel_prop;
        ssel_prop["name"] = "source_selection_buffer_id";
        ssel_prop["type"] = godot::Variant::INT;
        ssel_prop["hint_string"] = "buffer_option";
        props.push_back(ssel_prop);

        godot::Dictionary typ_prop;
        typ_prop["name"] = "narreme_type_buffer_id";
        typ_prop["type"] = godot::Variant::INT;
        typ_prop["hint_string"] = "buffer_option";
        props.push_back(typ_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "traversal_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Outgoing Edge To Target:0,Incoming From Source (Multi-Hop):1,Connected To Source Selection:2,Has Minimum Matching Degree:3,Mutual Reciprocal Edge:4";
        props.push_back(mode_prop);

        godot::Dictionary tok_prop;
        tok_prop["name"] = "required_relationship_token";
        tok_prop["type"] = godot::Variant::INT;
        tok_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(tok_prop);

        godot::Dictionary anc_prop;
        anc_prop["name"] = "source_or_target_narreme_id";
        anc_prop["type"] = godot::Variant::INT;
        anc_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(anc_prop);

        godot::Dictionary min_aff_prop;
        min_aff_prop["name"] = "min_affinity_weight";
        min_aff_prop["type"] = godot::Variant::INT;
        min_aff_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        min_aff_prop["hint_string"] = "-32768,32767,1";
        props.push_back(min_aff_prop);

        godot::Dictionary max_aff_prop;
        max_aff_prop["name"] = "max_affinity_weight";
        max_aff_prop["type"] = godot::Variant::INT;
        max_aff_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        max_aff_prop["hint_string"] = "-32768,32767,1";
        props.push_back(max_aff_prop);

        godot::Dictionary eflg_prop;
        eflg_prop["name"] = "required_edge_flags";
        eflg_prop["type"] = godot::Variant::INT;
        eflg_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(eflg_prop);

        godot::Dictionary deg_prop;
        deg_prop["name"] = "min_matching_degree";
        deg_prop["type"] = godot::Variant::INT;
        deg_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        deg_prop["hint_string"] = "1,64,1";
        props.push_back(deg_prop);

        godot::Dictionary hop_prop;
        hop_prop["name"] = "max_traversal_hops";
        hop_prop["type"] = godot::Variant::INT;
        hop_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        hop_prop["hint_string"] = "1,4,1";
        props.push_back(hop_prop);

        godot::Dictionary nmsk_prop;
        nmsk_prop["name"] = "allowed_narreme_type_mask";
        nmsk_prop["type"] = godot::Variant::INT;
        nmsk_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        nmsk_prop["hint_string"] = "Character,Prop,Location,Incident,Plot,Narrative";
        props.push_back(nmsk_prop);

        godot::Dictionary ex_prop;
        ex_prop["name"] = "exclude_anchor_self";
        ex_prop["type"] = godot::Variant::BOOL;
        ex_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ex_prop);

        godot::Dictionary neg_prop;
        neg_prop["name"] = "negate_result";
        neg_prop["type"] = godot::Variant::BOOL;
        neg_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(neg_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                   return target_buffer_id;
        if (relationship_row_offsets_buffer_id != INVALID_ID) return relationship_row_offsets_buffer_id;
        return narreme_type_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("relationship_row_offsets_buffer_id")) {
            relationship_row_offsets_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["relationship_row_offsets_buffer_id"]));
        }
        if (p_props.has("relationship_edges_buffer_id")) {
            relationship_edges_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["relationship_edges_buffer_id"]));
        }
        if (p_props.has("source_selection_buffer_id")) {
            source_selection_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_selection_buffer_id"]));
        }
        if (p_props.has("narreme_type_buffer_id")) {
            narreme_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["narreme_type_buffer_id"]));
        }
        if (p_props.has("required_relationship_token")) {
            required_relationship_token = static_cast<uint32_t>(static_cast<int64_t>(p_props["required_relationship_token"]));
        }
        if (p_props.has("source_or_target_narreme_id")) {
            source_or_target_narreme_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_or_target_narreme_id"]));
        }
        if (p_props.has("min_affinity_weight")) {
            min_affinity_weight = static_cast<int16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["min_affinity_weight"]), -32768, 32767));
        }
        if (p_props.has("max_affinity_weight")) {
            max_affinity_weight = static_cast<int16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_affinity_weight"]), -32768, 32767));
        }
        if (p_props.has("required_edge_flags")) {
            required_edge_flags = static_cast<uint16_t>(static_cast<int64_t>(p_props["required_edge_flags"]));
        }
        if (p_props.has("min_matching_degree")) {
            min_matching_degree = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["min_matching_degree"]), 1, 65535));
        }
        if (p_props.has("traversal_mode")) {
            traversal_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["traversal_mode"]), 0, 4));
        }
        if (p_props.has("max_traversal_hops")) {
            max_traversal_hops = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_traversal_hops"]), 1, 4));
        }
        if (p_props.has("allowed_narreme_type_mask")) {
            allowed_narreme_type_mask = static_cast<uint8_t>(static_cast<int64_t>(p_props["allowed_narreme_type_mask"]) & 0x3F);
        }
        if (p_props.has("exclude_anchor_self")) {
            exclude_anchor_self = static_cast<bool>(p_props["exclude_anchor_self"]);
        }
        if (p_props.has("negate_result")) {
            negate_result = static_cast<bool>(p_props["negate_result"]);
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_context,
        const T_View& p_view
    ) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, int32_t>) {
            const uint32_t tgt_id = get_target_buffer_id();

            const GrantPartPOD* off_part  = (relationship_row_offsets_buffer_id != INVALID_ID) ? p_context.get_grant_part(relationship_row_offsets_buffer_id) : nullptr;
            const GrantPartPOD* edg_part  = (relationship_edges_buffer_id != INVALID_ID)       ? p_context.get_grant_part(relationship_edges_buffer_id)       : nullptr;
            const GrantPartPOD* ssel_part = (source_selection_buffer_id != INVALID_ID)         ? p_context.get_grant_part(source_selection_buffer_id)         : nullptr;
            const GrantPartPOD* typ_part  = (narreme_type_buffer_id != INVALID_ID)             ? p_context.get_grant_part(narreme_type_buffer_id)             : nullptr;

            if (!off_part || !off_part->raw_base_ptr || !edg_part || !edg_part->raw_base_ptr) return;

            const uint32_t* __restrict csr_offsets = reinterpret_cast<const uint32_t*>(off_part->raw_base_ptr);
            const int64_t off_cap = static_cast<int64_t>(off_part->capacity_bytes / sizeof(uint32_t));

            const RelationshipEdgePOD* __restrict csr_edges = (edg_part->element_stride == sizeof(RelationshipEdgePOD))
                ? reinterpret_cast<const RelationshipEdgePOD*>(edg_part->raw_base_ptr)
                : nullptr;
            const int64_t edg_cap = csr_edges ? static_cast<int64_t>(edg_part->capacity_bytes / sizeof(RelationshipEdgePOD)) : 0;
            if (!csr_edges) return;

            const MemoryBufferSelectionPOD* src_sel = (source_selection_buffer_id != INVALID_ID)
                ? p_context.get_selection(source_selection_buffer_id)
                : (ssel_part ? &ssel_part->selection : nullptr);

            const uint8_t* __restrict typ_raw = (typ_part && typ_part->raw_base_ptr) ? typ_part->raw_base_ptr : nullptr;
            const uint32_t typ_stride         = typ_part ? typ_part->element_stride : 0u;
            const int64_t typ_cap             = (typ_part && typ_stride > 0u) ? static_cast<int64_t>(typ_part->capacity_bytes / typ_stride) : 0;

            auto edge_matches_criteria = [&](const RelationshipEdgePOD& edge) noexcept -> bool {
                if (edge.target_narreme_id == INVALID_ID) return false;
                if (required_relationship_token != 0u && edge.relationship_title_token != required_relationship_token) {
                    return false;
                }
                if (edge.affinity_weight < min_affinity_weight || edge.affinity_weight > max_affinity_weight) {
                    return false;
                }
                if (required_edge_flags != 0u && (edge.flags & required_edge_flags) != required_edge_flags) {
                    return false;
                }
                return true;
            };

            auto has_direct_edge = [&](uint32_t from_id, uint32_t to_id) noexcept -> bool {
                const int64_t fidx = static_cast<int64_t>(from_id);
                if (fidx < 0 || (fidx + 1) >= off_cap) return false;
                const uint32_t start_e = csr_offsets[fidx];
                const uint32_t end_e   = std::min<uint32_t>(csr_offsets[fidx + 1], static_cast<uint32_t>(edg_cap));
                for (uint32_t e = start_e; e < end_e; ++e) {
                    const RelationshipEdgePOD& edge = csr_edges[e];
                    if (edge.target_narreme_id == to_id && edge_matches_criteria(edge)) {
                        return true;
                    }
                }
                return false;
            };

            auto is_in_source_selection = [&](uint32_t nid) noexcept -> bool {
                if (!src_sel || nid == INVALID_ID) return false;
                const int64_t nidx = static_cast<int64_t>(nid);
                if (nidx < 0 || nidx >= src_sel->capacity) return false;
                if (src_sel->mode == SelectionMode::DENSE && src_sel->data.bitset) {
                    return (src_sel->data.bitset[nidx >> 6] & (1ULL << (nidx & 63))) != 0ULL;
                }
                if (src_sel->mode == SelectionMode::SPARSE && src_sel->data.indices) {
                    for (int64_t i = 0; i < src_sel->element_count; ++i) {
                        if (src_sel->data.indices[i] == nidx) return true;
                    }
                    return false;
                }
                if (src_sel->mode == SelectionMode::RANGE) {
                    return nidx >= src_sel->start_index && nidx < (src_sel->start_index + src_sel->element_count);
                }
                return false;
            };

            uint32_t reachable_nodes[64];
            int32_t reachable_count = 0;
            if (traversal_mode == REL_QUERY_INCOMING_FROM_SOURCE && source_or_target_narreme_id != INVALID_ID) {
                uint8_t reachable_depths[64];
                int32_t q_head = 0;
                reachable_nodes[reachable_count]  = source_or_target_narreme_id;
                reachable_depths[reachable_count] = 0u;
                ++reachable_count;

                while (q_head < reachable_count) {
                    const uint32_t u  = reachable_nodes[q_head];
                    const uint8_t dep = reachable_depths[q_head];
                    ++q_head;

                    if (dep >= max_traversal_hops) continue;
                    const int64_t uidx = static_cast<int64_t>(u);
                    if (uidx < 0 || (uidx + 1) >= off_cap) continue;

                    const uint32_t start_e = csr_offsets[uidx];
                    const uint32_t end_e   = std::min<uint32_t>(csr_offsets[uidx + 1], static_cast<uint32_t>(edg_cap));
                    for (uint32_t e = start_e; e < end_e; ++e) {
                        const RelationshipEdgePOD& edge = csr_edges[e];
                        if (!edge_matches_criteria(edge)) continue;
                        const uint32_t v = edge.target_narreme_id;
                        bool seen = false;
                        for (int32_t k = 0; k < reachable_count; ++k) {
                            if (reachable_nodes[k] == v) { seen = true; break; }
                        }
                        if (!seen && reachable_count < 64) {
                            reachable_nodes[reachable_count]  = v;
                            reachable_depths[reachable_count] = static_cast<uint8_t>(dep + 1u);
                            ++reachable_count;
                        }
                    }
                }
            }

            auto eval_row = [&](int64_t idx) noexcept -> bool {
                const uint32_t uid = static_cast<uint32_t>(idx);
                if (exclude_anchor_self && source_or_target_narreme_id != INVALID_ID && uid == source_or_target_narreme_id &&
                    traversal_mode != REL_QUERY_HAS_ANY_MATCHING_EDGE) {
                    return negate_result ? true : false;
                }

                if (typ_raw && idx >= 0 && idx < typ_cap && allowed_narreme_type_mask != 0u) {
                    const uint8_t ntype = (typ_stride == sizeof(uint32_t))
                        ? static_cast<uint8_t>(reinterpret_cast<const uint32_t*>(typ_raw)[idx])
                        : typ_raw[idx];
                    if (ntype < 6u && ((allowed_narreme_type_mask & (1u << ntype)) == 0u)) {
                        return negate_result ? true : false;
                    }
                }

                bool match = false;
                switch (static_cast<RelationshipTraversalQueryMode>(traversal_mode)) {
                    case REL_QUERY_OUTGOING_TO_TARGET: {
                        if (source_or_target_narreme_id != INVALID_ID) {
                            match = has_direct_edge(uid, source_or_target_narreme_id);
                        }
                        break;
                    }
                    case REL_QUERY_INCOMING_FROM_SOURCE: {
                        if (max_traversal_hops <= 1u && source_or_target_narreme_id != INVALID_ID) {
                            match = has_direct_edge(source_or_target_narreme_id, uid);
                        } else {
                            for (int32_t k = (exclude_anchor_self ? 1 : 0); k < reachable_count; ++k) {
                                if (reachable_nodes[k] == uid) {
                                    match = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    case REL_QUERY_CONNECTED_TO_SELECTION: {
                        if (idx >= 0 && (idx + 1) < off_cap) {
                            const uint32_t start_e = csr_offsets[idx];
                            const uint32_t end_e   = std::min<uint32_t>(csr_offsets[idx + 1], static_cast<uint32_t>(edg_cap));
                            for (uint32_t e = start_e; e < end_e; ++e) {
                                const RelationshipEdgePOD& edge = csr_edges[e];
                                if (edge_matches_criteria(edge) && is_in_source_selection(edge.target_narreme_id)) {
                                    match = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    case REL_QUERY_HAS_ANY_MATCHING_EDGE: {
                        if (idx >= 0 && (idx + 1) < off_cap) {
                            const uint32_t start_e = csr_offsets[idx];
                            const uint32_t end_e   = std::min<uint32_t>(csr_offsets[idx + 1], static_cast<uint32_t>(edg_cap));
                            uint16_t count = 0u;
                            for (uint32_t e = start_e; e < end_e; ++e) {
                                if (edge_matches_criteria(csr_edges[e])) {
                                    if (++count >= min_matching_degree) {
                                        match = true;
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case REL_QUERY_MUTUAL_RECIPROCAL_EDGE: {
                        if (source_or_target_narreme_id != INVALID_ID) {
                            match = has_direct_edge(uid, source_or_target_narreme_id) &&
                                    has_direct_edge(source_or_target_narreme_id, uid);
                        }
                        break;
                    }
                    default:
                        break;
                }

                return negate_result ? !match : match;
            };

            _apply_query_op<Op>(r_selection, p_context, tgt_id, eval_row);
        }
    }

private:
    template <QueryOp Op, typename PredFn>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _apply_query_op(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_context,
        uint32_t p_target_id,
        PredFn&& p_eval
    ) noexcept {
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                uint64_t* bitset = r_selection.data.bitset;
                for (int64_t i = 0; i < r_selection.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        if (!p_eval(i)) {
                            bitset[i >> 6] &= ~(1ULL << (i & 63));
                            --r_selection.element_count;
                        }
                    }
                }
            } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                int64_t write_ptr = 0;
                int64_t* indices  = r_selection.data.indices;
                for (int64_t i = 0; i < r_selection.element_count; ++i) {
                    const int64_t idx = indices[i];
                    if (p_eval(idx)) {
                        indices[write_ptr++] = idx;
                    }
                }
                r_selection.element_count = write_ptr;
            } else if (r_selection.mode == SelectionMode::RANGE && r_selection.data.indices) {
                const int64_t start = std::max<int64_t>(0, r_selection.start_index);
                const int64_t end   = std::min<int64_t>(start + r_selection.element_count, r_selection.capacity);
                int64_t write_ptr   = 0;
                for (int64_t i = start; i < end; ++i) {
                    if (p_eval(i)) {
                        r_selection.data.indices[write_ptr++] = i;
                    }
                }
                r_selection.mode          = SelectionMode::SPARSE;
                r_selection.start_index   = 0;
                r_selection.element_count = write_ptr;
            }
        } else if constexpr (Op == QueryOp::ADD) {
            if (r_selection.unclaimed_mask) {
                const uint64_t* unclaimed = r_selection.unclaimed_mask;
                const int64_t words       = (r_selection.capacity + 63) >> 6;
                for (int64_t w = 0; w < words; ++w) {
                    uint64_t mask = unclaimed[w];
                    while (mask != 0ULL) {
                        const int bit_index      = std::countr_zero(mask);
                        const int64_t global_idx = (w << 6) + bit_index;
                        if (global_idx >= r_selection.capacity) break;
                        if (p_eval(global_idx)) {
                            p_context.queue_selection_command(p_target_id, global_idx);
                        }
                        mask &= (mask - 1ULL);
                    }
                }
            } else {
                for (int64_t i = 0; i < r_selection.capacity; ++i) {
                    if (p_eval(i)) {
                        p_context.queue_selection_command(p_target_id, i);
                    }
                }
            }
        }
    }
};

static_assert(sizeof(RelationshipGraphTraversalQueryLogic<>) == 64, "RelationshipGraphTraversalQueryLogic must be exactly 64 bytes!");

} // namespace ideam::core