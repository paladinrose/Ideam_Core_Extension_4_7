#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include "../transform_logic/entity_time_scale_cascade_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_BOARD_PRESENCE_PARTITION_MODE_DEFINED
#define IDEAM_GAMES_BOARD_PRESENCE_PARTITION_MODE_DEFINED

/**
 * Partition resolution strategies for BoardPresencePartitionMetadataLogic (G18).
 */
enum BoardPartitionResolutionMode : uint8_t {
    BOARD_PARTITION_DIRECT_OR_ANCESTOR = 0, // Uses board_id_buffer_id if valid, otherwise walks parent_entity_id up to ENTITY_ARCHETYPE_BOARD
    BOARD_PARTITION_DIRECT_COLUMN_ONLY = 1, // Strictly reads board_id from board_id_buffer_id (or primary p_view)
    BOARD_PARTITION_ANCESTOR_WALK_ONLY = 2  // Strictly walks parent_entity_id up to the owning ENTITY_ARCHETYPE_BOARD ancestor
};

#endif // IDEAM_GAMES_BOARD_PRESENCE_PARTITION_MODE_DEFINED

/**
 * BoardPresencePartitionMetadataLogic<T> (G18)
 * Partitions GameAgent, GamePiece, and child action/property selections by their owning GameBoard ID
 * (r_selection.partition_ids) so board-scoped LAYER_ENVIRONMENT GameplayStyles run in isolated waves.
 * Supports direct board_id columns, owner_entity_id indirection (for properties/actions), and bounded
 * parent_entity_id tree walks up to the owning ENTITY_ARCHETYPE_BOARD row.
 */
template <typename T = int32_t>
struct alignas(64) BoardPresencePartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Board Presence Partition";
    static constexpr std::string_view ui_category  = "Hierarchy";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id           = INVALID_ID;                                     // 4B: Primary selection buffer to partition
    uint32_t board_id_buffer_id         = INVALID_ID;                                     // 4B: Optional direct board_id (int32_t/uint32_t) column
    uint32_t parent_entity_id_buffer_id = INVALID_ID;                                     // 4B: EntityHierarchyBuffer::parent_entity_id (uint32_t)
    uint32_t archetype_mask_buffer_id   = INVALID_ID;                                     // 4B: EntityHierarchyBuffer::archetype_mask (uint32_t)
    uint32_t active_flags_buffer_id     = INVALID_ID;                                     // 4B: Optional EntityHierarchyBuffer::active_flags (uint32_t)
    uint32_t owner_entity_id_buffer_id  = INVALID_ID;                                     // 4B: Optional owner_entity_id for Property/Action buffers
    uint32_t output_board_id_buffer_id  = INVALID_ID;                                     // 4B: Optional int32_t column to cache resolved board_id
    uint32_t target_archetype_mask      = ENTITY_ARCHETYPE_AGENT | ENTITY_ARCHETYPE_PIECE;// 4B: Archetypes eligible for board partitioning
    int32_t unassigned_partition_id     = -1;                                             // 4B: Fallback partition_id when not on an active GameBoard
    int32_t filter_board_id             = -1;                                             // 4B: Optional board_id filter (-1 = allow all boards)

    uint8_t resolution_mode             = BOARD_PARTITION_DIRECT_OR_ANCESTOR;             // 1B: BoardPartitionResolutionMode
    uint8_t max_hierarchy_depth         = 8u;                                             // 1B: Max parent hops when resolving ancestor GameBoard
    bool require_enabled                = true;                                           // 1B: Require ENTITY_FLAG_ENABLED on entity & owning board
    bool write_hop_depth_to_lod         = false;                                          // 1B: Stamp ancestor hop depth into r_selection.lod_levels
    uint8_t reserved_padding[20]        = {0};                                            // 20B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary brd_prop;
        brd_prop["name"] = "board_id_buffer_id";
        brd_prop["type"] = godot::Variant::INT;
        brd_prop["hint_string"] = "buffer_option";
        props.push_back(brd_prop);

        godot::Dictionary par_prop;
        par_prop["name"] = "parent_entity_id_buffer_id";
        par_prop["type"] = godot::Variant::INT;
        par_prop["hint_string"] = "buffer_option";
        props.push_back(par_prop);

        godot::Dictionary arch_prop;
        arch_prop["name"] = "archetype_mask_buffer_id";
        arch_prop["type"] = godot::Variant::INT;
        arch_prop["hint_string"] = "buffer_option";
        props.push_back(arch_prop);

        godot::Dictionary flg_prop;
        flg_prop["name"] = "active_flags_buffer_id";
        flg_prop["type"] = godot::Variant::INT;
        flg_prop["hint_string"] = "buffer_option";
        props.push_back(flg_prop);

        godot::Dictionary own_prop;
        own_prop["name"] = "owner_entity_id_buffer_id";
        own_prop["type"] = godot::Variant::INT;
        own_prop["hint_string"] = "buffer_option";
        props.push_back(own_prop);

        godot::Dictionary out_prop;
        out_prop["name"] = "output_board_id_buffer_id";
        out_prop["type"] = godot::Variant::INT;
        out_prop["hint_string"] = "buffer_option";
        props.push_back(out_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "resolution_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Direct Or Ancestor Walk:0,Direct Column Only:1,Ancestor Walk Only:2";
        props.push_back(mode_prop);

        godot::Dictionary tarch_prop;
        tarch_prop["name"] = "target_archetype_mask";
        tarch_prop["type"] = godot::Variant::INT;
        tarch_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        tarch_prop["hint_string"] = "Game,Board,Agent,Piece,AgentAction,PieceAction,Interaction";
        props.push_back(tarch_prop);

        godot::Dictionary un_prop;
        un_prop["name"] = "unassigned_partition_id";
        un_prop["type"] = godot::Variant::INT;
        un_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(un_prop);

        godot::Dictionary fbrd_prop;
        fbrd_prop["name"] = "filter_board_id";
        fbrd_prop["type"] = godot::Variant::INT;
        fbrd_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(fbrd_prop);

        godot::Dictionary dep_prop;
        dep_prop["name"] = "max_hierarchy_depth";
        dep_prop["type"] = godot::Variant::INT;
        dep_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        dep_prop["hint_string"] = "1,16,1";
        props.push_back(dep_prop);

        godot::Dictionary en_prop;
        en_prop["name"] = "require_enabled";
        en_prop["type"] = godot::Variant::BOOL;
        en_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(en_prop);

        godot::Dictionary lod_prop;
        lod_prop["name"] = "write_hop_depth_to_lod";
        lod_prop["type"] = godot::Variant::BOOL;
        lod_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(lod_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)           return target_buffer_id;
        if (board_id_buffer_id != INVALID_ID)         return board_id_buffer_id;
        if (parent_entity_id_buffer_id != INVALID_ID) return parent_entity_id_buffer_id;
        return archetype_mask_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("board_id_buffer_id")) {
            board_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["board_id_buffer_id"]));
        }
        if (p_props.has("parent_entity_id_buffer_id")) {
            parent_entity_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_entity_id_buffer_id"]));
        }
        if (p_props.has("archetype_mask_buffer_id")) {
            archetype_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["archetype_mask_buffer_id"]));
        }
        if (p_props.has("active_flags_buffer_id")) {
            active_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["active_flags_buffer_id"]));
        }
        if (p_props.has("owner_entity_id_buffer_id")) {
            owner_entity_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["owner_entity_id_buffer_id"]));
        }
        if (p_props.has("output_board_id_buffer_id")) {
            output_board_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_board_id_buffer_id"]));
        }
        if (p_props.has("target_archetype_mask")) {
            target_archetype_mask = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_archetype_mask"]));
        }
        if (p_props.has("unassigned_partition_id")) {
            unassigned_partition_id = static_cast<int32_t>(static_cast<int64_t>(p_props["unassigned_partition_id"]));
        }
        if (p_props.has("filter_board_id")) {
            filter_board_id = static_cast<int32_t>(static_cast<int64_t>(p_props["filter_board_id"]));
        }
        if (p_props.has("resolution_mode")) {
            resolution_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["resolution_mode"]), 0, 2));
        }
        if (p_props.has("max_hierarchy_depth")) {
            max_hierarchy_depth = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_hierarchy_depth"]), 1, 32));
        }
        if (p_props.has("require_enabled")) {
            require_enabled = static_cast<bool>(p_props["require_enabled"]);
        }
        if (p_props.has("write_hop_depth_to_lod")) {
            write_hop_depth_to_lod = static_cast<bool>(p_props["write_hop_depth_to_lod"]);
        }
    }

    template <typename T_View, typename T_Strategy>
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
        if constexpr (std::is_same_v<T, int32_t>) {
            if (!r_selection.partition_ids || r_selection.element_count == 0) return;

            const GrantPartPOD* brd_part  = (board_id_buffer_id != INVALID_ID)         ? p_context.get_grant_part(board_id_buffer_id)         : nullptr;
            const GrantPartPOD* par_part  = (parent_entity_id_buffer_id != INVALID_ID) ? p_context.get_grant_part(parent_entity_id_buffer_id) : nullptr;
            const GrantPartPOD* arch_part = (archetype_mask_buffer_id != INVALID_ID)   ? p_context.get_grant_part(archetype_mask_buffer_id)   : nullptr;
            const GrantPartPOD* flg_part  = (active_flags_buffer_id != INVALID_ID)     ? p_context.get_grant_part(active_flags_buffer_id)     : nullptr;
            const GrantPartPOD* own_part  = (owner_entity_id_buffer_id != INVALID_ID)  ? p_context.get_grant_part(owner_entity_id_buffer_id)  : nullptr;
            const GrantPartPOD* out_part  = (output_board_id_buffer_id != INVALID_ID)  ? p_context.get_grant_part(output_board_id_buffer_id)  : nullptr;

            const int32_t* __restrict board_ids   = (brd_part && brd_part->raw_base_ptr)   ? reinterpret_cast<const int32_t*>(brd_part->raw_base_ptr)   : nullptr;
            const uint32_t* __restrict parents    = (par_part && par_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(par_part->raw_base_ptr)  : nullptr;
            const uint32_t* __restrict archetypes = (arch_part && arch_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(arch_part->raw_base_ptr) : nullptr;
            const uint32_t* __restrict flags      = (flg_part && flg_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(flg_part->raw_base_ptr)  : nullptr;
            const uint32_t* __restrict owners     = (own_part && own_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(own_part->raw_base_ptr)  : nullptr;
            int32_t* __restrict out_board_ids     = (out_part && out_part->raw_base_ptr)   ? reinterpret_cast<int32_t*>(out_part->raw_base_ptr)         : nullptr;

            const int64_t brd_cap  = board_ids     ? static_cast<int64_t>(brd_part->capacity_bytes / sizeof(int32_t))   : 0;
            const int64_t par_cap  = parents       ? static_cast<int64_t>(par_part->capacity_bytes / sizeof(uint32_t))  : 0;
            const int64_t arch_cap = archetypes    ? static_cast<int64_t>(arch_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t flg_cap  = flags         ? static_cast<int64_t>(flg_part->capacity_bytes / sizeof(uint32_t))  : 0;
            const int64_t own_cap  = owners        ? static_cast<int64_t>(own_part->capacity_bytes / sizeof(uint32_t))  : 0;
            const int64_t out_cap  = out_board_ids ? static_cast<int64_t>(out_part->capacity_bytes / sizeof(int32_t))   : 0;

            auto is_entity_enabled = [&](int64_t eidx) noexcept -> bool {
                if (!require_enabled || !flags || eidx < 0 || eidx >= flg_cap) return true;
                return (flags[eidx] & ENTITY_FLAG_ENABLED) != 0u;
            };

            auto resolve_board_for_row = [&](int64_t row_idx, uint8_t& r_hops) noexcept -> int32_t {
                r_hops = 0u;

                // Resolve owning entity index if operating on a Property / Action buffer
                int64_t entity_idx = row_idx;
                if (owners && row_idx >= 0 && row_idx < own_cap) {
                    const uint32_t owner_id = owners[row_idx];
                    if (owner_id == INVALID_ID) return unassigned_partition_id;
                    entity_idx = static_cast<int64_t>(owner_id);
                }

                if (!is_entity_enabled(entity_idx)) {
                    return unassigned_partition_id;
                }

                // Filter by target_archetype_mask when archetype_mask column is bound
                if (archetypes && entity_idx >= 0 && entity_idx < arch_cap && target_archetype_mask != 0u) {
                    if ((archetypes[entity_idx] & target_archetype_mask) == 0u) {
                        return unassigned_partition_id;
                    }
                }

                int32_t resolved_board = unassigned_partition_id;

                // 1. Direct column lookup (or primary view fallback when no hierarchy columns are bound)
                if (resolution_mode != BOARD_PARTITION_ANCESTOR_WALK_ONLY) {
                    if (board_ids && entity_idx >= 0 && entity_idx < brd_cap) {
                        const int32_t direct_id = board_ids[entity_idx];
                        if (direct_id >= 0 && static_cast<uint32_t>(direct_id) != INVALID_ID) {
                            resolved_board = direct_id;
                        }
                    } else if (!parents && !archetypes) {
                        const int32_t view_val = _read_view(p_view, row_idx);
                        if (view_val >= 0 && static_cast<uint32_t>(view_val) != INVALID_ID) {
                            resolved_board = view_val;
                        }
                    }
                }

                // 2. Topological ancestor walk up EntityHierarchyBuffer to owning ENTITY_ARCHETYPE_BOARD
                if (resolved_board == unassigned_partition_id && resolution_mode != BOARD_PARTITION_DIRECT_COLUMN_ONLY) {
                    if (archetypes && entity_idx >= 0 && entity_idx < arch_cap &&
                        (archetypes[entity_idx] & ENTITY_ARCHETYPE_BOARD) != 0u) {
                        resolved_board = (board_ids && entity_idx < brd_cap && board_ids[entity_idx] >= 0)
                            ? board_ids[entity_idx]
                            : static_cast<int32_t>(entity_idx);
                    } else if (parents && entity_idx >= 0 && entity_idx < par_cap) {
                        uint32_t curr = parents[entity_idx];
                        for (uint8_t hop = 1u; hop <= max_hierarchy_depth && curr != INVALID_ID; ++hop) {
                            const int64_t cidx = static_cast<int64_t>(curr);
                            if (cidx < 0 || cidx >= par_cap) break;

                            if (!is_entity_enabled(cidx)) {
                                break;
                            }

                            const bool is_board = archetypes
                                ? (cidx < arch_cap && ((archetypes[cidx] & ENTITY_ARCHETYPE_BOARD) != 0u))
                                : (parents[cidx] == INVALID_ID || parents[cidx] == curr);

                            if (is_board) {
                                r_hops = hop;
                                resolved_board = (board_ids && cidx < brd_cap && board_ids[cidx] >= 0)
                                    ? board_ids[cidx]
                                    : static_cast<int32_t>(cidx);
                                break;
                            }

                            const uint32_t next_parent = parents[cidx];
                            if (next_parent == curr) break;
                            curr = next_parent;
                        }
                    }
                }

                if (filter_board_id >= 0 && resolved_board != filter_board_id) {
                    return unassigned_partition_id;
                }
                return resolved_board;
            };

            auto stamp_row = [&](int64_t idx) noexcept {
                if (idx < 0 || idx >= r_selection.capacity) return;
                uint8_t hops = 0u;
                const int32_t board_part = resolve_board_for_row(idx, hops);
                r_selection.partition_ids[idx] = static_cast<int64_t>(board_part);
                if (write_hop_depth_to_lod && r_selection.lod_levels) {
                    r_selection.lod_levels[idx] = hops;
                }
                if (out_board_ids && idx < out_cap) {
                    out_board_ids[idx] = board_part;
                }
            };

            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                const uint64_t* bitset = r_selection.data.bitset;
                const int64_t cap      = r_selection.capacity;
                for (int64_t i = 0; i < cap; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        stamp_row(i);
                    }
                }
            } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                const int64_t* indices = r_selection.data.indices;
                const int64_t count    = r_selection.element_count;
                for (int64_t i = 0; i < count; ++i) {
                    stamp_row(indices[i]);
                }
            } else if (r_selection.mode == SelectionMode::RANGE) {
                const int64_t start = std::max<int64_t>(0, r_selection.start_index);
                const int64_t end   = std::min<int64_t>(start + r_selection.element_count, r_selection.capacity);
                for (int64_t i = start; i < end; ++i) {
                    stamp_row(i);
                }
            }
        }
    }

private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _read_view(const T_View& p_view, int64_t idx) const noexcept {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else if constexpr (requires { static_cast<T>(p_view[idx]); }) {
            return static_cast<T>(p_view[idx]);
        } else {
            return T{};
        }
    }
};

static_assert(sizeof(BoardPresencePartitionMetadataLogic<>) == 64, "BoardPresencePartitionMetadataLogic must be exactly 64 bytes!");

} // namespace ideam::core