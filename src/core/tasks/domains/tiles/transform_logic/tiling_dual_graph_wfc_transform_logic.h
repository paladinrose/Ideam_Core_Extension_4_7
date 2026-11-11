#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../tasks/i_native_task.h"
#include "../../tasks/task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * TIL8: TilingDualGraphWFCTransformLogic
 * Runs either:
 *   - Mode 0: Symmetry-Aware Wave Function Collapse (bitmask superpositions + BFS arc-consistency propagation)
 *   - Mode 1: Gray-Scott two-chemical reaction-diffusion PDE over Wallpaper/Isohedral dual graphs
 */
struct TilingDualGraphWFCTransformLogic {
    static constexpr std::string_view ui_name = "Tiling Dual-Graph WFC & Reaction-Diffusion";
    static constexpr std::string_view ui_category = "Tiles/Procedural";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SWAP,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 4> supported_types = {
        DataType::INT32,
        DataType::UINT32,
        DataType::FLOAT32,
        DataType::VECTOR2
    };

    static constexpr int32_t MAX_DUAL_CELLS = 8192;

    int32_t solver_mode = 0; // 0 = Bitmask WFC, 1 = Gray-Scott Reaction-Diffusion
    int32_t num_states = 8;
    int32_t valence_ports = 6;
    int32_t random_seed = 1337;
    int32_t max_collapse_steps = 64;
    float diffusion_u = 0.16f;
    float diffusion_v = 0.08f;
    float feed_rate = 0.055f;
    float kill_rate = 0.062f;

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
        add_prop("solver_mode", godot::Variant::INT, solver_mode, 2, "Symmetry-Aware WFC,Gray-Scott Reaction-Diffusion");
        add_prop("num_states", godot::Variant::INT, num_states);
        add_prop("valence_ports", godot::Variant::INT, valence_ports);
        add_prop("random_seed", godot::Variant::INT, random_seed);
        add_prop("max_collapse_steps", godot::Variant::INT, max_collapse_steps);
        add_prop("diffusion_u", godot::Variant::FLOAT, diffusion_u);
        add_prop("diffusion_v", godot::Variant::FLOAT, diffusion_v);
        add_prop("feed_rate", godot::Variant::FLOAT, feed_rate);
        add_prop("kill_rate", godot::Variant::FLOAT, kill_rate);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("solver_mode")) solver_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["solver_mode"]), 0, 1);
        if (p_props.has("num_states")) num_states = std::clamp<int32_t>(static_cast<int32_t>(p_props["num_states"]), 1, 31);
        if (p_props.has("valence_ports")) valence_ports = std::clamp<int32_t>(static_cast<int32_t>(p_props["valence_ports"]), 1, 8);
        if (p_props.has("random_seed")) random_seed = static_cast<int32_t>(p_props["random_seed"]);
        if (p_props.has("max_collapse_steps")) max_collapse_steps = std::max<int32_t>(1, static_cast<int32_t>(p_props["max_collapse_steps"]));
        if (p_props.has("diffusion_u")) diffusion_u = std::max(0.0f, static_cast<float>(p_props["diffusion_u"]));
        if (p_props.has("diffusion_v")) diffusion_v = std::max(0.0f, static_cast<float>(p_props["diffusion_v"]));
        if (p_props.has("feed_rate")) feed_rate = std::max(0.0f, static_cast<float>(p_props["feed_rate"]));
        if (p_props.has("kill_rate")) kill_rate = std::max(0.0f, static_cast<float>(p_props["kill_rate"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_DUAL_CELLS * sizeof(godot::Vector2) + MAX_DUAL_CELLS * sizeof(int32_t) + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_DUAL_CELLS * sizeof(godot::Vector2) + MAX_DUAL_CELLS * sizeof(int32_t) + 256;
    }

    static inline uint32_t hash_u32(uint32_t x) noexcept {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return x;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2 || !p_ctx.local_workspace) return;

        GrantPartPOD& state_part = p_ctx.grant->parts[0];
        const GrantPartPOD& adj_part = p_ctx.grant->parts[1];
        if (!state_part.raw_base_ptr || !adj_part.raw_base_ptr) return;

        const GrantPartPOD* rule_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        const int32_t ports = std::clamp<int32_t>(valence_ports, 1, 8);
        const int32_t max_adj_cells = static_cast<int32_t>((adj_part.capacity_bytes / sizeof(int32_t)) / ports);
        const int32_t raw_cells = (state_part.selection.element_count > 0)
            ? static_cast<int32_t>(state_part.selection.element_count)
            : static_cast<int32_t>(state_part.capacity_bytes / std::max<uint32_t>(4, state_part.element_stride));
        const int32_t n_cells = std::min({raw_cells, max_adj_cells, MAX_DUAL_CELLS});
        if (n_cells <= 0) return;

        const int32_t* adj_table = reinterpret_cast<const int32_t*>(adj_part.raw_base_ptr);

        if (solver_mode == 0) {
            // Mode 0: Bitmask Wave Function Collapse over Dual Graph
            uint32_t* domains = reinterpret_cast<uint32_t*>(state_part.raw_base_ptr);
            const uint32_t full_mask = (num_states >= 31) ? 0x7FFFFFFFu : ((1u << num_states) - 1u);

            for (int32_t i = 0; i < n_cells; ++i) {
                if (domains[i] == 0) domains[i] = full_mask;
            }

            const uint32_t* compat_masks = (rule_part && rule_part->capacity_bytes >= static_cast<size_t>(num_states) * sizeof(uint32_t))
                ? reinterpret_cast<const uint32_t*>(rule_part->raw_base_ptr)
                : nullptr;

            auto allowed_neighbor_mask = [&](uint32_t cell_mask) noexcept -> uint32_t {
                uint32_t allowed = 0;
                for (int32_t s = 0; s < num_states; ++s) {
                    if (!(cell_mask & (1u << s))) continue;
                    if (compat_masks) {
                        allowed |= compat_masks[s];
                    } else {
                        // Default smooth adjacency rule: state s compatible with s-1, s, s+1 (mod num_states)
                        allowed |= (1u << s);
                        allowed |= (1u << ((s + 1) % num_states));
                        allowed |= (1u << ((s + num_states - 1) % num_states));
                    }
                }
                return allowed & full_mask;
            };

            int32_t* bfs_queue = static_cast<int32_t*>(p_ctx.local_workspace);
            uint32_t rng = static_cast<uint32_t>(random_seed);

            for (int32_t step = 0; step < max_collapse_steps; ++step) {
                // Find uncollapsed cell with minimum popcount entropy > 1
                int32_t best_cell = -1;
                int32_t best_entropy = 999;
                uint32_t best_tie = 0xFFFFFFFFu;

                for (int32_t i = 0; i < n_cells; ++i) {
                    const int32_t bits = std::popcount(domains[i]);
                    if (bits <= 1) continue;
                    const uint32_t tie = hash_u32(rng ^ static_cast<uint32_t>(i));
                    if (bits < best_entropy || (bits == best_entropy && tie < best_tie)) {
                        best_entropy = bits;
                        best_tie = tie;
                        best_cell = i;
                    }
                }

                if (best_cell < 0) break; // Fully collapsed

                // Collapse best_cell to one of its valid bit states
                rng = hash_u32(rng + static_cast<uint32_t>(step) + 1u);
                const int32_t pick_idx = static_cast<int32_t>(rng % static_cast<uint32_t>(best_entropy));
                uint32_t mask = domains[best_cell];
                int32_t chosen_bit = 0;
                int32_t seen = 0;
                for (int32_t b = 0; b < num_states; ++b) {
                    if (mask & (1u << b)) {
                        if (seen == pick_idx) {
                            chosen_bit = b;
                            break;
                        }
                        ++seen;
                    }
                }
                domains[best_cell] = (1u << chosen_bit);

                // Propagate constraints via BFS
                int32_t q_head = 0, q_tail = 0;
                bfs_queue[q_tail++] = best_cell;

                while (q_head < q_tail) {
                    const int32_t curr = bfs_queue[q_head++ % MAX_DUAL_CELLS];
                    const uint32_t allowed = allowed_neighbor_mask(domains[curr]);
                    const int32_t* n_row = adj_table + static_cast<size_t>(curr) * ports;

                    for (int32_t p = 0; p < ports; ++p) {
                        const int32_t nb = n_row[p];
                        if (nb < 0 || nb >= n_cells) continue;
                        const uint32_t old_dom = domains[nb];
                        const uint32_t new_dom = old_dom & allowed;
                        if (new_dom != 0 && new_dom != old_dom) {
                            domains[nb] = new_dom;
                            if (q_tail - q_head < MAX_DUAL_CELLS) {
                                bfs_queue[q_tail++ % MAX_DUAL_CELLS] = nb;
                            }
                        }
                    }
                }
            }
        } else {
            // Mode 1: Gray-Scott Reaction-Diffusion over Dual Graph
            if (state_part.capacity_bytes < static_cast<size_t>(n_cells) * sizeof(godot::Vector2)) return;
            godot::Vector2* uv = reinterpret_cast<godot::Vector2*>(state_part.raw_base_ptr);
            godot::Vector2* next_uv = static_cast<godot::Vector2*>(p_ctx.local_workspace);
            const float dt = (p_ctx.delta > 0.0) ? std::min(1.0f, static_cast<float>(p_ctx.delta)) : 1.0f;

            for (int32_t i = 0; i < n_cells; ++i) {
                const godot::Vector2 cur = uv[i];
                const int32_t* n_row = adj_table + static_cast<size_t>(i) * ports;

                godot::Vector2 lap(0.0f, 0.0f);
                int32_t deg = 0;
                for (int32_t p = 0; p < ports; ++p) {
                    const int32_t nb = n_row[p];
                    if (nb >= 0 && nb < n_cells) {
                        lap += (uv[nb] - cur);
                        ++deg;
                    }
                }
                if (deg > 0) {
                    lap /= static_cast<float>(deg);
                }

                const float u = cur.x;
                const float v = cur.y;
                const float uvv = u * v * v;
                const float du = diffusion_u * lap.x - uvv + feed_rate * (1.0f - u);
                const float dv = diffusion_v * lap.y + uvv - (feed_rate + kill_rate) * v;

                next_uv[i] = godot::Vector2(
                    std::clamp(u + du * dt, 0.0f, 1.0f),
                    std::clamp(v + dv * dt, 0.0f, 1.0f)
                );
            }

            for (int32_t i = 0; i < n_cells; ++i) {
                uv[i] = next_uv[i];
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core