#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * TilingPieceMovementRuleEvalQueryLogic<T, T_Strategy, T_View> (TIL9)
 * Evaluates step/radius rings, directional ray slides (Rook/Bishop/Queen), topological L-jumps (Knight),
 * and symmetry-orbit movement rules across TileDOD dual-graph neighbor tables and occupancy buffers,
 * culling or adding valid destination board spaces in r_selection for a GamePiece.
 */
template <
    typename T = int32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) TilingPieceMovementRuleEvalQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr int32_t MAX_BOARD_SPACES = 4096;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::INT32 | DataType::UINT32 | DataType::VECTOR2 | DataType::VECTOR3;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = MAX_BOARD_SPACES * sizeof(int32_t) * 2 + 256;

    static constexpr std::string_view display_name = "Tiling Piece Movement Rule Evaluator";
    static constexpr std::string_view ui_category  = "Tiles/GamesBridge";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id    = INVALID_ID; // 4B
    uint32_t adjacency_buffer_id = INVALID_ID; // 4B
    uint32_t occupancy_buffer_id = INVALID_ID; // 4B
    int32_t origin_space_index   = 0;          // 4B -> 16B
    int32_t movement_pattern     = 1;          // 4B: 0 = Step Ring (BFS), 1 = Ray Slide, 2 = Topological Jump, 3 = Symmetry Orbit
    int32_t valence_ports        = 8;          // 4B
    int32_t min_steps            = 1;          // 4B
    int32_t max_steps            = 8;          // 4B -> 32B
    int32_t ray_direction_mask   = 0xFF;       // 4B
    int32_t jump_primary_steps   = 2;          // 4B
    int32_t jump_secondary_steps = 1;          // 4B
    int32_t piece_team_id        = 1;          // 4B -> 48B
    bool allow_capture_enemy     = true;       // 1B
    bool stop_on_occupied        = true;       // 1B
    uint8_t reserved_padding[14] = {0};        // 14B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("adjacency_buffer_id");
        add_buf("occupancy_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            props.push_back(d);
        };
        add_prop("origin_space_index", godot::Variant::INT, 0);
        add_prop("movement_pattern", godot::Variant::INT, 1, 2, "Step Ring (BFS),Directional Ray Slide,Topological Jump (L-Hop),Symmetry Orbit");
        add_prop("valence_ports", godot::Variant::INT, 8);
        add_prop("min_steps", godot::Variant::INT, 1);
        add_prop("max_steps", godot::Variant::INT, 8);
        add_prop("ray_direction_mask", godot::Variant::INT, 0xFF);
        add_prop("jump_primary_steps", godot::Variant::INT, 2);
        add_prop("jump_secondary_steps", godot::Variant::INT, 1);
        add_prop("piece_team_id", godot::Variant::INT, 1);
        add_prop("allow_capture_enemy", godot::Variant::BOOL, true);
        add_prop("stop_on_occupied", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : adjacency_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("adjacency_buffer_id")) adjacency_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["adjacency_buffer_id"]));
        if (p_props.has("occupancy_buffer_id")) occupancy_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["occupancy_buffer_id"]));
        if (p_props.has("origin_space_index")) origin_space_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["origin_space_index"]));
        if (p_props.has("movement_pattern")) movement_pattern = std::clamp<int32_t>(static_cast<int32_t>(p_props["movement_pattern"]), 0, 3);
        if (p_props.has("valence_ports")) valence_ports = std::clamp<int32_t>(static_cast<int32_t>(p_props["valence_ports"]), 1, 8);
        if (p_props.has("min_steps")) min_steps = std::max<int32_t>(1, static_cast<int32_t>(p_props["min_steps"]));
        if (p_props.has("max_steps")) max_steps = std::max<int32_t>(min_steps, static_cast<int32_t>(p_props["max_steps"]));
        if (p_props.has("ray_direction_mask")) ray_direction_mask = static_cast<int32_t>(p_props["ray_direction_mask"]);
        if (p_props.has("jump_primary_steps")) jump_primary_steps = std::max<int32_t>(1, static_cast<int32_t>(p_props["jump_primary_steps"]));
        if (p_props.has("jump_secondary_steps")) jump_secondary_steps = std::max<int32_t>(1, static_cast<int32_t>(p_props["jump_secondary_steps"]));
        if (p_props.has("piece_team_id")) piece_team_id = static_cast<int32_t>(p_props["piece_team_id"]);
        if (p_props.has("allow_capture_enemy")) allow_capture_enemy = static_cast<bool>(p_props["allow_capture_enemy"]);
        if (p_props.has("stop_on_occupied")) stop_on_occupied = static_cast<bool>(p_props["stop_on_occupied"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <QueryOp Op>
    static inline void _mark_space(MemoryBufferSelectionPOD& r_sel, const TaskContextPOD& p_ctx, uint32_t p_buf_id, int64_t idx) noexcept {
        if (idx < 0 || idx >= r_sel.capacity) return;
        if constexpr (Op == QueryOp::ADD) {
            p_ctx.queue_selection_command(p_buf_id, idx);
            if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
                const uint64_t word = static_cast<uint64_t>(idx) >> 6;
                const uint64_t bit = 1ULL << (idx & 63);
                if (!(r_sel.data.bitset[word] & bit)) {
                    r_sel.data.bitset[word] |= bit;
                    r_sel.element_count++;
                }
            }
        } else {
            if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
                const uint64_t word = static_cast<uint64_t>(idx) >> 6;
                const uint64_t bit = 1ULL << (idx & 63);
                if (r_sel.data.bitset[word] & bit) {
                    r_sel.data.bitset[word] &= ~bit;
                    r_sel.element_count = std::max<int64_t>(0, r_sel.element_count - 1);
                }
            }
        }
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        const T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* adj_part = (adjacency_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(adjacency_buffer_id)
                : ((target_buffer_id != INVALID_ID)
                    ? p_ctx.get_grant_part(target_buffer_id)
                    : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr));
            if (!adj_part || !adj_part->raw_base_ptr) return;

            const GrantPartPOD* occ_part = (occupancy_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(occupancy_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr) ? &p_ctx.grant->parts[1] : nullptr);

            const int32_t ports = std::clamp<int32_t>(valence_ports, 1, 8);
            const int32_t n_spaces = std::min<int32_t>(
                MAX_BOARD_SPACES,
                static_cast<int32_t>((adj_part->capacity_bytes / sizeof(int32_t)) / ports)
            );
            if (n_spaces <= 0 || origin_space_index < 0 || origin_space_index >= n_spaces) return;

            const int32_t* adj_table = reinterpret_cast<const int32_t*>(adj_part->raw_base_ptr);
            const int32_t* occ_table = (occ_part && occ_part->raw_base_ptr && occ_part->capacity_bytes >= static_cast<size_t>(n_spaces) * sizeof(int32_t))
                ? reinterpret_cast<const int32_t*>(occ_part->raw_base_ptr)
                : nullptr;

            auto check_occupant = [&](int32_t space_idx, bool& r_can_land, bool& r_blocks_further) noexcept {
                if (!occ_table) {
                    r_can_land = true;
                    r_blocks_further = false;
                    return;
                }
                const int32_t occ_team = occ_table[space_idx];
                if (occ_team == 0) {
                    r_can_land = true;
                    r_blocks_further = false;
                } else if (occ_team == piece_team_id) {
                    r_can_land = false;
                    r_blocks_further = stop_on_occupied;
                } else {
                    r_can_land = allow_capture_enemy;
                    r_blocks_further = stop_on_occupied;
                }
            };

            const uint32_t active_buf_id = get_target_buffer_id();

            if (movement_pattern == 1) {
                for (int32_t p = 0; p < ports; ++p) {
                    if (!(ray_direction_mask & (1 << p))) continue;
                    int32_t curr = origin_space_index;
                    for (int32_t step = 1; step <= max_steps; ++step) {
                        const int32_t next = adj_table[static_cast<size_t>(curr) * ports + p];
                        if (next < 0 || next >= n_spaces) break;

                        bool can_land = true, blocks = false;
                        check_occupant(next, can_land, blocks);
                        if (step >= min_steps && can_land) {
                            _mark_space<Op>(r_selection, p_ctx, active_buf_id, next);
                        }
                        if (blocks) break;
                        curr = next;
                    }
                }
            } else if (movement_pattern == 2) {
                for (int32_t p = 0; p < ports; ++p) {
                    if (!(ray_direction_mask & (1 << p))) continue;
                    int32_t curr = origin_space_index;
                    bool valid_leg = true;
                    for (int32_t s1 = 0; s1 < jump_primary_steps; ++s1) {
                        curr = adj_table[static_cast<size_t>(curr) * ports + p];
                        if (curr < 0 || curr >= n_spaces) { valid_leg = false; break; }
                    }
                    if (!valid_leg) continue;

                    const int32_t ortho_ports[2] = {
                        (p + (ports / 4 > 0 ? ports / 4 : 1)) % ports,
                        (p + ports - (ports / 4 > 0 ? ports / 4 : 1)) % ports
                    };
                    for (int k = 0; k < 2; ++k) {
                        int32_t branch = curr;
                        bool ok = true;
                        for (int32_t s2 = 0; s2 < jump_secondary_steps; ++s2) {
                            branch = adj_table[static_cast<size_t>(branch) * ports + ortho_ports[k]];
                            if (branch < 0 || branch >= n_spaces) { ok = false; break; }
                        }
                        if (ok && branch != origin_space_index) {
                            bool can_land = true, blocks = false;
                            check_occupant(branch, can_land, blocks);
                            if (can_land) {
                                _mark_space<Op>(r_selection, p_ctx, active_buf_id, branch);
                            }
                        }
                    }
                }
            } else if (movement_pattern == 0 && p_ctx.local_workspace) {
                int32_t* dist = static_cast<int32_t*>(p_ctx.local_workspace);
                int32_t* queue = dist + MAX_BOARD_SPACES;
                for (int32_t i = 0; i < n_spaces; ++i) dist[i] = -1;

                int32_t q_head = 0, q_tail = 0;
                dist[origin_space_index] = 0;
                queue[q_tail++] = origin_space_index;

                while (q_head < q_tail) {
                    const int32_t u = queue[q_head++];
                    const int32_t d = dist[u];
                    if (d >= max_steps) continue;

                    const int32_t* row = adj_table + static_cast<size_t>(u) * ports;
                    for (int32_t p = 0; p < ports; ++p) {
                        if (!(ray_direction_mask & (1 << p))) continue;
                        const int32_t v = row[p];
                        if (v < 0 || v >= n_spaces || dist[v] >= 0) continue;

                        bool can_land = true, blocks = false;
                        check_occupant(v, can_land, blocks);
                        if (!can_land && blocks) continue;

                        dist[v] = d + 1;
                        if (dist[v] >= min_steps && can_land) {
                            _mark_space<Op>(r_selection, p_ctx, active_buf_id, v);
                        }
                        if (!blocks && q_tail < MAX_BOARD_SPACES) {
                            queue[q_tail++] = v;
                        }
                    }
                }
            } else if (movement_pattern == 3 && r_selection.partition_ids) {
                const int64_t target_orbit = r_selection.partition_ids[origin_space_index];
                for (int32_t i = 0; i < n_spaces; ++i) {
                    if (i == origin_space_index) continue;
                    if (r_selection.partition_ids[i] == target_orbit) {
                        bool can_land = true, blocks = false;
                        check_occupant(i, can_land, blocks);
                        if (can_land) {
                            _mark_space<Op>(r_selection, p_ctx, active_buf_id, i);
                        }
                    }
                }
            }
        }
    }
};

static_assert(sizeof(TilingPieceMovementRuleEvalQueryLogic<>) == 64, "TilingPieceMovementRuleEvalQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<TilingPieceMovementRuleEvalQueryLogic<>>, "TilingPieceMovementRuleEvalQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core