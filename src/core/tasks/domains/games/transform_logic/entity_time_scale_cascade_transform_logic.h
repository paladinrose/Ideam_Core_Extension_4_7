#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_ENTITY_HIERARCHY_FLAGS_DEFINED
#define IDEAM_GAMES_ENTITY_HIERARCHY_FLAGS_DEFINED

/**
 * Bitflags for EntityHierarchyBuffer::active_flags (uint32_t)
 * Mirrors GameEntity runtime state booleans (enabled, entity_is_paused, entity_is_initialized).
 */
enum EntityActiveFlags : uint32_t {
    ENTITY_FLAG_NONE        = 0u,
    ENTITY_FLAG_ENABLED     = 1u << 0, // GameEntity::enabled
    ENTITY_FLAG_PAUSED      = 1u << 1, // GameEntity::entity_is_paused
    ENTITY_FLAG_INITIALIZED = 1u << 2, // GameEntity::entity_is_initialized
    ENTITY_FLAG_PROCESSED   = 1u << 3  // GameEntity::game_processed
};

/**
 * Bitflags for EntityHierarchyBuffer::archetype_mask (uint32_t)
 * Identifies the concrete GameEntity subclass in the DOD hierarchy table.
 */
enum EntityArchetypeMask : uint32_t {
    ENTITY_ARCHETYPE_NONE         = 0u,
    ENTITY_ARCHETYPE_GAME         = 1u << 0,
    ENTITY_ARCHETYPE_BOARD        = 1u << 1,
    ENTITY_ARCHETYPE_AGENT        = 1u << 2,
    ENTITY_ARCHETYPE_PIECE        = 1u << 3,
    ENTITY_ARCHETYPE_AGENT_ACTION = 1u << 4,
    ENTITY_ARCHETYPE_PIECE_ACTION = 1u << 5,
    ENTITY_ARCHETYPE_INTERACTION  = 1u << 6,
    ENTITY_ARCHETYPE_ALL          = 0xFFFFFFFFu
};

#endif // IDEAM_GAMES_ENTITY_HIERARCHY_FLAGS_DEFINED

/**
 * EntityTimeScaleCascadeTransformLogic<T> (G1)
 * Propagates cumulative time_scale from Game -> GameBoard -> GameAgent -> GamePiece
 * (and owned Actions / Interactions) across topological depth levels in EntityHierarchyBuffer:
 *   effective_time_scale[i] =
 *       (parent_id != INVALID_ID)
 *           ? (effective_time_scale[parent_id] * local_time_scale[i])
 *           : (root_time_scale * local_time_scale[i])
 * Eliminates recursive scene-tree traversal and virtual GameEntity::game_process(delta) scaling.
 */
template <typename T = float>
struct alignas(64) EntityTimeScaleCascadeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = BridgeView<T, T, 1, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    // ViewCapability::NONE allows both BridgeView (DefaultView) and linear/SIMD views
    // (SingleElementView, AOSOAView, AtomicView, SwapView) to validate cleanly.
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    =
        BufferLayoutType::FLAT | BufferLayoutType::SOA |
        BufferLayoutType::AOS  | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types              = DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Entity Time Scale Cascade";
    static constexpr std::string_view ui_category  = "Hierarchy";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t effective_time_scale_buffer_id = INVALID_ID;       // 4B: Output effective_time_scale (float) column
    uint32_t local_time_scale_buffer_id     = INVALID_ID;       // 4B: Input local_time_scale (float) column
    uint32_t parent_entity_id_buffer_id     = INVALID_ID;       // 4B: Topological parent_entity_id (uint32_t/int32_t, INVALID_ID = root)
    uint32_t active_flags_buffer_id         = INVALID_ID;       // 4B: Optional active_flags (uint32_t: enabled, paused, initialized)
    uint32_t archetype_mask_buffer_id       = INVALID_ID;       // 4B: Optional archetype_mask (uint32_t) column
    uint32_t target_archetype_mask          = ENTITY_ARCHETYPE_ALL; // 4B: Mask of entity archetypes to evaluate

    float root_time_scale                   = 1.0f;             // 4B: Global Game time_scale multiplier for root entities
    float min_time_scale                    = 0.0f;             // 4B: Minimum clamped effective time scale
    float max_time_scale                    = 100.0f;           // 4B: Maximum clamped effective time scale

    uint8_t max_depth_passes                = 1;                // 1B: 1 for topologically sorted buffers; >1 for unsorted trees
    bool zero_when_paused_or_disabled       = true;             // 1B: Forces effective_time_scale = 0 when disabled or paused
    bool clamp_time_scale                   = true;             // 1B: Clamps effective_time_scale to [min_time_scale, max_time_scale]
    uint8_t reserved_padding[25]            = {0};              // 25B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary eff_prop;
        eff_prop["name"] = "effective_time_scale_buffer_id";
        eff_prop["type"] = godot::Variant::INT;
        eff_prop["hint_string"] = "buffer_option";
        props.push_back(eff_prop);

        godot::Dictionary loc_prop;
        loc_prop["name"] = "local_time_scale_buffer_id";
        loc_prop["type"] = godot::Variant::INT;
        loc_prop["hint_string"] = "buffer_option";
        props.push_back(loc_prop);

        godot::Dictionary par_prop;
        par_prop["name"] = "parent_entity_id_buffer_id";
        par_prop["type"] = godot::Variant::INT;
        par_prop["hint_string"] = "buffer_option";
        props.push_back(par_prop);

        godot::Dictionary flg_prop;
        flg_prop["name"] = "active_flags_buffer_id";
        flg_prop["type"] = godot::Variant::INT;
        flg_prop["hint_string"] = "buffer_option";
        props.push_back(flg_prop);

        godot::Dictionary arch_prop;
        arch_prop["name"] = "archetype_mask_buffer_id";
        arch_prop["type"] = godot::Variant::INT;
        arch_prop["hint_string"] = "buffer_option";
        props.push_back(arch_prop);

        godot::Dictionary tmask_prop;
        tmask_prop["name"] = "target_archetype_mask";
        tmask_prop["type"] = godot::Variant::INT;
        tmask_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        tmask_prop["hint_string"] = "Game,Board,Agent,Piece,AgentAction,PieceAction,Interaction";
        props.push_back(tmask_prop);

        godot::Dictionary root_prop;
        root_prop["name"] = "root_time_scale";
        root_prop["type"] = godot::Variant::FLOAT;
        root_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        root_prop["hint_string"] = "0.0,10.0,0.01,suffix:x";
        props.push_back(root_prop);

        godot::Dictionary min_prop;
        min_prop["name"] = "min_time_scale";
        min_prop["type"] = godot::Variant::FLOAT;
        min_prop["hint_string"] = "suffix:x";
        props.push_back(min_prop);

        godot::Dictionary max_prop;
        max_prop["name"] = "max_time_scale";
        max_prop["type"] = godot::Variant::FLOAT;
        max_prop["hint_string"] = "suffix:x";
        props.push_back(max_prop);

        godot::Dictionary pass_prop;
        pass_prop["name"] = "max_depth_passes";
        pass_prop["type"] = godot::Variant::INT;
        pass_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        pass_prop["hint_string"] = "1,8,1";
        props.push_back(pass_prop);

        godot::Dictionary zero_prop;
        zero_prop["name"] = "zero_when_paused_or_disabled";
        zero_prop["type"] = godot::Variant::BOOL;
        zero_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(zero_prop);

        godot::Dictionary clamp_prop;
        clamp_prop["name"] = "clamp_time_scale";
        clamp_prop["type"] = godot::Variant::BOOL;
        clamp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clamp_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (effective_time_scale_buffer_id != INVALID_ID) return effective_time_scale_buffer_id;
        return local_time_scale_buffer_id;
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
            const uint32_t child_buf_id = (effective_time_scale_buffer_id != INVALID_ID)
                ? effective_time_scale_buffer_id
                : local_time_scale_buffer_id;
            if (child_buf_id != INVALID_ID) {
                p_view.bind_secondary(p_context.get_grant_part(child_buf_id));
            }
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("effective_time_scale_buffer_id")) {
            effective_time_scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["effective_time_scale_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && effective_time_scale_buffer_id == INVALID_ID) {
            effective_time_scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("local_time_scale_buffer_id")) {
            local_time_scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["local_time_scale_buffer_id"]));
        }
        if (p_props.has("parent_entity_id_buffer_id")) {
            parent_entity_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_entity_id_buffer_id"]));
        }
        if (p_props.has("active_flags_buffer_id")) {
            active_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["active_flags_buffer_id"]));
        }
        if (p_props.has("archetype_mask_buffer_id")) {
            archetype_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["archetype_mask_buffer_id"]));
        }
        if (p_props.has("target_archetype_mask")) {
            target_archetype_mask = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_archetype_mask"]));
        }
        if (p_props.has("root_time_scale")) {
            root_time_scale = static_cast<float>(p_props["root_time_scale"]);
        }
        if (p_props.has("min_time_scale")) {
            min_time_scale = static_cast<float>(p_props["min_time_scale"]);
        }
        if (p_props.has("max_time_scale")) {
            max_time_scale = static_cast<float>(p_props["max_time_scale"]);
        }
        if (p_props.has("max_depth_passes")) {
            const int64_t passes = static_cast<int64_t>(p_props["max_depth_passes"]);
            max_depth_passes = static_cast<uint8_t>(std::clamp<int64_t>(passes, 1, 16));
        }
        if (p_props.has("zero_when_paused_or_disabled")) {
            zero_when_paused_or_disabled = static_cast<bool>(p_props["zero_when_paused_or_disabled"]);
        }
        if (p_props.has("clamp_time_scale")) {
            clamp_time_scale = static_cast<bool>(p_props["clamp_time_scale"]);
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
        if constexpr (std::is_same_v<T, float>) {
            const uint32_t eff_id = (effective_time_scale_buffer_id != INVALID_ID)
                ? effective_time_scale_buffer_id
                : local_time_scale_buffer_id;
            const GrantPartPOD* eff_part = (eff_id != INVALID_ID) ? p_context.get_grant_part(eff_id) : nullptr;
            if (!eff_part || !eff_part->raw_base_ptr) return;

            const GrantPartPOD* loc_part  = (local_time_scale_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(local_time_scale_buffer_id)
                : eff_part;
            const GrantPartPOD* par_part  = (parent_entity_id_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(parent_entity_id_buffer_id)
                : nullptr;
            const GrantPartPOD* flg_part  = (active_flags_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(active_flags_buffer_id)
                : nullptr;
            const GrantPartPOD* arch_part = (archetype_mask_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(archetype_mask_buffer_id)
                : nullptr;

            float* __restrict effective_scales      = reinterpret_cast<float*>(eff_part->raw_base_ptr);
            const float* __restrict local_scales    = (loc_part && loc_part->raw_base_ptr)
                ? reinterpret_cast<const float*>(loc_part->raw_base_ptr)
                : effective_scales;
            const uint32_t* __restrict parent_ids   = (par_part && par_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(par_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict active_flags = (flg_part && flg_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(flg_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict archetypes   = (arch_part && arch_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(arch_part->raw_base_ptr)
                : nullptr;

            const int64_t entity_count = _resolve_entity_count(eff_part, loc_part, par_part, flg_part, arch_part);
            if (entity_count <= 0) return;

            const float lo_bound = std::min(min_time_scale, max_time_scale);
            const float hi_bound = std::max(min_time_scale, max_time_scale);
            const uint8_t passes = (parent_ids && max_depth_passes > 1) ? max_depth_passes : 1;

            auto eval_entity = [&](int64_t idx, uint8_t pass_idx) noexcept {
                if (archetypes && ((archetypes[idx] & target_archetype_mask) == 0u)) {
                    return;
                }

                if (zero_when_paused_or_disabled && active_flags) {
                    const uint32_t flags = active_flags[idx];
                    const bool is_enabled = (flags & ENTITY_FLAG_ENABLED) != 0u;
                    const bool is_paused  = (flags & ENTITY_FLAG_PAUSED) != 0u;
                    if (!is_enabled || is_paused) {
                        effective_scales[idx] = 0.0f;
                        return;
                    }
                }

                // In in-place mode (when local_scales aliases effective_scales), only run 1 pass
                // so local_scales[idx] is not multiplied repeatedly by root/parent scales.
                const float local_ts = (local_scales != effective_scales || pass_idx == 0)
                    ? local_scales[idx]
                    : effective_scales[idx];

                float parent_ts = root_time_scale;
                if (parent_ids) {
                    const uint32_t raw_parent = parent_ids[idx];
                    const int64_t parent_idx  = static_cast<int64_t>(static_cast<int32_t>(raw_parent));
                    if (raw_parent != INVALID_ID &&
                        parent_idx >= 0 &&
                        parent_idx < entity_count &&
                        parent_idx != idx) {
                        parent_ts = effective_scales[parent_idx];
                    }
                }

                float resolved_ts = parent_ts * local_ts;
                if (clamp_time_scale) {
                    resolved_ts = std::clamp(resolved_ts, lo_bound, hi_bound);
                }
                effective_scales[idx] = resolved_ts;
            };

            const MemoryBufferSelectionPOD& sel = eff_part->selection;
            for (uint8_t p = 0; p < passes; ++p) {
                if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                    const uint64_t* bitset = sel.data.bitset;
                    const int64_t cap = std::min<int64_t>(sel.capacity, entity_count);
                    for (int64_t i = 0; i < cap; ++i) {
                        if (bitset[i >> 6] & (1ULL << (i & 63))) {
                            eval_entity(i, p);
                        }
                    }
                } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                    for (int64_t i = 0; i < sel.element_count; ++i) {
                        const int64_t idx = sel.data.indices[i];
                        if (idx >= 0 && idx < entity_count) {
                            eval_entity(idx, p);
                        }
                    }
                } else {
                    const int64_t start = std::max<int64_t>(0, sel.start_index);
                    const int64_t count = (sel.element_count > 0) ? sel.element_count : entity_count;
                    const int64_t end   = std::min<int64_t>(start + count, entity_count);
                    for (int64_t i = start; i < end; ++i) {
                        eval_entity(i, p);
                    }
                }
            }
        }
    }

private:
    [[nodiscard]] static inline int64_t _resolve_entity_count(
        const GrantPartPOD* p_eff,
        const GrantPartPOD* p_loc,
        const GrantPartPOD* p_par,
        const GrantPartPOD* p_flg,
        const GrantPartPOD* p_arch
    ) noexcept {
        int64_t count = static_cast<int64_t>(p_eff->capacity_bytes / sizeof(float));
        auto clamp_part = [&](const GrantPartPOD* part, size_t elem_size) noexcept {
            if (!part || !part->raw_base_ptr) return;
            const int64_t cap = static_cast<int64_t>(part->capacity_bytes / elem_size);
            if (cap > 0) count = std::min(count, cap);
        };
        clamp_part(p_loc,  sizeof(float));
        clamp_part(p_par,  sizeof(uint32_t));
        clamp_part(p_flg,  sizeof(uint32_t));
        clamp_part(p_arch, sizeof(uint32_t));
        return count;
    }
};

static_assert(sizeof(EntityTimeScaleCascadeTransformLogic<>) == 64, "EntityTimeScaleCascadeTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core