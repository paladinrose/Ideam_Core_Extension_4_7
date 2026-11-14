#pragma once

/**
 * parametric_assembly_morph_metadata_logic.h
 * 64-byte cache-aligned MetadataLogic task struct (`DSG4`) for `IdeamDesign`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.1 — `DSG4`)
 * ============================================================================
 * Evaluates live assembly morph parameters (`dream_morph_factor`, `seat_height_override`,
 * and physics caster-wheel joint activation) over `StructureNodePod` and `SocketPod`
 * buffers and stamps `DesignCascadeLayer` and `SemanticRoleCategory` flags into
 * `r_selection.lod_levels` and `r_selection.group_masks`.
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include "../../../../design/design_common.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) ParametricAssemblyMorphMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32;

    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Design Parametric Assembly & Dream Morph Metadata";
    static constexpr std::string_view ui_category  = "Design";

    uint32_t target_buffer_id      = INVALID_ID; ///< `STRUCTURE_NODE_POD` (`StructureNodePod`)
    uint32_t socket_buffer_id      = INVALID_ID; ///< `SOCKET_POD` (`SocketPod`)
    uint32_t status_mask_buffer_id = INVALID_ID; ///< `NODE_STATUS_MASK` (`uint32_t`)
    float    dream_morph_factor    = 0.0f;       ///< `[0.0 .. 1.0]`
    float    seat_height_override  = -1.0f;      ///< `< 0.0` = keep existing
    uint8_t  reserved_padding[44]  = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",      godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("socket_buffer_id",      godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("status_mask_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("dream_morph_factor",    godot::Variant::FLOAT, 0.0);
        add_prop("seat_height_override",  godot::Variant::FLOAT, -1.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))      target_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("socket_buffer_id"))      socket_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["socket_buffer_id"]));
        if (p_props.has("status_mask_buffer_id")) status_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["status_mask_buffer_id"]));
        if (p_props.has("dream_morph_factor"))    dream_morph_factor    = static_cast<float>(static_cast<double>(p_props["dream_morph_factor"]));
        if (p_props.has("seat_height_override"))  seat_height_override  = static_cast<float>(static_cast<double>(p_props["seat_height_override"]));
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
                    uint8_t *raw = buf->master_block_ptr + buf->memory_offset;
                    r_cap = static_cast<size_t>(std::max<int64_t>(0, buf->current_count > 0 ? buf->current_count : buf->max_elements));
                    return reinterpret_cast<T_Elem *>(raw);
                }
            }
        }
        return nullptr;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t node_cap = 0, sock_cap = 0, mask_cap = 0;
        auto *nodes        = _resolve_ptr<StructureNodePod>(p_ctx, target_buffer_id, node_cap);
        auto *sockets      = _resolve_ptr<SocketPod>(p_ctx, socket_buffer_id, sock_cap);
        auto *status_masks = _resolve_ptr<uint32_t>(p_ctx, status_mask_buffer_id, mask_cap);
        if (!nodes || node_cap == 0u) return;

        const float morph = std::clamp(dream_morph_factor, 0.0f, 1.0f);
        for (size_t i = 0; i < node_cap; ++i) {
            StructureNodePod &node = nodes[i];
            if (morph > 0.0f) {
                const float phase      = static_cast<float>(i) * 1.15f;
                const float scale_wave = 1.0f + morph * 0.20f * std::sin(phase);
                node.rest_obb_extents = node.rest_obb_extents * scale_wave;
                node.cascade_layer     = DesignCascadeLayer::LAYER_DREAM_SURREALISM;
            }
            if (seat_height_override > 0.0f &&
                match_semantic_role_hierarchy(SEMANTIC_ROLE_SURFACE_SEAT, node.semantic_tag_id) > 0u) {
                node.rest_obb_center.y = seat_height_override;
            }
            if (status_masks && i < mask_cap) {
                status_masks[i] = (node.semantic_tag_id >> 24) & 0xFFu;
            }
        }

        if (sockets) {
            for (size_t i = 0; i < sock_cap; ++i) {
                if (sockets[i].physics_binding_mode == SocketPhysicsBindingMode::PHYSICS_BIND_PIN_JOINT_3D) {
                    sockets[i].is_occupied = 1u;
                }
            }
        }

        if (GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(target_buffer_id))) {
            MemoryBufferSelectionPOD &sel = target_part->selection;
            const size_t sel_lim = static_cast<size_t>(std::max<int64_t>(0, sel.element_count));
            for (size_t i = 0; i < node_cap && i < sel_lim; ++i) {
                if (sel.lod_levels) {
                    sel.lod_levels[i] = static_cast<uint8_t>(nodes[i].cascade_layer);
                }
                if (sel.group_masks) {
                    sel.group_masks[i] = (nodes[i].semantic_tag_id >> 24) & 0xFFu;
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

static_assert(sizeof(ParametricAssemblyMorphMetadataLogic<>) == 64, "ParametricAssemblyMorphMetadataLogic must be 64 bytes!");
static_assert(IsMetadataLogic<ParametricAssemblyMorphMetadataLogic<>>, "ParametricAssemblyMorphMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
