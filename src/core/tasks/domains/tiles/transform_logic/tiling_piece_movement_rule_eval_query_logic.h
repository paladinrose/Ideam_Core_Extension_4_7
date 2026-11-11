#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../tasks/i_native_task.h"
#include "../../tasks/task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * TIL9: TilingPieceMovementRuleEvalQueryLogic
 * Evaluates step/radius rings, directional ray slides (Rook/Bishop/Queen), topological L-jumps (Knight),
 * and symmetry-orbit movement rules across TileDOD dual-graph neighbor tables and occupancy buffers,
 * waking/sleeping valid destination board spaces in r_selection for a GamePiece.
 */
struct TilingPieceMovementRuleEvalQueryLogic {
    static constexpr std::string_view ui_name = "Tiling Piece Movement Rule Evaluator";
    static constexpr std::string_view ui_category = "Tiles/GamesBridge";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::BRIDGE,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 4> supported_types = {
        DataType::INT32,
        DataType::UINT32,
        DataType::VECTOR2,
        DataType::VECTOR3
    };
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::WAKE,
        QueryOp::SLEEP,
        QueryOp::INVERT
    };

    static constexpr int32_t MAX_BOARD_SPACES = 4096;

    int32_t origin_space_index = 0;
    int32_t movement_pattern = 1; // 0 = Step Ring (BFS), 1 = Ray Slide, 2 = Topological Jump (Knight), 3 = Symmetry Orbit
    int32_t valence_ports = 8;
    int32_t min_steps = 1;
    int32_t max_steps = 8;
    int32_t ray_direction_mask = 0xFF;
    int32_t jump_primary_steps = 2;
    int32_t jump_secondary_steps = 1;
    int32_t piece_team_id = 1;
    bool allow_capture_enemy = true;
    bool stop_on_occupied = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            list.push_back(d);
        };
        add_prop("origin_space_index", godot::Variant::INT, origin_space_index);
        add_prop("movement_pattern", godot::Variant::INT, movement_pattern, 2, "Step Ring (BFS),Directional Ray Slide,Topological Jump (L-Hop),Symmetry Orbit");
        add_prop("valence_ports", godot::Variant::INT, valence_ports);
        add_prop("min_steps", godot::Variant::INT, min_steps);
        add_prop("max_steps", godot::Variant::INT, max_steps);
        add_prop("ray_direction_mask", godot::Variant::INT, ray_direction_mask);
        add_prop("jump_primary_steps", godot::Variant::INT, jump_primary_steps);
        add_prop("jump_secondary_steps", godot::Variant::INT, jump_secondary_steps);
        add_prop("piece_team_id", godot::Variant::INT, piece_team_id);
        add_prop("allow_capture_enemy", godot::Variant::BOOL, allow_capture_enemy);
        add_prop("stop_on_occupied", godot::Variant::BOOL, stop_on_occupied);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
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

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_BOARD_SPACES * sizeof(int32_t) * 2 + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_BOARD_SPACES * sizeof(int32_t) * 2 + 256;
    }

    static inline void apply_selection_op(MemoryBufferSelectionPOD& r_sel, int64_t idx, QueryOp p_op) noexcept {
        if (idx < 0 || idx >= r_sel.capacity) return;
        if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
            const uint64_t word = static_cast<uint64_t>(idx) >> 6;
            const uint64_t bit = 1ULL << (idx & 63);
            const bool was_set = (r_sel.data.bitset[word] & bit) != 0;
            if (p_op == QueryOp::WAKE && !was_set) {
                r_sel.data.bitset[word] |= bit;
                r_sel.element_count++;
            } else if (p_op == QueryOp::SLEEP && was_set) {
                r_sel.data.bitset[word] &= ~bit;
                r_sel.element_count = std::max<int64_t>(0, r_sel.element_count - 1);
            } else if (p_op == QueryOp::INVERT) {
                r_sel.data.bitset[word] ^= bit;
                r_sel.element_count += was_set ? -1 : 1;
            }
        } else if (r_sel.mode == SelectionMode::SPARSE && r_sel.data.indices) {
            if (p_op == QueryOp::WAKE && r_sel.element_count < r_sel.capacity) {
                r_sel.data.indices[r_sel.element_count++] = idx;
            }
        }
    }

    void execute_with_op(const TaskContextPOD& p_ctx, QueryOp p_op) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& adj_part = p_ctx.grant->parts[0];
        if (!adj_part.raw_base_ptr) return;

        const GrantPartPOD* occ_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        MemoryBufferSelectionPOD& sel = adj_part.selection;
        const int32_t ports = std::clamp<int32_t>(valence_ports, 1, 8);
        const int32_t n_spaces = std::min<int32_t>(
            MAX_BOARD_SPACES,
            static_cast<int32_t>((adj_part.capacity_bytes / sizeof(int32_t)) / ports)
        );
        if (n_spaces <= 0 || origin_space_index < 0 || origin_space_index >= n_spaces) return;

        const int32_t* adj_table = reinterpret_cast<const int32_t*>(adj_part.raw_base_ptr);
        const int32_t* occ_table = (occ_part && occ_part->capacity_bytes >= static_cast<size_t>(n_spaces) * sizeof(int32_t))
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

        if (movement_pattern == 1) {
            // Pattern 1: Directional Ray Slide (Rook / Bishop / Queen / Hex Ray)
            for (int32_t p = 0; p < ports; ++p) {
                if (!(ray_direction_mask & (1 << p))) continue;
                int32_t curr = origin_space_index;
                for (int32_t step = 1; step <= max_steps; ++step) {
                    const int32_t next = adj_table[static_cast<size_t>(curr) * ports + p];
                    if (next < 0 || next >= n_spaces) break;

                    bool can_land = true, blocks = false;
                    check_occupant(next, can_land, blocks);
                    if (step >= min_steps && can_land) {
                        apply_selection_op(sel, next, p_op);
                    }
                    if (blocks) break;
                    curr = next;
                }
            }
        } else if (movement_pattern == 2) {
            // Pattern 2: Topological Jump (Knight / L-Hop across dual-graph ports)
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
                            apply_selection_op(sel, branch, p_op);
                        }
                    }
                }
            }
        } else if (movement_pattern == 0 && p_ctx.local_workspace) {
            // Pattern 0: Step / Radius Ring via BFS
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
                        apply_selection_op(sel, v, p_op);
                    }
                    if (!blocks && q_tail < MAX_BOARD_SPACES) {
                        queue[q_tail++] = v;
                    }
                }
            }
        } else if (movement_pattern == 3 && sel.partition_ids) {
            // Pattern 3: Quotient Symmetry Orbit (wake spaces sharing origin's prototile partition_id)
            const int64_t target_orbit = sel.partition_ids[origin_space_index];
            for (int32_t i = 0; i < n_spaces; ++i) {
                if (i == origin_space_index) continue;
                if (sel.partition_ids[i] == target_orbit) {
                    bool can_land = true, blocks = false;
                    check_occupant(i, can_land, blocks);
                    if (can_land) {
                        apply_selection_op(sel, i, p_op);
                    }
                }
            }
        }
    }

    void execute(const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, QueryOp p_op, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, p_op);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }
};

} // namespace ideam::core