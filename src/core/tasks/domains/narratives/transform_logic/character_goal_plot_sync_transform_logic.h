#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "narreme_symbolic_similarity_transform_logic.h"
#include "plot_status_evaluate_transform_logic.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_CHARACTER_GOAL_EVENT_POD_DEFINED
#define IDEAM_NARRATIVES_CHARACTER_GOAL_EVENT_POD_DEFINED

/**
 * Character goal outcome flags written to goal_transition_flags_buffer_id and CharacterGoalEventPOD.
 */
enum CharacterGoalOutcomeFlags : uint8_t {
    GOAL_OUTCOME_NONE        = 0u,
    GOAL_OUTCOME_IN_PROGRESS = 1u << 0, // Active goal is PLOT_NOT_BEGUN or PLOT_IN_PROGRESS
    GOAL_OUTCOME_ACHIEVED    = 1u << 1, // Emits Character "goal_achieved" signal (Character::goal_success)
    GOAL_OUTCOME_FAILED      = 1u << 2, // Emits Character "goal_failed" signal (Character::goal_fail)
    GOAL_OUTCOME_REMOVED     = 1u << 3  // Emits Character "goal_removed" signal
};

/**
 * CharacterGoalEventPOD (16-Byte Aligned)
 * Reactive event record queued when a Character's goal Plot completes or fails.
 */
struct alignas(16) CharacterGoalEventPOD {
    uint32_t character_id  = INVALID_ID;              // 4B: Character Narreme ID
    uint32_t plot_id       = INVALID_ID;              // 4B: Goal Plot Narreme ID
    int8_t plot_status     = PLOT_STATUS_NOT_BEGUN;   // 1B: Resolved PlotStatusDOD
    uint8_t outcome_flags  = GOAL_OUTCOME_NONE;       // 1B: CharacterGoalOutcomeFlags
    uint16_t goal_slot_idx = 0u;                      // 2B: Local goal slot index on Character
    uint32_t reserved      = 0u;                      // 4B -> Total = 16B
};

static_assert(sizeof(CharacterGoalEventPOD) == 16, "CharacterGoalEventPOD must be 16 bytes!");

#endif // IDEAM_NARRATIVES_CHARACTER_GOAL_EVENT_POD_DEFINED

/**
 * CharacterGoalPlotSyncTransformLogic<T> (N9)
 * Updates Character goal statuses from IncidentAndPlotStateBuffer::plot_status
 * (mirroring Character::check_goal_status, check_goal_status_at, goal_success, and goal_fail)
 * over BridgeView slices or 64-bit goal bitmasks, and queues reactive CharacterGoalEventPOD records.
 */
template <typename T = int32_t>
struct alignas(64) CharacterGoalPlotSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = BridgeView<T, T, 1, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    =
        BufferLayoutType::FLAT | BufferLayoutType::SOA |
        BufferLayoutType::AOS  | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types              = DataType::INT32 | DataType::BYTE;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Character Goal Plot Sync";
    static constexpr std::string_view ui_category  = "Characters";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t character_goal_status_buffer_id = INVALID_ID; // 4B: Primary output Character goal status (int8_t/int32_t)
    uint32_t primary_goal_plot_id_buffer_id  = INVALID_ID; // 4B: Optional 1:1 primary goal Plot ID per Character (uint32_t)
    uint32_t character_goal_mask_buffer_id   = INVALID_ID; // 4B: Optional uint64_t bitmask of Plot IDs [0..63] per Character
    uint32_t goal_slice_offset_buffer_id     = INVALID_ID; // 4B: Optional CSR goal slice start offset per Character (uint32_t)
    uint32_t goal_slice_count_buffer_id      = INVALID_ID; // 4B: Optional CSR goal slice count per Character (uint16_t/uint32_t)
    uint32_t goal_plot_id_buffer_id          = INVALID_ID; // 4B: Optional CSR contiguous array of goal Plot IDs (uint32_t)
    uint32_t plot_status_buffer_id           = INVALID_ID; // 4B: Bridged IncidentAndPlotStateBuffer::plot_status (int8_t/int32_t)
    uint32_t goal_transition_flags_buffer_id = INVALID_ID; // 4B: Optional uint8_t CharacterGoalOutcomeFlags per Character
    uint32_t goal_event_ring_buffer_id       = INVALID_ID; // 4B: Optional CharacterGoalEventPOD ring buffer
    uint32_t ring_cursor_buffer_id           = INVALID_ID; // 4B: Optional uint32_t[2] [read_idx, write_idx] ring cursor buffer
    uint32_t narreme_type_buffer_id          = INVALID_ID; // 4B: Optional NarremeIdentityAndTraitBuffer::narreme_type (uint8_t)

    bool auto_clear_resolved_goals           = false;      // 1B: Clear completed/failed Plot bits from character_goal_mask
    bool emit_goal_events_to_ring            = true;       // 1B: Push CharacterGoalEventPOD entries when goals complete/fail
    bool filter_character_type               = true;       // 1B: Only evaluate rows with narreme_type == NARREME_TYPE_CHARACTER
    bool wake_characters_on_goal_change      = true;       // 1B: Queue affected character_id into TaskContextPOD::wave_commands
    uint8_t reserved_padding[16]             = {0};        // 16B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary cgst_prop;
        cgst_prop["name"] = "character_goal_status_buffer_id";
        cgst_prop["type"] = godot::Variant::INT;
        cgst_prop["hint_string"] = "buffer_option";
        props.push_back(cgst_prop);

        godot::Dictionary pgp_prop;
        pgp_prop["name"] = "primary_goal_plot_id_buffer_id";
        pgp_prop["type"] = godot::Variant::INT;
        pgp_prop["hint_string"] = "buffer_option";
        props.push_back(pgp_prop);

        godot::Dictionary gmsk_prop;
        gmsk_prop["name"] = "character_goal_mask_buffer_id";
        gmsk_prop["type"] = godot::Variant::INT;
        gmsk_prop["hint_string"] = "buffer_option";
        props.push_back(gmsk_prop);

        godot::Dictionary goff_prop;
        goff_prop["name"] = "goal_slice_offset_buffer_id";
        goff_prop["type"] = godot::Variant::INT;
        goff_prop["hint_string"] = "buffer_option";
        props.push_back(goff_prop);

        godot::Dictionary gcnt_prop;
        gcnt_prop["name"] = "goal_slice_count_buffer_id";
        gcnt_prop["type"] = godot::Variant::INT;
        gcnt_prop["hint_string"] = "buffer_option";
        props.push_back(gcnt_prop);

        godot::Dictionary gpid_prop;
        gpid_prop["name"] = "goal_plot_id_buffer_id";
        gpid_prop["type"] = godot::Variant::INT;
        gpid_prop["hint_string"] = "buffer_option";
        props.push_back(gpid_prop);

        godot::Dictionary pst_prop;
        pst_prop["name"] = "plot_status_buffer_id";
        pst_prop["type"] = godot::Variant::INT;
        pst_prop["hint_string"] = "buffer_option";
        props.push_back(pst_prop);

        godot::Dictionary gtrn_prop;
        gtrn_prop["name"] = "goal_transition_flags_buffer_id";
        gtrn_prop["type"] = godot::Variant::INT;
        gtrn_prop["hint_string"] = "buffer_option";
        props.push_back(gtrn_prop);

        godot::Dictionary grng_prop;
        grng_prop["name"] = "goal_event_ring_buffer_id";
        grng_prop["type"] = godot::Variant::INT;
        grng_prop["hint_string"] = "buffer_option";
        props.push_back(grng_prop);

        godot::Dictionary rcur_prop;
        rcur_prop["name"] = "ring_cursor_buffer_id";
        rcur_prop["type"] = godot::Variant::INT;
        rcur_prop["hint_string"] = "buffer_option";
        props.push_back(rcur_prop);

        godot::Dictionary ntyp_prop;
        ntyp_prop["name"] = "narreme_type_buffer_id";
        ntyp_prop["type"] = godot::Variant::INT;
        ntyp_prop["hint_string"] = "buffer_option";
        props.push_back(ntyp_prop);

        godot::Dictionary clr_prop;
        clr_prop["name"] = "auto_clear_resolved_goals";
        clr_prop["type"] = godot::Variant::BOOL;
        clr_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clr_prop);

        godot::Dictionary emt_prop;
        emt_prop["name"] = "emit_goal_events_to_ring";
        emt_prop["type"] = godot::Variant::BOOL;
        emt_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(emt_prop);

        godot::Dictionary flt_prop;
        flt_prop["name"] = "filter_character_type";
        flt_prop["type"] = godot::Variant::BOOL;
        flt_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(flt_prop);

        godot::Dictionary wak_prop;
        wak_prop["name"] = "wake_characters_on_goal_change";
        wak_prop["type"] = godot::Variant::BOOL;
        wak_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wak_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (character_goal_status_buffer_id != INVALID_ID) return character_goal_status_buffer_id;
        if (character_goal_mask_buffer_id != INVALID_ID)   return character_goal_mask_buffer_id;
        return primary_goal_plot_id_buffer_id;
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        (void)p_part;
        if constexpr (requires { p_view.bind_secondary(p_part); }) {
            const uint32_t sec_id = (plot_status_buffer_id != INVALID_ID)
                ? plot_status_buffer_id
                : get_target_buffer_id();
            if (sec_id != INVALID_ID) {
                p_view.bind_secondary(p_context.get_grant_part(sec_id));
            }
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("character_goal_status_buffer_id")) {
            character_goal_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["character_goal_status_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && character_goal_status_buffer_id == INVALID_ID) {
            character_goal_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("primary_goal_plot_id_buffer_id")) {
            primary_goal_plot_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["primary_goal_plot_id_buffer_id"]));
        }
        if (p_props.has("character_goal_mask_buffer_id")) {
            character_goal_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["character_goal_mask_buffer_id"]));
        }
        if (p_props.has("goal_slice_offset_buffer_id")) {
            goal_slice_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["goal_slice_offset_buffer_id"]));
        }
        if (p_props.has("goal_slice_count_buffer_id")) {
            goal_slice_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["goal_slice_count_buffer_id"]));
        }
        if (p_props.has("goal_plot_id_buffer_id")) {
            goal_plot_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["goal_plot_id_buffer_id"]));
        }
        if (p_props.has("plot_status_buffer_id")) {
            plot_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["plot_status_buffer_id"]));
        }
        if (p_props.has("goal_transition_flags_buffer_id")) {
            goal_transition_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["goal_transition_flags_buffer_id"]));
        }
        if (p_props.has("goal_event_ring_buffer_id")) {
            goal_event_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["goal_event_ring_buffer_id"]));
        }
        if (p_props.has("ring_cursor_buffer_id")) {
            ring_cursor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ring_cursor_buffer_id"]));
        }
        if (p_props.has("narreme_type_buffer_id")) {
            narreme_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["narreme_type_buffer_id"]));
        }
        if (p_props.has("auto_clear_resolved_goals")) {
            auto_clear_resolved_goals = static_cast<bool>(p_props["auto_clear_resolved_goals"]);
        }
        if (p_props.has("emit_goal_events_to_ring")) {
            emit_goal_events_to_ring = static_cast<bool>(p_props["emit_goal_events_to_ring"]);
        }
        if (p_props.has("filter_character_type")) {
            filter_character_type = static_cast<bool>(p_props["filter_character_type"]);
        }
        if (p_props.has("wake_characters_on_goal_change")) {
            wake_characters_on_goal_change = static_cast<bool>(p_props["wake_characters_on_goal_change"]);
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
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint8_t>) {
            const uint32_t tgt_id = get_target_buffer_id();
            const GrantPartPOD* primary_part = (tgt_id != INVALID_ID) ? p_context.get_grant_part(tgt_id) : nullptr;
            const GrantPartPOD* pst_part     = (plot_status_buffer_id != INVALID_ID) ? p_context.get_grant_part(plot_status_buffer_id) : nullptr;
            if (!primary_part || !primary_part->raw_base_ptr || !pst_part || !pst_part->raw_base_ptr) return;

            const GrantPartPOD* cgst_part = (character_goal_status_buffer_id != INVALID_ID) ? p_context.get_grant_part(character_goal_status_buffer_id) : nullptr;
            const GrantPartPOD* pgp_part  = (primary_goal_plot_id_buffer_id != INVALID_ID)  ? p_context.get_grant_part(primary_goal_plot_id_buffer_id)  : nullptr;
            const GrantPartPOD* gmsk_part = (character_goal_mask_buffer_id != INVALID_ID)   ? p_context.get_grant_part(character_goal_mask_buffer_id)   : nullptr;
            const GrantPartPOD* goff_part = (goal_slice_offset_buffer_id != INVALID_ID)     ? p_context.get_grant_part(goal_slice_offset_buffer_id)     : nullptr;
            const GrantPartPOD* gcnt_part = (goal_slice_count_buffer_id != INVALID_ID)      ? p_context.get_grant_part(goal_slice_count_buffer_id)      : nullptr;
            const GrantPartPOD* gpid_part = (goal_plot_id_buffer_id != INVALID_ID)          ? p_context.get_grant_part(goal_plot_id_buffer_id)          : nullptr;
            const GrantPartPOD* gtrn_part = (goal_transition_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(goal_transition_flags_buffer_id) : nullptr;
            const GrantPartPOD* grng_part = (goal_event_ring_buffer_id != INVALID_ID)       ? p_context.get_grant_part(goal_event_ring_buffer_id)       : nullptr;
            const GrantPartPOD* rcur_part = (ring_cursor_buffer_id != INVALID_ID)           ? p_context.get_grant_part(ring_cursor_buffer_id)           : nullptr;
            const GrantPartPOD* ntyp_part = (narreme_type_buffer_id != INVALID_ID)          ? p_context.get_grant_part(narreme_type_buffer_id)          : nullptr;

            uint8_t* __restrict cgst_raw  = (cgst_part && cgst_part->raw_base_ptr) ? cgst_part->raw_base_ptr : nullptr;
            const uint32_t cgst_stride    = cgst_part ? cgst_part->element_stride : 0;
            const int64_t cgst_cap        = (cgst_raw && cgst_stride > 0) ? static_cast<int64_t>(cgst_part->capacity_bytes / cgst_stride) : 0;

            uint32_t* __restrict pri_goals= (pgp_part && pgp_part->raw_base_ptr)   ? reinterpret_cast<uint32_t*>(pgp_part->raw_base_ptr)   : nullptr;
            const int64_t pgp_cap         = pri_goals ? static_cast<int64_t>(pgp_part->capacity_bytes / sizeof(uint32_t)) : 0;

            uint64_t* __restrict goal_masks=(gmsk_part && gmsk_part->raw_base_ptr) ? reinterpret_cast<uint64_t*>(gmsk_part->raw_base_ptr)  : nullptr;
            const int64_t gmsk_cap        = goal_masks ? static_cast<int64_t>(gmsk_part->capacity_bytes / sizeof(uint64_t)) : 0;

            const uint32_t* __restrict goal_offsets = (goff_part && goff_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(goff_part->raw_base_ptr) : nullptr;
            const int64_t goff_cap        = goal_offsets ? static_cast<int64_t>(goff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict gcnt_raw = (gcnt_part && gcnt_part->raw_base_ptr) ? gcnt_part->raw_base_ptr : nullptr;
            const uint32_t gcnt_stride    = gcnt_part ? gcnt_part->element_stride : 0;
            const int64_t gcnt_cap        = (gcnt_raw && gcnt_stride > 0) ? static_cast<int64_t>(gcnt_part->capacity_bytes / gcnt_stride) : 0;

            const uint32_t* __restrict goal_plot_ids = (gpid_part && gpid_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(gpid_part->raw_base_ptr) : nullptr;
            const int64_t gpid_cap        = goal_plot_ids ? static_cast<int64_t>(gpid_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict pst_raw = pst_part->raw_base_ptr;
            const uint32_t pst_stride     = (pst_part->element_stride > 0) ? pst_part->element_stride : sizeof(int8_t);
            const int64_t pst_cap         = static_cast<int64_t>(pst_part->capacity_bytes / pst_stride);

            uint8_t* __restrict gtrn_raw  = (gtrn_part && gtrn_part->raw_base_ptr) ? gtrn_part->raw_base_ptr : nullptr;
            const uint32_t gtrn_stride    = gtrn_part ? gtrn_part->element_stride : 0;
            const int64_t gtrn_cap        = (gtrn_raw && gtrn_stride > 0) ? static_cast<int64_t>(gtrn_part->capacity_bytes / gtrn_stride) : 0;

            const uint8_t* __restrict ntyp_raw = (ntyp_part && ntyp_part->raw_base_ptr) ? ntyp_part->raw_base_ptr : nullptr;
            const uint32_t ntyp_stride    = ntyp_part ? ntyp_part->element_stride : 0;
            const int64_t ntyp_cap        = (ntyp_raw && ntyp_stride > 0) ? static_cast<int64_t>(ntyp_part->capacity_bytes / ntyp_stride) : 0;

            CharacterGoalEventPOD* __restrict ring_events = (grng_part && grng_part->raw_base_ptr)
                ? reinterpret_cast<CharacterGoalEventPOD*>(grng_part->raw_base_ptr)
                : nullptr;
            const uint32_t ring_cap       = ring_events ? static_cast<uint32_t>(grng_part->capacity_bytes / sizeof(CharacterGoalEventPOD)) : 0u;
            uint32_t* __restrict ring_cursors = (rcur_part && rcur_part->raw_base_ptr && rcur_part->capacity_bytes >= 2 * sizeof(uint32_t))
                ? reinterpret_cast<uint32_t*>(rcur_part->raw_base_ptr)
                : nullptr;
            uint32_t local_write_idx      = ring_cursors ? ring_cursors[1] : 0u;
            const uint32_t local_read_idx = ring_cursors ? ring_cursors[0] : 0u;

            const int64_t char_cap = static_cast<int64_t>(
                primary_part->capacity_bytes / (primary_part->element_stride > 0 ? primary_part->element_stride : sizeof(T))
            );
            if (char_cap <= 0 || pst_cap <= 0) return;

            auto read_plot_status = [&](uint32_t pid) noexcept -> int8_t {
                if (pid == INVALID_ID || static_cast<int64_t>(pid) >= pst_cap) return PLOT_STATUS_CANNOT_COMPLETE;
                if (pst_stride == sizeof(int32_t)) {
                    return static_cast<int8_t>(reinterpret_cast<const int32_t*>(pst_raw)[pid]);
                }
                return reinterpret_cast<const int8_t*>(pst_raw)[pid];
            };

            auto push_goal_event = [&](uint32_t char_id, uint32_t plot_id, int8_t p_status, uint8_t outcome, uint16_t slot_idx) noexcept {
                if (!emit_goal_events_to_ring || !ring_events || ring_cap == 0u) return;
                CharacterGoalEventPOD evt{char_id, plot_id, p_status, outcome, slot_idx, 0u};
                if (ring_cursors) {
                    const uint32_t next_w = (local_write_idx + 1u) % ring_cap;
                    if (next_w == local_read_idx) return;
                    ring_events[local_write_idx % ring_cap] = evt;
                    local_write_idx = next_w;
                } else if (local_write_idx < ring_cap) {
                    ring_events[local_write_idx++] = evt;
                }
            };

            auto eval_character = [&](int64_t cidx) noexcept {
                if (filter_character_type && ntyp_raw && cidx < ntyp_cap) {
                    const uint8_t ntype = (ntyp_stride == sizeof(int32_t))
                        ? static_cast<uint8_t>(reinterpret_cast<const int32_t*>(ntyp_raw)[cidx])
                        : ntyp_raw[cidx];
                    if (ntype != NARREME_TYPE_CHARACTER) return;
                }

                uint8_t agg_outcome   = GOAL_OUTCOME_NONE;
                int8_t latest_plot_st = PLOT_STATUS_NOT_BEGUN;
                bool any_resolved     = false;

                auto inspect_goal = [&](uint32_t plot_id, uint16_t slot_idx) noexcept -> bool {
                    if (plot_id == INVALID_ID || static_cast<int64_t>(plot_id) >= pst_cap) return false;
                    const int8_t pst = read_plot_status(plot_id);
                    latest_plot_st   = pst;

                    if (pst == PLOT_STATUS_COMPLETE) {
                        agg_outcome |= GOAL_OUTCOME_ACHIEVED;
                        push_goal_event(static_cast<uint32_t>(cidx), plot_id, pst, GOAL_OUTCOME_ACHIEVED, slot_idx);
                        any_resolved = true;
                        return true;
                    }
                    if (pst == PLOT_STATUS_CANNOT_COMPLETE || pst == PLOT_STATUS_FAILED) {
                        agg_outcome |= GOAL_OUTCOME_FAILED;
                        push_goal_event(static_cast<uint32_t>(cidx), plot_id, pst, GOAL_OUTCOME_FAILED, slot_idx);
                        any_resolved = true;
                        return true;
                    }
                    agg_outcome |= GOAL_OUTCOME_IN_PROGRESS;
                    return false;
                };

                // 1. Evaluate CSR goal slice if bound
                if (goal_offsets && gcnt_raw && goal_plot_ids && cidx < goff_cap && cidx < gcnt_cap) {
                    const int64_t off = static_cast<int64_t>(goal_offsets[cidx]);
                    const uint32_t cnt = (gcnt_stride == sizeof(uint32_t))
                        ? reinterpret_cast<const uint32_t*>(gcnt_raw)[cidx]
                        : static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(gcnt_raw)[cidx]);
                    const int64_t end = std::min<int64_t>(off + static_cast<int64_t>(cnt), gpid_cap);
                    for (int64_t g = off; g < end; ++g) {
                        inspect_goal(goal_plot_ids[g], static_cast<uint16_t>(g - off));
                    }
                }
                // 2. Evaluate 64-bit goal bitmask if bound
                else if (goal_masks && cidx < gmsk_cap) {
                    uint64_t mask = goal_masks[cidx];
                    uint64_t cleared_bits = 0ULL;
                    uint16_t slot_k = 0u;
                    while (mask != 0ULL) {
                        const int bit = std::countr_zero(mask);
                        if (inspect_goal(static_cast<uint32_t>(bit), slot_k++) && auto_clear_resolved_goals) {
                            cleared_bits |= (1ULL << bit);
                        }
                        mask &= (mask - 1ULL);
                    }
                    if (cleared_bits != 0ULL) {
                        goal_masks[cidx] &= ~cleared_bits;
                    }
                }
                // 3. Evaluate primary 1:1 goal Plot ID if bound
                else if (pri_goals && cidx < pgp_cap) {
                    const uint32_t pid = pri_goals[cidx];
                    if (inspect_goal(pid, 0u) && auto_clear_resolved_goals) {
                        pri_goals[cidx] = INVALID_ID;
                    }
                }

                if (cgst_raw && cidx < cgst_cap) {
                    if (cgst_stride == sizeof(int32_t)) {
                        reinterpret_cast<int32_t*>(cgst_raw)[cidx] = static_cast<int32_t>(latest_plot_st);
                    } else {
                        reinterpret_cast<int8_t*>(cgst_raw)[cidx] = latest_plot_st;
                    }
                }

                if (gtrn_raw && cidx < gtrn_cap && agg_outcome != GOAL_OUTCOME_NONE) {
                    if (gtrn_stride == sizeof(uint32_t)) {
                        reinterpret_cast<uint32_t*>(gtrn_raw)[cidx] |= static_cast<uint32_t>(agg_outcome);
                    } else {
                        gtrn_raw[cidx] |= agg_outcome;
                    }
                }

                if (any_resolved && wake_characters_on_goal_change) {
                    p_context.queue_selection_command(primary_part->buffer_id, cidx);
                }
            };

            _iterate_selection(primary_part->selection, char_cap, eval_character);

            if (ring_cursors) {
                ring_cursors[1] = local_write_idx;
            } else if (grng_part && !ring_cursors && emit_goal_events_to_ring) {
                const_cast<GrantPartPOD*>(grng_part)->selection.element_count = static_cast<int64_t>(local_write_idx);
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

static_assert(sizeof(CharacterGoalPlotSyncTransformLogic<>) == 64, "CharacterGoalPlotSyncTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core