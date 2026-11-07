#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "narreme_symbolic_similarity_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_SPATIAL_TRANSFER_POD_DEFINED
#define IDEAM_NARRATIVES_SPATIAL_TRANSFER_POD_DEFINED

/**
 * Operations supported by NarremeLocationTransferTransformLogic (N10),
 * mirroring Character and Prop spatial/possession methods in character.cpp, prop.cpp, and location.cpp.
 */
enum NarremeSpatialTransferOp : uint8_t {
    SPATIAL_OP_ENTER_LOCATION = 0, // Character::enter_location / Prop::enter_location / Location::*_entrance
    SPATIAL_OP_LEAVE_LOCATION = 1, // Character::leave_current_location / Prop::leave_current_location / Location::*_exit
    SPATIAL_OP_PROP_TAKEN_BY  = 2, // Prop::taken_by(new_possessor) / Character::take_possession
    SPATIAL_OP_PROP_RELEASED  = 3, // Prop::released_by(old_possessor) / Character::lose_possession
    SPATIAL_OP_PROP_LOST      = 4, // Prop::become_lost() (clears both possessor_id and location_id)
    SPATIAL_OP_PROP_FOUND_BY  = 5  // Prop::found_by(finder) (assigns possessor_id and syncs location_id)
};

/**
 * Transition bitflags written to spatial_transition_flags_buffer_id for N11 signal emission.
 */
enum NarremeSpatialTransitionFlags : uint8_t {
    SPATIAL_TRANSITION_NONE             = 0u,
    SPATIAL_TRANSITION_LOCATION_ENTERED = 1u << 0, // Emits "location_entered" & Location "character_entered"/"prop_entered"
    SPATIAL_TRANSITION_LOCATION_EXITED  = 1u << 1, // Emits "location_exited"  & Location "character_exited"/"prop_exited"
    SPATIAL_TRANSITION_PROP_TAKEN       = 1u << 2, // Emits Prop "taken" & Character "possession_taken"
    SPATIAL_TRANSITION_PROP_RELEASED    = 1u << 3, // Emits Prop "released" & Character "possession_lost"
    SPATIAL_TRANSITION_PROP_LOST        = 1u << 4, // Emits Prop "became_lost"
    SPATIAL_TRANSITION_PROP_FOUND       = 1u << 5  // Emits Prop "found" & Character "possession_taken"
};

/**
 * NarremeSpatialTransferCommandPOD (16-Byte Aligned)
 * Queued command record for batched location / possession transfers across thousands of Characters and Props.
 */
struct alignas(16) NarremeSpatialTransferCommandPOD {
    uint32_t narreme_id     = INVALID_ID;                // 4B: Target Character or Prop ID in NarremeIdentityAndTraitBuffer
    uint32_t target_id      = INVALID_ID;                // 4B: Destination Location ID or Possessor Character ID
    uint8_t transfer_op     = SPATIAL_OP_ENTER_LOCATION; // 1B: NarremeSpatialTransferOp
    uint8_t narreme_type    = NARREME_TYPE_CHARACTER;    // 1B: NarremeType (NARREME_TYPE_CHARACTER or NARREME_TYPE_PROP)
    uint16_t flags          = 0u;                        // 2B: Reserved command flags
    uint32_t previous_id    = INVALID_ID;                // 4B: Optional expected previous Location/Possessor -> Total = 16B
};

static_assert(sizeof(NarremeSpatialTransferCommandPOD) == 16, "NarremeSpatialTransferCommandPOD must be 16 bytes!");

#endif // IDEAM_NARRATIVES_SPATIAL_TRANSFER_POD_DEFINED

/**
 * NarremeLocationTransferTransformLogic<T> (N10)
 * Processes batched enter_location / leave_current_location and taken_by / released_by / become_lost /
 * found_by commands (from a NarremeSpatialTransferCommandPOD ring queue or pending target columns)
 * for thousands of Characters and Props, updating NarremeIdentityAndTraitBuffer (location_id,
 * possessor_id) and Location occupancy counts in a single pass.
 */
template <typename T = int32_t>
struct alignas(64) NarremeLocationTransferTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Narreme Location Transfer";
    static constexpr std::string_view ui_category  = "Spatial";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t location_id_buffer_id              = INVALID_ID; // 4B: Primary target (NarremeIdentityAndTraitBuffer::location_id)
    uint32_t possessor_id_buffer_id             = INVALID_ID; // 4B: NarremeIdentityAndTraitBuffer::possessor_id (uint32_t)
    uint32_t pending_location_id_buffer_id      = INVALID_ID; // 4B: Optional staged new location_id per Narreme
    uint32_t pending_possessor_id_buffer_id     = INVALID_ID; // 4B: Optional staged new possessor_id per Prop
    uint32_t previous_location_id_buffer_id     = INVALID_ID; // 4B: Optional column recording exited location_id for N11 signals
    uint32_t previous_possessor_id_buffer_id    = INVALID_ID; // 4B: Optional column recording previous possessor_id for N11 signals
    uint32_t narreme_type_buffer_id             = INVALID_ID; // 4B: NarremeIdentityAndTraitBuffer::narreme_type (uint8_t)
    uint32_t spatial_transition_flags_buffer_id = INVALID_ID; // 4B: Optional uint8_t NarremeSpatialTransitionFlags column
    uint32_t location_char_count_buffer_id      = INVALID_ID; // 4B: Optional uint32_t/int32_t character occupancy count per Location
    uint32_t location_prop_count_buffer_id      = INVALID_ID; // 4B: Optional uint32_t/int32_t prop occupancy count per Location
    uint32_t transfer_command_ring_buffer_id    = INVALID_ID; // 4B: Optional NarremeSpatialTransferCommandPOD ring buffer
    uint32_t ring_cursor_buffer_id              = INVALID_ID; // 4B: Optional uint32_t[2] [read_idx, write_idx] ring cursor buffer

    uint8_t default_transfer_op                 = SPATIAL_OP_ENTER_LOCATION; // 1B: Default operation for staged column transfers
    bool sync_possessed_prop_locations          = true;       // 1B: Automatically co-locate possessed Props with their possessor Character
    bool update_location_occupancy_counts       = true;       // 1B: Increment/decrement location_char_count and location_prop_count
    bool clear_pending_after_apply              = true;       // 1B: Reset pending_location_id / pending_possessor_id to INVALID_ID
    bool wake_affected_narremes                 = true;       // 1B: Queue moved/transferred narreme_id into TaskContextPOD::wave_commands
    uint8_t reserved_padding[11]                = {0};        // 11B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

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

        godot::Dictionary ploc_prop;
        ploc_prop["name"] = "pending_location_id_buffer_id";
        ploc_prop["type"] = godot::Variant::INT;
        ploc_prop["hint_string"] = "buffer_option";
        props.push_back(ploc_prop);

        godot::Dictionary ppos_prop;
        ppos_prop["name"] = "pending_possessor_id_buffer_id";
        ppos_prop["type"] = godot::Variant::INT;
        ppos_prop["hint_string"] = "buffer_option";
        props.push_back(ppos_prop);

        godot::Dictionary pvloc_prop;
        pvloc_prop["name"] = "previous_location_id_buffer_id";
        pvloc_prop["type"] = godot::Variant::INT;
        pvloc_prop["hint_string"] = "buffer_option";
        props.push_back(pvloc_prop);

        godot::Dictionary pvpos_prop;
        pvpos_prop["name"] = "previous_possessor_id_buffer_id";
        pvpos_prop["type"] = godot::Variant::INT;
        pvpos_prop["hint_string"] = "buffer_option";
        props.push_back(pvpos_prop);

        godot::Dictionary ntyp_prop;
        ntyp_prop["name"] = "narreme_type_buffer_id";
        ntyp_prop["type"] = godot::Variant::INT;
        ntyp_prop["hint_string"] = "buffer_option";
        props.push_back(ntyp_prop);

        godot::Dictionary trn_prop;
        trn_prop["name"] = "spatial_transition_flags_buffer_id";
        trn_prop["type"] = godot::Variant::INT;
        trn_prop["hint_string"] = "buffer_option";
        props.push_back(trn_prop);

        godot::Dictionary lcc_prop;
        lcc_prop["name"] = "location_char_count_buffer_id";
        lcc_prop["type"] = godot::Variant::INT;
        lcc_prop["hint_string"] = "buffer_option";
        props.push_back(lcc_prop);

        godot::Dictionary lpc_prop;
        lpc_prop["name"] = "location_prop_count_buffer_id";
        lpc_prop["type"] = godot::Variant::INT;
        lpc_prop["hint_string"] = "buffer_option";
        props.push_back(lpc_prop);

        godot::Dictionary rng_prop;
        rng_prop["name"] = "transfer_command_ring_buffer_id";
        rng_prop["type"] = godot::Variant::INT;
        rng_prop["hint_string"] = "buffer_option";
        props.push_back(rng_prop);

        godot::Dictionary rcur_prop;
        rcur_prop["name"] = "ring_cursor_buffer_id";
        rcur_prop["type"] = godot::Variant::INT;
        rcur_prop["hint_string"] = "buffer_option";
        props.push_back(rcur_prop);

        godot::Dictionary dop_prop;
        dop_prop["name"] = "default_transfer_op";
        dop_prop["type"] = godot::Variant::INT;
        dop_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dop_prop["hint_string"] = "Enter Location:0,Leave Location:1,Prop Taken By:2,Prop Released:3,Prop Become Lost:4,Prop Found By:5";
        props.push_back(dop_prop);

        godot::Dictionary spl_prop;
        spl_prop["name"] = "sync_possessed_prop_locations";
        spl_prop["type"] = godot::Variant::BOOL;
        spl_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(spl_prop);

        godot::Dictionary occ_prop;
        occ_prop["name"] = "update_location_occupancy_counts";
        occ_prop["type"] = godot::Variant::BOOL;
        occ_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(occ_prop);

        godot::Dictionary clp_prop;
        clp_prop["name"] = "clear_pending_after_apply";
        clp_prop["type"] = godot::Variant::BOOL;
        clp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clp_prop);

        godot::Dictionary wak_prop;
        wak_prop["name"] = "wake_affected_narremes";
        wak_prop["type"] = godot::Variant::BOOL;
        wak_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wak_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (location_id_buffer_id != INVALID_ID)           return location_id_buffer_id;
        if (possessor_id_buffer_id != INVALID_ID)          return possessor_id_buffer_id;
        return transfer_command_ring_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("location_id_buffer_id")) {
            location_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["location_id_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && location_id_buffer_id == INVALID_ID) {
            location_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("possessor_id_buffer_id")) {
            possessor_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["possessor_id_buffer_id"]));
        }
        if (p_props.has("pending_location_id_buffer_id")) {
            pending_location_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["pending_location_id_buffer_id"]));
        }
        if (p_props.has("pending_possessor_id_buffer_id")) {
            pending_possessor_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["pending_possessor_id_buffer_id"]));
        }
        if (p_props.has("previous_location_id_buffer_id")) {
            previous_location_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["previous_location_id_buffer_id"]));
        }
        if (p_props.has("previous_possessor_id_buffer_id")) {
            previous_possessor_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["previous_possessor_id_buffer_id"]));
        }
        if (p_props.has("narreme_type_buffer_id")) {
            narreme_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["narreme_type_buffer_id"]));
        }
        if (p_props.has("spatial_transition_flags_buffer_id")) {
            spatial_transition_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["spatial_transition_flags_buffer_id"]));
        }
        if (p_props.has("location_char_count_buffer_id")) {
            location_char_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["location_char_count_buffer_id"]));
        }
        if (p_props.has("location_prop_count_buffer_id")) {
            location_prop_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["location_prop_count_buffer_id"]));
        }
        if (p_props.has("transfer_command_ring_buffer_id")) {
            transfer_command_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transfer_command_ring_buffer_id"]));
        }
        if (p_props.has("ring_cursor_buffer_id")) {
            ring_cursor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ring_cursor_buffer_id"]));
        }
        if (p_props.has("default_transfer_op")) {
            default_transfer_op = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["default_transfer_op"]), 0, 5));
        }
        if (p_props.has("sync_possessed_prop_locations")) {
            sync_possessed_prop_locations = static_cast<bool>(p_props["sync_possessed_prop_locations"]);
        }
        if (p_props.has("update_location_occupancy_counts")) {
            update_location_occupancy_counts = static_cast<bool>(p_props["update_location_occupancy_counts"]);
        }
        if (p_props.has("clear_pending_after_apply")) {
            clear_pending_after_apply = static_cast<bool>(p_props["clear_pending_after_apply"]);
        }
        if (p_props.has("wake_affected_narremes")) {
            wake_affected_narremes = static_cast<bool>(p_props["wake_affected_narremes"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, int32_t>) {
            const GrantPartPOD* loc_part   = (location_id_buffer_id != INVALID_ID)              ? p_context.get_grant_part(location_id_buffer_id)              : nullptr;
            const GrantPartPOD* pos_part   = (possessor_id_buffer_id != INVALID_ID)             ? p_context.get_grant_part(possessor_id_buffer_id)             : nullptr;
            const GrantPartPOD* ploc_part  = (pending_location_id_buffer_id != INVALID_ID)      ? p_context.get_grant_part(pending_location_id_buffer_id)      : nullptr;
            const GrantPartPOD* ppos_part  = (pending_possessor_id_buffer_id != INVALID_ID)     ? p_context.get_grant_part(pending_possessor_id_buffer_id)     : nullptr;
            const GrantPartPOD* pvloc_part = (previous_location_id_buffer_id != INVALID_ID)     ? p_context.get_grant_part(previous_location_id_buffer_id)     : nullptr;
            const GrantPartPOD* pvpos_part = (previous_possessor_id_buffer_id != INVALID_ID)    ? p_context.get_grant_part(previous_possessor_id_buffer_id)    : nullptr;
            const GrantPartPOD* ntyp_part  = (narreme_type_buffer_id != INVALID_ID)             ? p_context.get_grant_part(narreme_type_buffer_id)             : nullptr;
            const GrantPartPOD* trn_part   = (spatial_transition_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(spatial_transition_flags_buffer_id) : nullptr;
            const GrantPartPOD* lcc_part   = (location_char_count_buffer_id != INVALID_ID)      ? p_context.get_grant_part(location_char_count_buffer_id)      : nullptr;
            const GrantPartPOD* lpc_part   = (location_prop_count_buffer_id != INVALID_ID)      ? p_context.get_grant_part(location_prop_count_buffer_id)      : nullptr;
            const GrantPartPOD* rng_part   = (transfer_command_ring_buffer_id != INVALID_ID)    ? p_context.get_grant_part(transfer_command_ring_buffer_id)    : nullptr;
            const GrantPartPOD* rcur_part  = (ring_cursor_buffer_id != INVALID_ID)              ? p_context.get_grant_part(ring_cursor_buffer_id)              : nullptr;

            const GrantPartPOD* primary_part = loc_part ? loc_part : pos_part;
            if (!primary_part || !primary_part->raw_base_ptr) return;

            uint32_t* __restrict loc_ids        = (loc_part && loc_part->raw_base_ptr)     ? reinterpret_cast<uint32_t*>(loc_part->raw_base_ptr)     : nullptr;
            uint32_t* __restrict pos_ids        = (pos_part && pos_part->raw_base_ptr)     ? reinterpret_cast<uint32_t*>(pos_part->raw_base_ptr)     : nullptr;
            uint32_t* __restrict pend_locs      = (ploc_part && ploc_part->raw_base_ptr)   ? reinterpret_cast<uint32_t*>(ploc_part->raw_base_ptr)    : nullptr;
            uint32_t* __restrict pend_poss      = (ppos_part && ppos_part->raw_base_ptr)   ? reinterpret_cast<uint32_t*>(ppos_part->raw_base_ptr)    : nullptr;
            uint32_t* __restrict prev_locs      = (pvloc_part && pvloc_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(pvloc_part->raw_base_ptr)   : nullptr;
            uint32_t* __restrict prev_poss      = (pvpos_part && pvpos_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(pvpos_part->raw_base_ptr)   : nullptr;
            int32_t* __restrict loc_char_counts = (lcc_part && lcc_part->raw_base_ptr)     ? reinterpret_cast<int32_t*>(lcc_part->raw_base_ptr)      : nullptr;
            int32_t* __restrict loc_prop_counts = (lpc_part && lpc_part->raw_base_ptr)     ? reinterpret_cast<int32_t*>(lpc_part->raw_base_ptr)      : nullptr;

            const uint8_t* __restrict ntyp_raw  = (ntyp_part && ntyp_part->raw_base_ptr)   ? ntyp_part->raw_base_ptr : nullptr;
            const uint32_t ntyp_stride          = ntyp_part ? ntyp_part->element_stride : 0;
            const int64_t ntyp_cap              = (ntyp_raw && ntyp_stride > 0) ? static_cast<int64_t>(ntyp_part->capacity_bytes / ntyp_stride) : 0;

            uint8_t* __restrict trn_raw         = (trn_part && trn_part->raw_base_ptr)     ? trn_part->raw_base_ptr : nullptr;
            const uint32_t trn_stride           = trn_part ? trn_part->element_stride : 0;
            const int64_t trn_cap               = (trn_raw && trn_stride > 0) ? static_cast<int64_t>(trn_part->capacity_bytes / trn_stride) : 0;

            const int64_t loc_cap   = loc_ids         ? static_cast<int64_t>(loc_part->capacity_bytes / sizeof(uint32_t))   : 0;
            const int64_t pos_cap   = pos_ids         ? static_cast<int64_t>(pos_part->capacity_bytes / sizeof(uint32_t))   : 0;
            const int64_t ploc_cap  = pend_locs       ? static_cast<int64_t>(ploc_part->capacity_bytes / sizeof(uint32_t))  : 0;
            const int64_t ppos_cap  = pend_poss       ? static_cast<int64_t>(ppos_part->capacity_bytes / sizeof(uint32_t))  : 0;
            const int64_t pvloc_cap = prev_locs       ? static_cast<int64_t>(pvloc_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t pvpos_cap = prev_poss       ? static_cast<int64_t>(pvpos_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t lcc_cap   = loc_char_counts ? static_cast<int64_t>(lcc_part->capacity_bytes / sizeof(int32_t))    : 0;
            const int64_t lpc_cap   = loc_prop_counts ? static_cast<int64_t>(lpc_part->capacity_bytes / sizeof(int32_t))    : 0;

            auto read_type = [&](int64_t idx, uint8_t fallback) noexcept -> uint8_t {
                if (!ntyp_raw || idx < 0 || idx >= ntyp_cap) return fallback;
                if (ntyp_stride == sizeof(int32_t)) {
                    return static_cast<uint8_t>(reinterpret_cast<const int32_t*>(ntyp_raw)[idx]);
                }
                return ntyp_raw[idx];
            };

            auto add_transition_flag = [&](int64_t idx, uint8_t fbits) noexcept {
                if (!trn_raw || idx < 0 || idx >= trn_cap) return;
                if (trn_stride == sizeof(uint32_t)) {
                    reinterpret_cast<uint32_t*>(trn_raw)[idx] |= static_cast<uint32_t>(fbits);
                } else {
                    trn_raw[idx] |= fbits;
                }
            };

            auto adjust_occupancy = [&](uint32_t location_id, uint8_t ntype, int32_t delta) noexcept {
                if (!update_location_occupancy_counts || location_id == INVALID_ID) return;
                const int64_t lidx = static_cast<int64_t>(location_id);
                if (ntype == NARREME_TYPE_CHARACTER && loc_char_counts && lidx >= 0 && lidx < lcc_cap) {
                    loc_char_counts[lidx] = std::max<int32_t>(0, loc_char_counts[lidx] + delta);
                } else if (ntype == NARREME_TYPE_PROP && loc_prop_counts && lidx >= 0 && lidx < lpc_cap) {
                    loc_prop_counts[lidx] = std::max<int32_t>(0, loc_prop_counts[lidx] + delta);
                }
            };

            auto apply_transfer = [&](int64_t idx, uint32_t target_id, uint8_t op, uint8_t fallback_type) noexcept {
                const uint8_t ntype = read_type(idx, fallback_type);
                bool changed = false;

                switch (op) {
                    case SPATIAL_OP_ENTER_LOCATION: {
                        if (!loc_ids || idx < 0 || idx >= loc_cap) break;
                        const uint32_t old_loc = loc_ids[idx];
                        if (old_loc == target_id) break;
                        if (prev_locs && idx < pvloc_cap) prev_locs[idx] = old_loc;

                        uint8_t flags = SPATIAL_TRANSITION_NONE;
                        if (old_loc != INVALID_ID) {
                            adjust_occupancy(old_loc, ntype, -1);
                            flags |= SPATIAL_TRANSITION_LOCATION_EXITED;
                        }
                        loc_ids[idx] = target_id;
                        if (target_id != INVALID_ID) {
                            adjust_occupancy(target_id, ntype, +1);
                            flags |= SPATIAL_TRANSITION_LOCATION_ENTERED;
                        }
                        add_transition_flag(idx, flags);
                        changed = true;
                        break;
                    }
                    case SPATIAL_OP_LEAVE_LOCATION: {
                        if (!loc_ids || idx < 0 || idx >= loc_cap) break;
                        const uint32_t old_loc = loc_ids[idx];
                        if (old_loc == INVALID_ID) break;
                        if (prev_locs && idx < pvloc_cap) prev_locs[idx] = old_loc;
                        adjust_occupancy(old_loc, ntype, -1);
                        loc_ids[idx] = INVALID_ID;
                        add_transition_flag(idx, SPATIAL_TRANSITION_LOCATION_EXITED);
                        changed = true;
                        break;
                    }
                    case SPATIAL_OP_PROP_TAKEN_BY:
                    case SPATIAL_OP_PROP_FOUND_BY: {
                        if (!pos_ids || idx < 0 || idx >= pos_cap || target_id == INVALID_ID) break;
                        const uint32_t old_pos = pos_ids[idx];
                        if (old_pos == target_id) break;
                        if (prev_poss && idx < pvpos_cap) prev_poss[idx] = old_pos;
                        pos_ids[idx] = target_id;
                        // Mirrors Prop::taken_by(): if !possessor.is_valid() -> found_by()
                        const uint8_t flag = (old_pos == INVALID_ID || op == SPATIAL_OP_PROP_FOUND_BY)
                            ? SPATIAL_TRANSITION_PROP_FOUND
                            : SPATIAL_TRANSITION_PROP_TAKEN;
                        add_transition_flag(idx, flag);

                        if (sync_possessed_prop_locations && loc_ids && idx < loc_cap && static_cast<int64_t>(target_id) < loc_cap) {
                            const uint32_t char_loc = loc_ids[target_id];
                            if (loc_ids[idx] != char_loc) {
                                adjust_occupancy(loc_ids[idx], NARREME_TYPE_PROP, -1);
                                loc_ids[idx] = char_loc;
                                adjust_occupancy(char_loc, NARREME_TYPE_PROP, +1);
                            }
                        }
                        changed = true;
                        break;
                    }
                    case SPATIAL_OP_PROP_RELEASED: {
                        if (!pos_ids || idx < 0 || idx >= pos_cap) break;
                        const uint32_t old_pos = pos_ids[idx];
                        if (old_pos == INVALID_ID) break;
                        if (target_id != INVALID_ID && old_pos != target_id) break;
                        if (prev_poss && idx < pvpos_cap) prev_poss[idx] = old_pos;
                        pos_ids[idx] = INVALID_ID;
                        add_transition_flag(idx, SPATIAL_TRANSITION_PROP_RELEASED);
                        changed = true;
                        break;
                    }
                    case SPATIAL_OP_PROP_LOST: {
                        if (pos_ids && idx >= 0 && idx < pos_cap && pos_ids[idx] != INVALID_ID) {
                            if (prev_poss && idx < pvpos_cap) prev_poss[idx] = pos_ids[idx];
                            pos_ids[idx] = INVALID_ID;
                            add_transition_flag(idx, SPATIAL_TRANSITION_PROP_RELEASED);
                        }
                        if (loc_ids && idx >= 0 && idx < loc_cap && loc_ids[idx] != INVALID_ID) {
                            if (prev_locs && idx < pvloc_cap) prev_locs[idx] = loc_ids[idx];
                            adjust_occupancy(loc_ids[idx], NARREME_TYPE_PROP, -1);
                            loc_ids[idx] = INVALID_ID;
                        }
                        add_transition_flag(idx, SPATIAL_TRANSITION_PROP_LOST);
                        changed = true;
                        break;
                    }
                    default:
                        break;
                }

                if (changed && wake_affected_narremes) {
                    p_context.queue_selection_command(primary_part->buffer_id, idx);
                }
            };

            // --- Phase 1: Drain NarremeSpatialTransferCommandPOD Ring Buffer (if bound) ---
            if (rng_part && rng_part->raw_base_ptr) {
                const NarremeSpatialTransferCommandPOD* __restrict cmds =
                    reinterpret_cast<const NarremeSpatialTransferCommandPOD*>(rng_part->raw_base_ptr);
                const uint32_t ring_cap = static_cast<uint32_t>(rng_part->capacity_bytes / sizeof(NarremeSpatialTransferCommandPOD));
                if (ring_cap > 0u) {
                    uint32_t* __restrict cursors = (rcur_part && rcur_part->raw_base_ptr && rcur_part->capacity_bytes >= 2 * sizeof(uint32_t))
                        ? reinterpret_cast<uint32_t*>(rcur_part->raw_base_ptr)
                        : nullptr;
                    if (cursors) {
                        uint32_t r_idx = cursors[0] % ring_cap;
                        const uint32_t w_idx = cursors[1] % ring_cap;
                        while (r_idx != w_idx) {
                            const NarremeSpatialTransferCommandPOD& cmd = cmds[r_idx];
                            if (cmd.narreme_id != INVALID_ID) {
                                apply_transfer(static_cast<int64_t>(cmd.narreme_id), cmd.target_id, cmd.transfer_op, cmd.narreme_type);
                            }
                            r_idx = (r_idx + 1u) % ring_cap;
                        }
                        cursors[0] = r_idx;
                    } else {
                        const uint32_t cnt = static_cast<uint32_t>(std::clamp<int64_t>(
                            rng_part->selection.element_count, 0, static_cast<int64_t>(ring_cap)
                        ));
                        for (uint32_t i = 0u; i < cnt; ++i) {
                            const NarremeSpatialTransferCommandPOD& cmd = cmds[i];
                            if (cmd.narreme_id != INVALID_ID) {
                                apply_transfer(static_cast<int64_t>(cmd.narreme_id), cmd.target_id, cmd.transfer_op, cmd.narreme_type);
                            }
                        }
                        const_cast<GrantPartPOD*>(rng_part)->selection.element_count = 0;
                    }
                }
            }

            // --- Phase 2: Process Staged Pending Columns & Co-Locate Possessed Props ---
            const int64_t narreme_cap = std::max<int64_t>(loc_cap, pos_cap);
            if (narreme_cap > 0 && (pend_locs || pend_poss || sync_possessed_prop_locations)) {
                auto eval_narreme = [&](int64_t idx) noexcept {
                    if (pend_locs && idx < ploc_cap && pend_locs[idx] != INVALID_ID) {
                        apply_transfer(idx, pend_locs[idx], default_transfer_op, NARREME_TYPE_CHARACTER);
                        if (clear_pending_after_apply) pend_locs[idx] = INVALID_ID;
                    }
                    if (pend_poss && idx < ppos_cap && pend_poss[idx] != INVALID_ID) {
                        apply_transfer(idx, pend_poss[idx], SPATIAL_OP_PROP_TAKEN_BY, NARREME_TYPE_PROP);
                        if (clear_pending_after_apply) pend_poss[idx] = INVALID_ID;
                    }
                    if (sync_possessed_prop_locations && pos_ids && loc_ids && idx < pos_cap && idx < loc_cap) {
                        const uint32_t possessor = pos_ids[idx];
                        if (possessor != INVALID_ID && static_cast<int64_t>(possessor) < loc_cap) {
                            const uint32_t owner_loc = loc_ids[possessor];
                            if (loc_ids[idx] != owner_loc) {
                                apply_transfer(idx, owner_loc, SPATIAL_OP_ENTER_LOCATION, NARREME_TYPE_PROP);
                            }
                        }
                    }
                };
                _iterate_selection(primary_part->selection, narreme_cap, eval_narreme);
            }
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

static_assert(sizeof(NarremeLocationTransferTransformLogic<>) == 64, "NarremeLocationTransferTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core