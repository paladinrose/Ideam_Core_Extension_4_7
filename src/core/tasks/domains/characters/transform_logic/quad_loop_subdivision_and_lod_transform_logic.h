#pragma once

/**
 * quad_loop_subdivision_and_lod_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`CHR1`) for `IdeamCharacters`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.2 — `CHR1`)
 * ============================================================================
 * Evaluates `QuadLoopVertexPod::edge_ring_mask` (`r_selection.group_masks`),
 * `partition_id` (`r_selection.partition_ids`), and `min_lod_level`
 * (`r_selection.lod_levels`) for deterministic quad-ring LOD compaction/subdivision,
 * initializing `DEFORMED_POSITION_VEC3`, `DEFORMED_NORMAL_VEC3`, and `VERTEX_ACTIVE_LOD_MASK`.
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../../characters/parametric_character_mesh.h"
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
struct alignas(64) QuadLoopSubdivisionAndLODTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32 | DataType::VECTOR3;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Character Quad-Loop Subdivision & LOD";
    static constexpr std::string_view ui_category  = "Characters";

    uint32_t target_buffer_id            = INVALID_ID; ///< `DEFORMED_POSITION_VEC3` (`glm::vec3`)
    uint32_t quad_vertex_buffer_id       = INVALID_ID; ///< `QUAD_LOOP_VERTEX_POD` (`QuadLoopVertexPod`)
    uint32_t quad_face_buffer_id         = INVALID_ID; ///< `QUAD_FACE_POD` (`QuadFacePod`)
    uint32_t deformed_normal_buffer_id   = INVALID_ID; ///< `DEFORMED_NORMAL_VEC3` (`glm::vec3`)
    uint32_t active_lod_mask_buffer_id   = INVALID_ID; ///< `VERTEX_ACTIVE_LOD_MASK` (`uint32_t`)
    uint32_t target_lod_level            = 1u;         ///< `0..3`
    uint64_t active_edge_ring_mask       = 0ULL;       ///< `0` = all rings active, else bitmask filter
    uint8_t  reserved_padding[32]        = {0};        ///< 32B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",          godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("quad_vertex_buffer_id",     godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("quad_face_buffer_id",       godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("deformed_normal_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("active_lod_mask_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("target_lod_level",          godot::Variant::INT, static_cast<int64_t>(1));
        add_prop("active_edge_ring_mask",     godot::Variant::INT, static_cast<int64_t>(0));
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : quad_vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))          target_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("quad_vertex_buffer_id"))     quad_vertex_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["quad_vertex_buffer_id"]));
        if (p_props.has("quad_face_buffer_id"))       quad_face_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["quad_face_buffer_id"]));
        if (p_props.has("deformed_normal_buffer_id")) deformed_normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["deformed_normal_buffer_id"]));
        if (p_props.has("active_lod_mask_buffer_id")) active_lod_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["active_lod_mask_buffer_id"]));
        if (p_props.has("target_lod_level"))          target_lod_level          = static_cast<uint32_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["target_lod_level"]), 0, 3));
        if (p_props.has("active_edge_ring_mask"))     active_edge_ring_mask     = static_cast<uint64_t>(static_cast<int64_t>(p_props["active_edge_ring_mask"]));
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
        size_t v_cap = 0, q_cap = 0, pos_cap = 0, nrm_cap = 0, mask_cap = 0;
        const auto *verts = _resolve_ptr<QuadLoopVertexPod>(p_ctx, quad_vertex_buffer_id, v_cap);
        const auto *quads = _resolve_ptr<QuadFacePod>(p_ctx, quad_face_buffer_id, q_cap);
        auto *out_pos     = _resolve_ptr<glm::vec3>(p_ctx, target_buffer_id, pos_cap);
        auto *out_nrm     = _resolve_ptr<glm::vec3>(p_ctx, deformed_normal_buffer_id, nrm_cap);
        auto *out_masks   = _resolve_ptr<uint32_t>(p_ctx, active_lod_mask_buffer_id, mask_cap);

        if (!verts || v_cap == 0u || !out_pos || pos_cap == 0u) return;

        const size_t cap = std::min<size_t>(pos_cap, nrm_cap > 0u ? nrm_cap : pos_cap);
        ParametricCharacterMesh::evaluate_quad_loop_lod(
            std::span<const QuadLoopVertexPod>(verts, v_cap),
            quads ? std::span<const QuadFacePod>(quads, q_cap) : std::span<const QuadFacePod>{},
            static_cast<uint8_t>(std::min<uint32_t>(target_lod_level, 3u)),
            active_edge_ring_mask,
            out_pos,
            out_nrm,
            out_masks,
            cap
        );

        // Stamp selection metadata (`lod_levels`, `group_masks`, `partition_ids`) if present on grant part
        if (GrantPartPOD *part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(get_target_buffer_id()))) {
            MemoryBufferSelectionPOD &sel = part->selection;
            const size_t sel_lim = static_cast<size_t>(std::max<int64_t>(0, sel.element_count));
            for (size_t i = 0; i < v_cap && i < sel_lim; ++i) {
                if (sel.lod_levels) {
                    sel.lod_levels[i] = verts[i].min_lod_level;
                }
                if (sel.group_masks) {
                    sel.group_masks[i] = static_cast<uint32_t>(verts[i].edge_ring_mask & 0xFFFFFFFFULL);
                }
                if (sel.partition_ids) {
                    sel.partition_ids[i] = verts[i].partition_id;
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(QuadLoopSubdivisionAndLODTransformLogic<>) == 64, "QuadLoopSubdivisionAndLODTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<QuadLoopSubdivisionAndLODTransformLogic<>>, "QuadLoopSubdivisionAndLODTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
