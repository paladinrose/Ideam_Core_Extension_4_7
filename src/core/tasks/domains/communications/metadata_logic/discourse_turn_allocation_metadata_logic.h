#pragma once

/**
 * discourse_turn_allocation_metadata_logic.h
 * `COM1`: 64-byte cache-aligned `MetadataLogic` kernel for `IdeamCommunications`
 * (`src/core/tasks/domains/communications/metadata_logic/`).
 *
 * Evaluates the Sacks-Schegloff-Jefferson (1974) 3-rule turn allocation state
 * machine over `DiscourseTurnStatePOD` and `DiscourseParticipantPOD` buffers:
 *   - Rule 1a (`RULE_1A_CURRENT_SELECTS_NEXT`): Current speaker explicitly
 *     addresses next speaker (`addressed_target_id`).
 *   - Rule 1b (`RULE_1B_SELF_SELECTION`): Highest `excitation_urgency + turn_claim_priority`
 *     listener self-selects at a Transition Relevance Place (TRP).
 *   - Rule 1c (`RULE_1C_CURRENT_CONTINUES`): Current speaker continues with the
 *     next `TurnConstructionUnit` (`LEXICAL`, `PHRASAL`, `CLAUSAL`, `SENTENTIAL`).
 *   - Also stamps turn / participant priority tiers (`0 = Active Speaker`,
 *     `1 = Addressed Next Speaker`, `2 = Floor Claimant`, `3 = Passive Listener`)
 *     into `r_selection.lod_levels`.
 */

#include "../../../../communications/noisy_channel_transceiver.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) DiscourseTurnAllocationMetadataLogic {
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

    static constexpr std::string_view display_name = "COM1: Discourse Turn Construction Unit (TCU) Allocation";
    static constexpr std::string_view ui_category  = "Communications/Discourse";

    // --- 64-Byte Cache-Line Member Layout ---
    uint32_t target_buffer_id                = INVALID_ID; ///< Offset 0  (4B): Target selection buffer
    uint32_t discourse_turn_state_buffer_id  = INVALID_ID; ///< Offset 4  (4B): `COMM_DISCOURSE_TURN_STATE_POD`
    uint32_t discourse_participant_buffer_id = INVALID_ID; ///< Offset 8  (4B): `COMM_DISCOURSE_PARTICIPANT_POD`
    uint32_t signal_packet_buffer_id         = INVALID_ID; ///< Offset 12 (4B): Optional `COMM_SIGNAL_PACKET_POD`
    uint32_t active_lect_id                  = 0;          ///< Offset 16 (4B): Preferred dialect/lect filter
    float    intonation_trp_pitch_threshold  = 0.18f;      ///< Offset 20 (4B): Pitch delta triggering intonational TRP
    float    interruption_margin_threshold_db= 6.0f;       ///< Offset 24 (4B): dB margin triggering pragmatic interruption
    uint8_t  reserved_padding[36]            = {0};        ///< Offset 28 (36B) -> Total = 64 Bytes

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",                 godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("discourse_turn_state_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("discourse_participant_buffer_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("signal_packet_buffer_id",          godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("active_lect_id",                   godot::Variant::INT,   static_cast<int64_t>(0));
        add_prop("intonation_trp_pitch_threshold",   godot::Variant::FLOAT, 0.18);
        add_prop("interruption_margin_threshold_db", godot::Variant::FLOAT, 6.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                 target_buffer_id                 = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("metadata_buffer_id"))               discourse_turn_state_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["metadata_buffer_id"]));
        if (p_props.has("discourse_turn_state_buffer_id"))   discourse_turn_state_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["discourse_turn_state_buffer_id"]));
        if (p_props.has("discourse_participant_buffer_id"))  discourse_participant_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["discourse_participant_buffer_id"]));
        if (p_props.has("signal_packet_buffer_id"))          signal_packet_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["signal_packet_buffer_id"]));
        if (p_props.has("active_lect_id"))                   active_lect_id                   = static_cast<uint32_t>(static_cast<int64_t>(p_props["active_lect_id"]));
        if (p_props.has("intonation_trp_pitch_threshold"))   intonation_trp_pitch_threshold   = static_cast<float>(static_cast<double>(p_props["intonation_trp_pitch_threshold"]));
        if (p_props.has("interruption_margin_threshold_db")) interruption_margin_threshold_db = static_cast<float>(static_cast<double>(p_props["interruption_margin_threshold_db"]));
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
        size_t turn_cap = 0, part_cap = 0, pkt_cap = 0;
        const uint32_t turn_buf = (discourse_turn_state_buffer_id != INVALID_ID)
            ? discourse_turn_state_buffer_id
            : target_buffer_id;
        auto *turns        = _resolve_ptr<DiscourseTurnStatePOD>(p_ctx, turn_buf, turn_cap);
        auto *participants = _resolve_ptr<DiscourseParticipantPOD>(p_ctx, discourse_participant_buffer_id, part_cap);
        auto *packets      = _resolve_ptr<SignalPacketPOD>(p_ctx, signal_packet_buffer_id, pkt_cap);

        if (turns && turn_cap > 0 && participants && part_cap > 0) {
            std::span<DiscourseParticipantPOD> part_span(participants, part_cap);
            for (size_t t = 0; t < turn_cap; ++t) {
                NoisyChannelTransceiver::evaluate_discourse_turn_allocation(turns[t], part_span);
            }
        }

        // Propagate turn resolution to any active SignalPacketPODs
        if (packets && pkt_cap > 0 && turns && turn_cap > 0) {
            const DiscourseTurnStatePOD &active_turn = turns[0];
            for (size_t i = 0; i < pkt_cap; ++i) {
                if (packets[i].resolution == TurnResolutionType::UNRESOLVED && active_turn.trp_reached != 0u) {
                    packets[i].resolution = active_turn.resolution;
                }
            }
        }

        // Stamp participant priority LOD levels into target selection if available
        if (GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(target_buffer_id))) {
            MemoryBufferSelectionPOD &sel = target_part->selection;
            if (sel.lod_levels != nullptr && participants != nullptr) {
                const size_t n = std::min<size_t>(part_cap, static_cast<size_t>(std::max<int64_t>(0, sel.element_count)));
                const uint32_t next_id = (turns && turn_cap > 0) ? turns[0].next_selected_speaker_id : 0xFFFFFFFFu;
                for (size_t i = 0; i < n; ++i) {
                    if (participants[i].is_speaking != 0u) {
                        sel.lod_levels[i] = 0u;
                    } else if (participants[i].participant_id == next_id) {
                        sel.lod_levels[i] = 1u;
                    } else if (participants[i].wants_floor != 0u) {
                        sel.lod_levels[i] = 2u;
                    } else {
                        sel.lod_levels[i] = 3u;
                    }
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
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

static_assert(sizeof(DiscourseTurnAllocationMetadataLogic<>) == 64, "DiscourseTurnAllocationMetadataLogic must be 64 bytes!");
static_assert(IsMetadataLogic<DiscourseTurnAllocationMetadataLogic<>>, "DiscourseTurnAllocationMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
