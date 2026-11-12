#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * MeshEdgeStrainQueryLogic<T, T_Strategy, T_View> (M40)
 * Evaluates relative edge strain |L_current - L_rest| / L_rest across deformed and rest-pose
 * 3D vertex buffers (and optional triangle index buffer), culling or adding over-strained vertices.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SwapView
>
struct alignas(64) MeshEdgeStrainQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities =
        ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS | ViewCapability::SWAP_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA |
        BufferLayoutType::SPARSE_SET | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types =
        DataType::VECTOR3 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Mesh Edge Strain Threshold Query";
    static constexpr std::string_view ui_category  = "Model/Query";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id      = INVALID_ID; // 4B: Current deformed vertex buffer
    uint32_t rest_vertex_buffer_id = INVALID_ID; // 4B: Rest-pose vertex buffer
    uint32_t index_buffer_id       = INVALID_ID; // 4B: Optional triangle index buffer
    float strain_threshold         = 0.25f;      // 4B -> 16B
    float fallback_rest_length     = 0.25f;      // 4B
    bool evaluate_compression      = false;      // 1B
    uint8_t reserved_padding[43]   = {0};        // 43B -> Total = 64B

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
        add_buf("rest_vertex_buffer_id");
        add_buf("index_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("strain_threshold", godot::Variant::FLOAT, 0.25f);
        add_prop("fallback_rest_length", godot::Variant::FLOAT, 0.25f);
        add_prop("evaluate_compression", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("rest_vertex_buffer_id")) rest_vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rest_vertex_buffer_id"]));
        if (p_props.has("index_buffer_id")) index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        if (p_props.has("strain_threshold")) strain_threshold = std::max(1e-5f, static_cast<float>(p_props["strain_threshold"]));
        if (p_props.has("fallback_rest_length")) fallback_rest_length = std::max(1e-5f, static_cast<float>(p_props["fallback_rest_length"]));
        if (p_props.has("evaluate_compression")) evaluate_compression = static_cast<bool>(p_props["evaluate_compression"]);
    }

    template <QueryOp Op>
    static inline void _apply_op(MemoryBufferSelectionPOD& r_sel, const TaskContextPOD& p_ctx, uint32_t p_buf_id, int64_t idx) noexcept {
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
            const GrantPartPOD* cur_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            const GrantPartPOD* rest_part = (rest_vertex_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(rest_vertex_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!cur_part || !cur_part->raw_base_ptr || !rest_part || !rest_part->raw_base_ptr) return;

            const GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(index_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : nullptr);

            const godot::Vector3* cur_v = reinterpret_cast<const godot::Vector3*>(cur_part->raw_base_ptr);
            const godot::Vector3* rest_v = reinterpret_cast<const godot::Vector3*>(rest_part->raw_base_ptr);
            const int32_t n_verts = std::min<int32_t>(
                static_cast<int32_t>(cur_part->capacity_bytes / sizeof(godot::Vector3)),
                static_cast<int32_t>(rest_part->capacity_bytes / sizeof(godot::Vector3))
            );
            if (n_verts <= 0) return;

            if (idx_part && idx_part->raw_base_ptr) {
                const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
                const int32_t n_tris = static_cast<int32_t>(((idx_part->selection.element_count > 0)
                    ? idx_part->selection.element_count
                    : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t))) / 3);

                for (int32_t t = 0; t < n_tris; ++t) {
                    const int32_t tri[3] = { indices[t * 3 + 0], indices[t * 3 + 1], indices[t * 3 + 2] };
                    if (tri[0] < 0 || tri[0] >= n_verts || tri[1] < 0 || tri[1] >= n_verts || tri[2] < 0 || tri[2] >= n_verts) continue;

                    for (int e = 0; e < 3; ++e) {
                        const int32_t a = tri[e];
                        const int32_t b = tri[(e + 1) % 3];
                        const float l_cur = (cur_v[b] - cur_v[a]).length();
                        const float l_rest = std::max(1e-5f, (rest_v[b] - rest_v[a]).length());
                        const float strain = evaluate_compression
                            ? (std::abs(l_cur - l_rest) / l_rest)
                            : ((l_cur - l_rest) / l_rest);
                        if (strain >= strain_threshold) {
                            _apply_op<Op>(r_selection, p_ctx, target_buffer_id, a);
                            _apply_op<Op>(r_selection, p_ctx, target_buffer_id, b);
                        }
                    }
                }
            } else {
                const float inv_rest = 1.0f / std::max(1e-5f, fallback_rest_length);
                for (int32_t i = 0; i < n_verts; ++i) {
                    const float disp_strain = (cur_v[i] - rest_v[i]).length() * inv_rest;
                    if (disp_strain >= strain_threshold) {
                        _apply_op<Op>(r_selection, p_ctx, target_buffer_id, i);
                    }
                }
            }
        }
    }
};

static_assert(sizeof(MeshEdgeStrainQueryLogic<>) == 64, "MeshEdgeStrainQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<MeshEdgeStrainQueryLogic<>>, "MeshEdgeStrainQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core