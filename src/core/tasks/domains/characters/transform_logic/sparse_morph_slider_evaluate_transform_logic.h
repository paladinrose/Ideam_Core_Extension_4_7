#pragma once

/**
 * sparse_morph_slider_evaluate_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`CHR2`) for `IdeamCharacters`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.2 — `CHR2`)
 * ============================================================================
 * Applies `MorphDeltaPod` sparse-set displacements across anatomical partitions
 * (`PARTITION_TORSO`, `PARTITION_HEAD`, `PARTITION_DIGIT_0..4`, etc.) weighted by
 * `MORPH_WEIGHT_FLOAT32` (including direct OpenXR trigger/grip analog inputs),
 * updating `DEFORMED_POSITION_VEC3` and re-normalizing `DEFORMED_NORMAL_VEC3`.
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
struct alignas(64) SparseMorphSliderEvaluateTransformLogic {
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

    static constexpr std::string_view display_name = "Character Sparse Anatomical Morph Slider Evaluate";
    static constexpr std::string_view ui_category  = "Characters";

    uint32_t target_buffer_id          = INVALID_ID;         ///< `DEFORMED_POSITION_VEC3` (`glm::vec3`)
    uint32_t deformed_normal_buffer_id = INVALID_ID;         ///< `DEFORMED_NORMAL_VEC3` (`glm::vec3`)
    uint32_t quad_vertex_buffer_id     = INVALID_ID;         ///< `QUAD_LOOP_VERTEX_POD` (`QuadLoopVertexPod`)
    uint32_t morph_delta_buffer_id     = INVALID_ID;         ///< `MORPH_DELTA_POD` (`MorphDeltaPod`)
    uint32_t morph_weight_buffer_id    = INVALID_ID;         ///< `MORPH_WEIGHT_FLOAT32` (`float`)
    uint32_t active_partition_mask     = PARTITION_MASK_ALL; ///< Bitmask of `AnatomicalPartitionId`s
    float    global_morph_multiplier   = 1.0f;
    uint8_t  reserved_padding[36]      = {0};                ///< 36B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",          godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("deformed_normal_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("quad_vertex_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("morph_delta_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("morph_weight_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("active_partition_mask",     godot::Variant::INT,   static_cast<int64_t>(PARTITION_MASK_ALL));
        add_prop("global_morph_multiplier",   godot::Variant::FLOAT, 1.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : morph_delta_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))          target_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("deformed_normal_buffer_id")) deformed_normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["deformed_normal_buffer_id"]));
        if (p_props.has("quad_vertex_buffer_id"))     quad_vertex_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["quad_vertex_buffer_id"]));
        if (p_props.has("morph_delta_buffer_id"))     morph_delta_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["morph_delta_buffer_id"]));
        if (p_props.has("morph_weight_buffer_id"))    morph_weight_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["morph_weight_buffer_id"]));
        if (p_props.has("active_partition_mask"))     active_partition_mask     = static_cast<uint32_t>(static_cast<int64_t>(p_props["active_partition_mask"]));
        if (p_props.has("global_morph_multiplier"))   global_morph_multiplier   = static_cast<float>(static_cast<double>(p_props["global_morph_multiplier"]));
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
        size_t pos_cap = 0, nrm_cap = 0, v_cap = 0, d_cap = 0, w_cap = 0;
        auto *positions     = _resolve_ptr<glm::vec3>(p_ctx, target_buffer_id, pos_cap);
        auto *normals       = _resolve_ptr<glm::vec3>(p_ctx, deformed_normal_buffer_id, nrm_cap);
        const auto *verts   = _resolve_ptr<QuadLoopVertexPod>(p_ctx, quad_vertex_buffer_id, v_cap);
        const auto *deltas  = _resolve_ptr<MorphDeltaPod>(p_ctx, morph_delta_buffer_id, d_cap);
        const auto *weights = _resolve_ptr<float>(p_ctx, morph_weight_buffer_id, w_cap);

        if (!positions || pos_cap == 0u || !verts || v_cap == 0u || !deltas || d_cap == 0u || !weights || w_cap == 0u) {
            return;
        }

        // Apply optional global_morph_multiplier via stack buffer for up to 64 channels
        float scaled_weights[64] = {0.0f};
        const size_t n_channels  = std::min<size_t>(w_cap, 64u);
        for (size_t c = 0; c < n_channels; ++c) {
            scaled_weights[c] = weights[c] * global_morph_multiplier;
        }

        ParametricCharacterMesh::evaluate_sparse_morph_sliders(
            std::span<const QuadLoopVertexPod>(verts, v_cap),
            std::span<const MorphDeltaPod>(deltas, d_cap),
            std::span<const float>(scaled_weights, n_channels),
            active_partition_mask,
            positions,
            normals,
            pos_cap
        );
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(SparseMorphSliderEvaluateTransformLogic<>) == 64, "SparseMorphSliderEvaluateTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<SparseMorphSliderEvaluateTransformLogic<>>, "SparseMorphSliderEvaluateTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
