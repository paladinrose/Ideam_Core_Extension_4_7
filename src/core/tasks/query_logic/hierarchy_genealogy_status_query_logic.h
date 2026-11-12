#pragma once

/**
 * hierarchy_genealogy_status_query_logic.h
 * Ecosystem Companion Bridge (GR3): Legacy Unity `Hierarchy.cs` 4-State Genealogy Status Port.
 *
 * Evaluates the 4-state `HierarchyStatus` (`Active`, `PersistentlyActive`, `Disabled`, `Paused`)
 * across parent-child genealogy trees (`parent_id_buffer_id` and/or CSR `child_row_offsets_buffer_id` +
 * `child_indices_buffer_id`):
 *   - `PersistentlyActive` nodes override `Disabled` or `Paused` ancestors (and shield their own
 *     active descendants unless a closer intermediate ancestor is `Disabled`).
 *   - `Paused` nodes halt their own execution (`effective_status == PAUSED`) WITHOUT causally
 *     disabling their children (children with local `Active` remain `Active` unless `cascade_pause`
 *     is explicitly enabled).
 *   - `Disabled` nodes causally disable themselves and all descendants that do not have an
 *     intervening `PersistentlyActive` ancestor.
 */

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/bridge_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_HIERARCHY_GENEALOGY_STATUS_ENUMS_DEFINED
#define IDEAM_HIERARCHY_GENEALOGY_STATUS_ENUMS_DEFINED

/**
 * 4-State `HierarchyStatus` ported from `old_unity_code/Ideam Group/Ideam/Logic/Sets/Hierarchy.cs`.
 */
enum class HierarchyStatus : uint8_t {
    ACTIVE              = 0, // Active unless an unshielded ancestor is DISABLED
    PERSISTENTLY_ACTIVE = 1, // Always active; overrides DISABLED / PAUSED ancestors
    DISABLED            = 2, // Inactive and causally disables non-persistent descendants
    PAUSED              = 3  // Halts self without causally disabling children
};

enum HierarchyStatusMask : uint8_t {
    HIERARCHY_MASK_ACTIVE              = 1u << static_cast<uint8_t>(HierarchyStatus::ACTIVE),
    HIERARCHY_MASK_PERSISTENTLY_ACTIVE = 1u << static_cast<uint8_t>(HierarchyStatus::PERSISTENTLY_ACTIVE),
    HIERARCHY_MASK_DISABLED            = 1u << static_cast<uint8_t>(HierarchyStatus::DISABLED),
    HIERARCHY_MASK_PAUSED              = 1u << static_cast<uint8_t>(HierarchyStatus::PAUSED),
    HIERARCHY_MASK_EXECUTABLE_DEFAULT  = HIERARCHY_MASK_ACTIVE | HIERARCHY_MASK_PERSISTENTLY_ACTIVE,
    HIERARCHY_MASK_ENABLED_IN_TREE     = HIERARCHY_MASK_ACTIVE | HIERARCHY_MASK_PERSISTENTLY_ACTIVE | HIERARCHY_MASK_PAUSED
};

#endif // IDEAM_HIERARCHY_GENEALOGY_STATUS_ENUMS_DEFINED

template <typename T = int32_t, typename T_Strategy = FlatStrategy>
struct alignas(64) HierarchyGenealogyStatusQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = BridgeView<T, T, 1, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::BYTE;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Hierarchy Genealogy Status";
    static constexpr std::string_view ui_category  = "Graphs/Hierarchy";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID;                        // 4B: Target selection buffer
    uint32_t local_status_buffer_id       = INVALID_ID;                        // 4B: Local HierarchyStatus (uint8_t / int32_t / uint32_t)
    uint32_t parent_id_buffer_id          = INVALID_ID;                        // 4B: Parent node index per element (INVALID_ID or <0 = root)
    uint32_t child_row_offsets_buffer_id  = INVALID_ID;                        // 4B: Optional CSR child row offsets [N + 1]
    uint32_t child_indices_buffer_id      = INVALID_ID;                        // 4B: Optional CSR child node indices
    uint32_t effective_status_buffer_id   = INVALID_ID;                        // 4B: Optional output column receiving resolved HierarchyStatus
    uint32_t anchor_ancestor_id           = INVALID_ID;                        // 4B: Optional required ancestor NodeID filter (INVALID_ID = any) -> 28B

    uint8_t accepted_status_mask          = HIERARCHY_MASK_EXECUTABLE_DEFAULT; // 1B: Bitmask of HierarchyStatus values that pass the query
    uint8_t max_genealogy_depth           = 32u;                               // 1B: Cycle-safe maximum ancestor traversal depth (1..128)
    bool cascade_pause_to_children        = false;                             // 1B: False = Paused halts self only (Hierarchy.cs default)
    bool write_effective_status           = true;                              // 1B: Write resolved status into effective_status_buffer_id if bound
    bool require_anchor_ancestor          = false;                             // 1B: Require node to descend from anchor_ancestor_id
    bool negate_result                    = false;                             // 1B: Invert final selection predicate -> 34B
    uint8_t reserved_padding[30]          = {0};                               // 30B -> Total = 64B

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        (void)p_part;
        if constexpr (requires { p_view.bind_secondary(nullptr); }) {
            const uint32_t sec_id = (parent_id_buffer_id != INVALID_ID)
                ? parent_id_buffer_id
                : ((local_status_buffer_id != INVALID_ID) ? local_status_buffer_id : get_target_buffer_id());
            if (const GrantPartPOD* sec_part = p_context.get_grant_part(sec_id)) {
                p_view.bind_secondary(sec_part);
            }
        }
    }

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("local_status_buffer_id");
        add_buf("parent_id_buffer_id");
        add_buf("child_row_offsets_buffer_id");
        add_buf("child_indices_buffer_id");
        add_buf("effective_status_buffer_id");

        godot::Dictionary mask_prop;
        mask_prop["name"]        = "accepted_status_mask";
        mask_prop["type"]        = godot::Variant::INT;
        mask_prop["hint"]        = godot::PROPERTY_HINT_FLAGS;
        mask_prop["hint_string"] = "Active,PersistentlyActive,Disabled,Paused";
        props.push_back(mask_prop);

        godot::Dictionary depth_prop;
        depth_prop["name"]        = "max_genealogy_depth";
        depth_prop["type"]        = godot::Variant::INT;
        depth_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        depth_prop["hint_string"] = "1,128,1";
        props.push_back(depth_prop);

        godot::Dictionary anc_prop;
        anc_prop["name"] = "anchor_ancestor_id";
        anc_prop["type"] = godot::Variant::INT;
        anc_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(anc_prop);

        godot::Dictionary cp_prop;
        cp_prop["name"] = "cascade_pause_to_children";
        cp_prop["type"] = godot::Variant::BOOL;
        cp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cp_prop);

        godot::Dictionary we_prop;
        we_prop["name"] = "write_effective_status";
        we_prop["type"] = godot::Variant::BOOL;
        we_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(we_prop);

        godot::Dictionary ra_prop;
        ra_prop["name"] = "require_anchor_ancestor";
        ra_prop["type"] = godot::Variant::BOOL;
        ra_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ra_prop);

        godot::Dictionary neg_prop;
        neg_prop["name"] = "negate_result";
        neg_prop["type"] = godot::Variant::BOOL;
        neg_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(neg_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)       return target_buffer_id;
        if (local_status_buffer_id != INVALID_ID) return local_status_buffer_id;
        return parent_id_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("local_status_buffer_id")) {
            local_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["local_status_buffer_id"]));
        }
        if (p_props.has("parent_id_buffer_id")) {
            parent_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_id_buffer_id"]));
        }
        if (p_props.has("child_row_offsets_buffer_id")) {
            child_row_offsets_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["child_row_offsets_buffer_id"]));
        }
        if (p_props.has("child_indices_buffer_id")) {
            child_indices_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["child_indices_buffer_id"]));
        }
        if (p_props.has("effective_status_buffer_id")) {
            effective_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["effective_status_buffer_id"]));
        }
        if (p_props.has("anchor_ancestor_id")) {
            anchor_ancestor_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["anchor_ancestor_id"]));
        }
        if (p_props.has("accepted_status_mask")) {
            accepted_status_mask = static_cast<uint8_t>(static_cast<int64_t>(p_props["accepted_status_mask"]) & 0x0Fu);
        }
        if (p_props.has("max_genealogy_depth")) {
            max_genealogy_depth = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_genealogy_depth"]), 1, 128));
        }
        if (p_props.has("cascade_pause_to_children")) {
            cascade_pause_to_children = static_cast<bool>(p_props["cascade_pause_to_children"]);
        }
        if (p_props.has("write_effective_status")) {
            write_effective_status = static_cast<bool>(p_props["write_effective_status"]);
        }
        if (p_props.has("require_anchor_ancestor")) {
            require_anchor_ancestor = static_cast<bool>(p_props["require_anchor_ancestor"]);
        }
        if (p_props.has("negate_result")) {
            negate_result = static_cast<bool>(p_props["negate_result"]);
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strat>
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
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint8_t>) {
            const uint32_t tgt_id   = get_target_buffer_id();
            const uint32_t stat_id  = (local_status_buffer_id != INVALID_ID) ? local_status_buffer_id : tgt_id;

            const GrantPartPOD* stat_part = (stat_id != INVALID_ID)                    ? p_context.get_grant_part(stat_id)                    : nullptr;
            const GrantPartPOD* par_part  = (parent_id_buffer_id != INVALID_ID)        ? p_context.get_grant_part(parent_id_buffer_id)        : nullptr;
            const GrantPartPOD* coff_part = (child_row_offsets_buffer_id != INVALID_ID)? p_context.get_grant_part(child_row_offsets_buffer_id): nullptr;
            const GrantPartPOD* cidx_part = (child_indices_buffer_id != INVALID_ID)    ? p_context.get_grant_part(child_indices_buffer_id)    : nullptr;
            const GrantPartPOD* eff_part  = (effective_status_buffer_id != INVALID_ID) ? p_context.get_grant_part(effective_status_buffer_id) : nullptr;

            if (!stat_part || !stat_part->raw_base_ptr) return;

            const uint8_t* __restrict stat_raw = stat_part->raw_base_ptr;
            const uint32_t stat_stride = (stat_part->element_stride > 0u) ? stat_part->element_stride : static_cast<uint32_t>(sizeof(T));
            const int64_t stat_cap     = static_cast<int64_t>(stat_part->capacity_bytes / stat_stride);
            if (stat_cap <= 0) return;

            const uint32_t* __restrict parent_ids = (par_part && par_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(par_part->raw_base_ptr)
                : nullptr;
            const int64_t par_cap = parent_ids ? static_cast<int64_t>(par_part->capacity_bytes / sizeof(uint32_t)) : 0;

            uint8_t* __restrict eff_raw = (eff_part && eff_part->raw_base_ptr) ? eff_part->raw_base_ptr : nullptr;
            const uint32_t eff_stride   = eff_part ? (eff_part->element_stride > 0u ? eff_part->element_stride : stat_stride) : 0u;
            const int64_t eff_cap       = (eff_part && eff_stride > 0u) ? static_cast<int64_t>(eff_part->capacity_bytes / eff_stride) : 0;

            auto read_local_status = [&](int64_t idx) noexcept -> HierarchyStatus {
                if (idx < 0 || idx >= stat_cap) return HierarchyStatus::DISABLED;
                uint32_t raw_v = 0u;
                if (stat_stride >= sizeof(uint32_t)) {
                    raw_v = reinterpret_cast<const uint32_t*>(stat_raw)[idx];
                } else if (stat_stride == sizeof(uint16_t)) {
                    raw_v = reinterpret_cast<const uint16_t*>(stat_raw)[idx];
                } else {
                    raw_v = stat_raw[idx];
                }
                return (raw_v <= static_cast<uint32_t>(HierarchyStatus::PAUSED))
                    ? static_cast<HierarchyStatus>(raw_v)
                    : HierarchyStatus::DISABLED;
            };

            auto write_eff_status = [&](int64_t idx, HierarchyStatus st) noexcept {
                if (!write_effective_status || !eff_raw || idx < 0 || idx >= eff_cap) return;
                if (eff_stride >= sizeof(uint32_t)) {
                    reinterpret_cast<uint32_t*>(eff_raw)[idx] = static_cast<uint32_t>(st);
                } else if (eff_stride == sizeof(uint16_t)) {
                    reinterpret_cast<uint16_t*>(eff_raw)[idx] = static_cast<uint16_t>(st);
                } else {
                    eff_raw[idx] = static_cast<uint8_t>(st);
                }
            };

            // Optional CSR top-down propagation when child_row_offsets + child_indices are provided without parent_id_buffer_id
            const uint32_t* __restrict child_offsets = (coff_part && coff_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(coff_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict child_indices = (cidx_part && cidx_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(cidx_part->raw_base_ptr)
                : nullptr;
            const int64_t coff_cap = child_offsets ? static_cast<int64_t>(coff_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t cidx_cap = child_indices ? static_cast<int64_t>(cidx_part->capacity_bytes / sizeof(uint32_t)) : 0;

            auto resolve_genealogy_status = [&](int64_t idx, bool& r_has_anchor) noexcept -> HierarchyStatus {
                const HierarchyStatus self_status = read_local_status(idx);
                r_has_anchor = (!require_anchor_ancestor || anchor_ancestor_id == INVALID_ID || static_cast<uint32_t>(idx) == anchor_ancestor_id);

                // PersistentlyActive always overrides ancestor Disabled/Paused states
                if (self_status == HierarchyStatus::PERSISTENTLY_ACTIVE && !require_anchor_ancestor) {
                    return HierarchyStatus::PERSISTENTLY_ACTIVE;
                }
                if (self_status == HierarchyStatus::DISABLED && !require_anchor_ancestor) {
                    return HierarchyStatus::DISABLED;
                }

                HierarchyStatus resolved = self_status;
                bool shielded_by_persistent = (self_status == HierarchyStatus::PERSISTENTLY_ACTIVE);

                if (parent_ids && idx >= 0 && idx < par_cap) {
                    int64_t cur = idx;
                    for (uint8_t depth = 0u; depth < max_genealogy_depth; ++depth) {
                        if (cur < 0 || cur >= par_cap) break;
                        const uint32_t raw_parent = parent_ids[cur];
                        const int64_t p_idx = static_cast<int64_t>(static_cast<int32_t>(raw_parent));
                        if (raw_parent == INVALID_ID || p_idx < 0 || p_idx >= stat_cap || p_idx == cur) {
                            break;
                        }

                        if (require_anchor_ancestor && raw_parent == anchor_ancestor_id) {
                            r_has_anchor = true;
                        }

                        if (!shielded_by_persistent) {
                            const HierarchyStatus anc_status = read_local_status(p_idx);
                            if (anc_status == HierarchyStatus::PERSISTENTLY_ACTIVE) {
                                // Ancestor is PersistentlyActive: shields this branch from any higher Disabled ancestor!
                                shielded_by_persistent = true;
                            } else if (anc_status == HierarchyStatus::DISABLED) {
                                // Unshielded Disabled ancestor causally disables descendants
                                resolved = HierarchyStatus::DISABLED;
                                shielded_by_persistent = true;
                            } else if (anc_status == HierarchyStatus::PAUSED && cascade_pause_to_children) {
                                // Only if cascade_pause_to_children is enabled; default Hierarchy.cs does NOT disable children of Paused
                                if (resolved == HierarchyStatus::ACTIVE) {
                                    resolved = HierarchyStatus::PAUSED;
                                }
                            }
                        }

                        if (shielded_by_persistent && r_has_anchor) {
                            break;
                        }
                        cur = p_idx;
                    }
                } else if (child_offsets && child_indices && !shielded_by_persistent) {
                    // Fallback 1-hop parent scan in CSR child table if parent_ids column is omitted
                    const int64_t n_parents = std::min<int64_t>(stat_cap, coff_cap - 1);
                    for (int64_t p = 0; p < n_parents; ++p) {
                        const uint32_t s_e = child_offsets[p];
                        const uint32_t e_e = std::min<uint32_t>(child_offsets[p + 1], static_cast<uint32_t>(cidx_cap));
                        for (uint32_t k = s_e; k < e_e; ++k) {
                            if (static_cast<int64_t>(child_indices[k]) == idx) {
                                if (require_anchor_ancestor && static_cast<uint32_t>(p) == anchor_ancestor_id) {
                                    r_has_anchor = true;
                                }
                                const HierarchyStatus anc_status = read_local_status(p);
                                if (anc_status == HierarchyStatus::DISABLED) {
                                    resolved = HierarchyStatus::DISABLED;
                                } else if (anc_status == HierarchyStatus::PAUSED && cascade_pause_to_children && resolved == HierarchyStatus::ACTIVE) {
                                    resolved = HierarchyStatus::PAUSED;
                                }
                                break;
                            }
                        }
                    }
                }

                return resolved;
            };

            auto eval_row = [&](int64_t idx) noexcept -> bool {
                bool has_anchor = true;
                const HierarchyStatus eff_st = resolve_genealogy_status(idx, has_anchor);
                write_eff_status(idx, eff_st);

                const uint8_t bit = static_cast<uint8_t>(1u << static_cast<uint8_t>(eff_st));
                const bool match  = has_anchor && ((accepted_status_mask & bit) != 0u);
                return negate_result ? !match : match;
            };

            if constexpr (Op == QueryOp::CULL) {
                if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                    uint64_t* bitset = r_selection.data.bitset;
                    const int64_t cap = std::min<int64_t>(r_selection.capacity, stat_cap);
                    for (int64_t i = 0; i < cap; ++i) {
                        if (bitset[i >> 6] & (1ULL << (i & 63))) {
                            if (!eval_row(i)) {
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
                        if (idx >= 0 && idx < stat_cap && eval_row(idx)) {
                            indices[write_ptr++] = idx;
                        }
                    }
                    r_selection.element_count = write_ptr;
                } else if (r_selection.mode == SelectionMode::RANGE && r_selection.data.indices) {
                    const int64_t start = std::max<int64_t>(0, r_selection.start_index);
                    const int64_t end   = std::min<int64_t>({start + r_selection.element_count, r_selection.capacity, stat_cap});
                    int64_t write_ptr   = 0;
                    for (int64_t i = start; i < end; ++i) {
                        if (eval_row(i)) {
                            r_selection.data.indices[write_ptr++] = i;
                        }
                    }
                    r_selection.mode          = SelectionMode::SPARSE;
                    r_selection.start_index   = 0;
                    r_selection.element_count = write_ptr;
                }
            } else if constexpr (Op == QueryOp::ADD) {
                const int64_t cap = std::min<int64_t>(r_selection.capacity, stat_cap);
                if (r_selection.unclaimed_mask) {
                    const uint64_t* unclaimed = r_selection.unclaimed_mask;
                    const int64_t words       = (cap + 63) >> 6;
                    for (int64_t w = 0; w < words; ++w) {
                        uint64_t mask = unclaimed[w];
                        while (mask != 0ULL) {
                            const int bit_index      = std::countr_zero(mask);
                            const int64_t global_idx = (w << 6) + bit_index;
                            if (global_idx >= cap) break;
                            if (eval_row(global_idx)) {
                                p_context.queue_selection_command(tgt_id, global_idx);
                            }
                            mask &= (mask - 1ULL);
                        }
                    }
                } else {
                    for (int64_t i = 0; i < cap; ++i) {
                        if (eval_row(i)) {
                            p_context.queue_selection_command(tgt_id, i);
                        }
                    }
                }
            }
        }
    }
};

static_assert(sizeof(HierarchyGenealogyStatusQueryLogic<>) == 64, "HierarchyGenealogyStatusQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<HierarchyGenealogyStatusQueryLogic<>>, "HierarchyGenealogyStatusQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core
