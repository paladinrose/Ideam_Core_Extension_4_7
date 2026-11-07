#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../transform_logic/narreme_symbolic_similarity_transform_logic.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_LOCATION_OCCUPANCY_QUERY_ENUMS_DEFINED
#define IDEAM_NARRATIVES_LOCATION_OCCUPANCY_QUERY_ENUMS_DEFINED

/**
 * Spatial occupancy query modes for LocationOccupancyQueryLogic (N13),
 * mirroring Narrative_Location::is_character_present(), is_prop_present(),
 * get_characters(), and get_props() in src/godot/narratives/location.cpp.
 */
enum LocationOccupancyQueryMode : uint8_t {
    LOC_OCCUPANCY_AT_TARGET_LOCATION     = 0, // Matches narremes whose current_location_id == target_location_id (or within max_location_hops)
    LOC_OCCUPANCY_IN_SELECTED_LOCATIONS  = 1, // Matches narremes whose current_location_id is selected in location_selection_buffer_id
    LOC_OCCUPANCY_OWNED_BY_TARGET_CHAR   = 2, // Matches props whose owner_character_id == target_character_id
    LOC_OCCUPANCY_CO_LOCATED_WITH_NARREME= 3, // Matches narremes at the same current_location_id as reference_narreme_id
    LOC_OCCUPANCY_UNPLACED_NARREMES      = 4  // Matches narremes with current_location_id == INVALID_ID and owner_character_id == INVALID_ID
};

#endif // IDEAM_NARRATIVES_LOCATION_OCCUPANCY_QUERY_ENUMS_DEFINED

/**
 * LocationOccupancyQueryLogic<T> (N13)
 * Selects or culls all Characters and Props currently present inside a target Location,
 * a selected set of Locations, or within K graph/hierarchy hops of a Location.
 * Supports resolving a Prop's effective location through its owning Character's
 * current_location_id when include_carried_inventory_props is enabled.
 */
template <typename T = int32_t>
struct alignas(64) LocationOccupancyQueryLogic {
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

    static constexpr std::string_view display_name = "Location Occupancy";
    static constexpr std::string_view ui_category  = "Spatial";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                = INVALID_ID;                         // 4B: Primary Narreme selection buffer to filter/wake
    uint32_t current_location_id_buffer_id   = INVALID_ID;                         // 4B: NarremeIdentitySoABuffer::current_location_id (uint32_t)
    uint32_t owner_character_id_buffer_id    = INVALID_ID;                         // 4B: NarremeIdentitySoABuffer::owner_character_id (uint32_t)
    uint32_t narreme_type_buffer_id          = INVALID_ID;                         // 4B: Optional NarremeIdentitySoABuffer::narreme_type (uint8_t/uint32_t)
    uint32_t location_selection_buffer_id    = INVALID_ID;                         // 4B: Secondary Location buffer whose selection defines active locations
    uint32_t location_csr_offsets_buffer_id  = INVALID_ID;                         // 4B: Optional Location adjacency CSR row_offsets (uint32_t)
    uint32_t location_csr_neighbors_buffer_id= INVALID_ID;                         // 4B: Optional Location adjacency CSR neighbor location IDs (uint32_t)
    uint32_t location_parent_id_buffer_id    = INVALID_ID;                         // 4B: Optional parent_location_id column for hierarchical regions
    uint32_t target_location_id              = INVALID_ID;                         // 4B: Target Location index for LOC_OCCUPANCY_AT_TARGET_LOCATION
    uint32_t target_character_or_ref_id      = INVALID_ID;                         // 4B: Target Character ID (mode 2) or Reference Narreme ID (mode 3)

    uint8_t occupancy_mode                   = LOC_OCCUPANCY_AT_TARGET_LOCATION;   // 1B: LocationOccupancyQueryMode (0..4)
    uint8_t allowed_narreme_type_mask        = (1u << NARREME_TYPE_CHARACTER) | (1u << NARREME_TYPE_PROP); // 1B: Bitmask of NarremeType (0..5)
    uint8_t max_location_hops                = 0u;                                 // 1B: Max graph/parent hops around target_location_id (0 = exact)
    bool include_carried_inventory_props     = true;                               // 1B: Resolve prop location via owner_character_id's current_location_id
    bool exclude_reference_self              = false;                              // 1B: Exclude target_character_or_ref_id itself in CO_LOCATED mode
    bool negate_result                       = false;                              // 1B: Invert occupancy match result
    uint8_t reserved_padding[18]             = {0};                                // 18B -> Total = 64B

    template <typename T_View>
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context) const noexcept {
        if constexpr (requires { p_view.bind_secondary(nullptr); }) {
            const uint32_t sec_id = (location_selection_buffer_id != INVALID_ID)
                ? location_selection_buffer_id
                : ((owner_character_id_buffer_id != INVALID_ID) ? owner_character_id_buffer_id : get_target_buffer_id());
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

        godot::Dictionary loc_prop;
        loc_prop["name"] = "current_location_id_buffer_id";
        loc_prop["type"] = godot::Variant::INT;
        loc_prop["hint_string"] = "buffer_option";
        props.push_back(loc_prop);

        godot::Dictionary own_prop;
        own_prop["name"] = "owner_character_id_buffer_id";
        own_prop["type"] = godot::Variant::INT;
        own_prop["hint_string"] = "buffer_option";
        props.push_back(own_prop);

        godot::Dictionary typ_prop;
        typ_prop["name"] = "narreme_type_buffer_id";
        typ_prop["type"] = godot::Variant::INT;
        typ_prop["hint_string"] = "buffer_option";
        props.push_back(typ_prop);

        godot::Dictionary lsel_prop;
        lsel_prop["name"] = "location_selection_buffer_id";
        lsel_prop["type"] = godot::Variant::INT;
        lsel_prop["hint_string"] = "buffer_option";
        props.push_back(lsel_prop);

        godot::Dictionary coff_prop;
        coff_prop["name"] = "location_csr_offsets_buffer_id";
        coff_prop["type"] = godot::Variant::INT;
        coff_prop["hint_string"] = "buffer_option";
        props.push_back(coff_prop);

        godot::Dictionary cnb_prop;
        cnb_prop["name"] = "location_csr_neighbors_buffer_id";
        cnb_prop["type"] = godot::Variant::INT;
        cnb_prop["hint_string"] = "buffer_option";
        props.push_back(cnb_prop);

        godot::Dictionary lpar_prop;
        lpar_prop["name"] = "location_parent_id_buffer_id";
        lpar_prop["type"] = godot::Variant::INT;
        lpar_prop["hint_string"] = "buffer_option";
        props.push_back(lpar_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "occupancy_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "At Target Location:0,In Selected Locations:1,Owned By Target Character:2,Co-Located With Narreme:3,Unplaced Narremes:4";
        props.push_back(mode_prop);

        godot::Dictionary tloc_prop;
        tloc_prop["name"] = "target_location_id";
        tloc_prop["type"] = godot::Variant::INT;
        tloc_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(tloc_prop);

        godot::Dictionary tref_prop;
        tref_prop["name"] = "target_character_or_ref_id";
        tref_prop["type"] = godot::Variant::INT;
        tref_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(tref_prop);

        godot::Dictionary nmsk_prop;
        nmsk_prop["name"] = "allowed_narreme_type_mask";
        nmsk_prop["type"] = godot::Variant::INT;
        nmsk_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        nmsk_prop["hint_string"] = "Character,Prop,Location,Incident,Plot,Narrative";
        props.push_back(nmsk_prop);

        godot::Dictionary hop_prop;
        hop_prop["name"] = "max_location_hops";
        hop_prop["type"] = godot::Variant::INT;
        hop_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        hop_prop["hint_string"] = "0,8,1";
        props.push_back(hop_prop);

        godot::Dictionary inv_prop;
        inv_prop["name"] = "include_carried_inventory_props";
        inv_prop["type"] = godot::Variant::BOOL;
        inv_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(inv_prop);

        godot::Dictionary ex_prop;
        ex_prop["name"] = "exclude_reference_self";
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
        if (target_buffer_id != INVALID_ID)              return target_buffer_id;
        if (current_location_id_buffer_id != INVALID_ID) return current_location_id_buffer_id;
        return owner_character_id_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("current_location_id_buffer_id")) {
            current_location_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["current_location_id_buffer_id"]));
        }
        if (p_props.has("owner_character_id_buffer_id")) {
            owner_character_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["owner_character_id_buffer_id"]));
        }
        if (p_props.has("narreme_type_buffer_id")) {
            narreme_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["narreme_type_buffer_id"]));
        }
        if (p_props.has("location_selection_buffer_id")) {
            location_selection_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["location_selection_buffer_id"]));
        }
        if (p_props.has("location_csr_offsets_buffer_id")) {
            location_csr_offsets_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["location_csr_offsets_buffer_id"]));
        }
        if (p_props.has("location_csr_neighbors_buffer_id")) {
            location_csr_neighbors_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["location_csr_neighbors_buffer_id"]));
        }
        if (p_props.has("location_parent_id_buffer_id")) {
            location_parent_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["location_parent_id_buffer_id"]));
        }
        if (p_props.has("target_location_id")) {
            target_location_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_location_id"]));
        }
        if (p_props.has("target_character_or_ref_id")) {
            target_character_or_ref_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_character_or_ref_id"]));
        }
        if (p_props.has("occupancy_mode")) {
            occupancy_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["occupancy_mode"]), 0, 4));
        }
        if (p_props.has("allowed_narreme_type_mask")) {
            allowed_narreme_type_mask = static_cast<uint8_t>(static_cast<int64_t>(p_props["allowed_narreme_type_mask"]) & 0x3F);
        }
        if (p_props.has("max_location_hops")) {
            max_location_hops = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_location_hops"]), 0, 8));
        }
        if (p_props.has("include_carried_inventory_props")) {
            include_carried_inventory_props = static_cast<bool>(p_props["include_carried_inventory_props"]);
        }
        if (p_props.has("exclude_reference_self")) {
            exclude_reference_self = static_cast<bool>(p_props["exclude_reference_self"]);
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
            const uint32_t loc_id = (current_location_id_buffer_id != INVALID_ID) ? current_location_id_buffer_id : tgt_id;

            const GrantPartPOD* loc_part  = (loc_id != INVALID_ID)                           ? p_context.get_grant_part(loc_id)                           : nullptr;
            const GrantPartPOD* own_part  = (owner_character_id_buffer_id != INVALID_ID)     ? p_context.get_grant_part(owner_character_id_buffer_id)     : nullptr;
            const GrantPartPOD* typ_part  = (narreme_type_buffer_id != INVALID_ID)           ? p_context.get_grant_part(narreme_type_buffer_id)           : nullptr;
            const GrantPartPOD* lsel_part = (location_selection_buffer_id != INVALID_ID)     ? p_context.get_grant_part(location_selection_buffer_id)     : nullptr;
            const GrantPartPOD* coff_part = (location_csr_offsets_buffer_id != INVALID_ID)   ? p_context.get_grant_part(location_csr_offsets_buffer_id)   : nullptr;
            const GrantPartPOD* cnb_part  = (location_csr_neighbors_buffer_id != INVALID_ID) ? p_context.get_grant_part(location_csr_neighbors_buffer_id) : nullptr;
            const GrantPartPOD* lpar_part = (location_parent_id_buffer_id != INVALID_ID)     ? p_context.get_grant_part(location_parent_id_buffer_id)     : nullptr;

            const uint32_t* __restrict loc_ids = (loc_part && loc_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(loc_part->raw_base_ptr)
                : nullptr;
            const int64_t loc_cap = loc_ids ? static_cast<int64_t>(loc_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint32_t* __restrict owner_ids = (own_part && own_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(own_part->raw_base_ptr)
                : nullptr;
            const int64_t own_cap = owner_ids ? static_cast<int64_t>(own_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict typ_raw = (typ_part && typ_part->raw_base_ptr) ? typ_part->raw_base_ptr : nullptr;
            const uint32_t typ_stride         = typ_part ? typ_part->element_stride : 0u;
            const int64_t typ_cap             = (typ_part && typ_stride > 0u) ? static_cast<int64_t>(typ_part->capacity_bytes / typ_stride) : 0;

            const MemoryBufferSelectionPOD* loc_sel = (location_selection_buffer_id != INVALID_ID)
                ? p_context.get_selection(location_selection_buffer_id)
                : (lsel_part ? &lsel_part->selection : nullptr);

            const uint32_t* __restrict csr_offsets = (coff_part && coff_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(coff_part->raw_base_ptr)
                : nullptr;
            const int64_t coff_cap = csr_offsets ? static_cast<int64_t>(coff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint32_t* __restrict csr_neighbors = (cnb_part && cnb_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(cnb_part->raw_base_ptr)
                : nullptr;
            const int64_t cnb_cap = csr_neighbors ? static_cast<int64_t>(cnb_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint32_t* __restrict loc_parents = (lpar_part && lpar_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(lpar_part->raw_base_ptr)
                : nullptr;
            const int64_t lpar_cap = loc_parents ? static_cast<int64_t>(lpar_part->capacity_bytes / sizeof(uint32_t)) : 0;

            auto resolve_effective_location = [&](int64_t idx) noexcept -> uint32_t {
                uint32_t lid = (loc_ids && idx >= 0 && idx < loc_cap) ? loc_ids[idx] : INVALID_ID;
                if (include_carried_inventory_props && owner_ids && idx >= 0 && idx < own_cap) {
                    const uint32_t owner_id = owner_ids[idx];
                    if (owner_id != INVALID_ID && loc_ids && static_cast<int64_t>(owner_id) < loc_cap) {
                        const uint32_t owner_loc = loc_ids[owner_id];
                        if (owner_loc != INVALID_ID) {
                            lid = owner_loc;
                        }
                    }
                }
                return lid;
            };

            auto is_location_in_selection = [&](uint32_t lid) noexcept -> bool {
                if (!loc_sel || lid == INVALID_ID) return false;
                const int64_t lidx = static_cast<int64_t>(lid);
                if (lidx < 0 || lidx >= loc_sel->capacity) return false;
                if (loc_sel->mode == SelectionMode::DENSE && loc_sel->data.bitset) {
                    return (loc_sel->data.bitset[lidx >> 6] & (1ULL << (lidx & 63))) != 0ULL;
                }
                if (loc_sel->mode == SelectionMode::SPARSE && loc_sel->data.indices) {
                    for (int64_t i = 0; i < loc_sel->element_count; ++i) {
                        if (loc_sel->data.indices[i] == lidx) return true;
                    }
                    return false;
                }
                if (loc_sel->mode == SelectionMode::RANGE) {
                    return lidx >= loc_sel->start_index && lidx < (loc_sel->start_index + loc_sel->element_count);
                }
                return false;
            };

            auto matches_location_within_hops = [&](uint32_t candidate_loc, uint32_t goal_loc) noexcept -> bool {
                if (candidate_loc == INVALID_ID || goal_loc == INVALID_ID) return false;
                if (candidate_loc == goal_loc) return true;
                if (max_location_hops == 0u) return false;

                if (loc_parents) {
                    uint32_t cur = candidate_loc;
                    for (uint8_t h = 0u; h < max_location_hops && cur != INVALID_ID; ++h) {
                        const int64_t cidx = static_cast<int64_t>(cur);
                        if (cidx < 0 || cidx >= lpar_cap) break;
                        const uint32_t par = loc_parents[cidx];
                        if (par == goal_loc) return true;
                        if (par == cur) break;
                        cur = par;
                    }
                }

                if (csr_offsets && csr_neighbors) {
                    uint32_t queue_nodes[32];
                    uint8_t queue_depths[32];
                    int32_t q_head = 0;
                    int32_t q_tail = 0;

                    queue_nodes[q_tail]  = goal_loc;
                    queue_depths[q_tail] = 0u;
                    ++q_tail;

                    while (q_head < q_tail) {
                        const uint32_t u   = queue_nodes[q_head];
                        const uint8_t dep  = queue_depths[q_head];
                        ++q_head;

                        if (dep >= max_location_hops) continue;
                        const int64_t uidx = static_cast<int64_t>(u);
                        if (uidx < 0 || (uidx + 1) >= coff_cap) continue;

                        const uint32_t start_e = csr_offsets[uidx];
                        const uint32_t end_e   = std::min<uint32_t>(csr_offsets[uidx + 1], static_cast<uint32_t>(cnb_cap));
                        for (uint32_t e = start_e; e < end_e; ++e) {
                            const uint32_t nb = csr_neighbors[e];
                            if (nb == candidate_loc) return true;
                            if (q_tail < 32) {
                                bool seen = false;
                                for (int32_t k = 0; k < q_tail; ++k) {
                                    if (queue_nodes[k] == nb) { seen = true; break; }
                                }
                                if (!seen) {
                                    queue_nodes[q_tail]  = nb;
                                    queue_depths[q_tail] = static_cast<uint8_t>(dep + 1u);
                                    ++q_tail;
                                }
                            }
                        }
                    }
                }

                return false;
            };

            const uint32_t ref_location_id = (occupancy_mode == LOC_OCCUPANCY_CO_LOCATED_WITH_NARREME && target_character_or_ref_id != INVALID_ID)
                ? resolve_effective_location(static_cast<int64_t>(target_character_or_ref_id))
                : target_location_id;

            auto eval_row = [&](int64_t idx) noexcept -> bool {
                if (typ_raw && idx >= 0 && idx < typ_cap && allowed_narreme_type_mask != 0u) {
                    const uint8_t ntype = (typ_stride == sizeof(uint32_t))
                        ? static_cast<uint8_t>(reinterpret_cast<const uint32_t*>(typ_raw)[idx])
                        : typ_raw[idx];
                    if (ntype < 6u && ((allowed_narreme_type_mask & (1u << ntype)) == 0u)) {
                        return negate_result ? true : false;
                    }
                }

                bool match = false;
                switch (static_cast<LocationOccupancyQueryMode>(occupancy_mode)) {
                    case LOC_OCCUPANCY_AT_TARGET_LOCATION: {
                        const uint32_t eff_loc = resolve_effective_location(idx);
                        match = matches_location_within_hops(eff_loc, target_location_id);
                        break;
                    }
                    case LOC_OCCUPANCY_IN_SELECTED_LOCATIONS: {
                        const uint32_t eff_loc = resolve_effective_location(idx);
                        match = is_location_in_selection(eff_loc);
                        break;
                    }
                    case LOC_OCCUPANCY_OWNED_BY_TARGET_CHAR: {
                        if (owner_ids && idx >= 0 && idx < own_cap && target_character_or_ref_id != INVALID_ID) {
                            match = (owner_ids[idx] == target_character_or_ref_id);
                        }
                        break;
                    }
                    case LOC_OCCUPANCY_CO_LOCATED_WITH_NARREME: {
                        if (exclude_reference_self && static_cast<uint32_t>(idx) == target_character_or_ref_id) {
                            match = false;
                        } else {
                            const uint32_t eff_loc = resolve_effective_location(idx);
                            match = matches_location_within_hops(eff_loc, ref_location_id);
                        }
                        break;
                    }
                    case LOC_OCCUPANCY_UNPLACED_NARREMES: {
                        const uint32_t dir_loc = (loc_ids && idx >= 0 && idx < loc_cap) ? loc_ids[idx] : INVALID_ID;
                        const uint32_t dir_own = (owner_ids && idx >= 0 && idx < own_cap) ? owner_ids[idx] : INVALID_ID;
                        match = (dir_loc == INVALID_ID) && (dir_own == INVALID_ID);
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

static_assert(sizeof(LocationOccupancyQueryLogic<>) == 64, "LocationOccupancyQueryLogic must be exactly 64 bytes!");

} // namespace ideam::core