#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * EdgeSplitTearTransformLogic<T> (M28)
 * Evaluates edge stretch across indexed 3D triangle meshes against rest-pose lengths;
 * when relative stretch exceeds tear_threshold, claims a reserve vertex slot from
 * r_selection.unclaimed_mask (or spare tail capacity) and splits the shared vertex to tear the mesh.
 */
template <typename T = godot::Vector3>
struct alignas(64) EdgeSplitTearTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::INT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Edge Split & Cloth/Metal Tear";
    static constexpr std::string_view ui_category  = "Model/Destruction";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id    = INVALID_ID; // 4B: Primary target vertex buffer (Vector3)
    uint32_t index_buffer_id     = INVALID_ID; // 4B: Triangle index buffer (int32_t triplets)
    uint32_t rest_pose_buffer_id = INVALID_ID; // 4B: Optional rest-pose vertex buffer (Vector3)

    float tear_threshold            = 0.35f; // 4B: Relative edge strain threshold (L - L0) / L0
    float fallback_rest_edge_length = 0.25f; // 4B: Fallback rest length when rest_pose_buffer_id is unbound
    float tear_recoil_offset        = 0.01f; // 4B: Inward centroid recoil offset on newly split vertices (meters)
    int32_t max_splits_per_frame    = 64;    // 4B: Maximum topological vertex splits per frame
    uint8_t reserved_padding[36]    = {0};   // 36B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary vtx_prop;
        vtx_prop["name"] = "vertex_buffer_id";
        vtx_prop["type"] = godot::Variant::INT;
        vtx_prop["hint_string"] = "buffer_option";
        props.push_back(vtx_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary rest_prop;
        rest_prop["name"] = "rest_pose_buffer_id";
        rest_prop["type"] = godot::Variant::INT;
        rest_prop["hint_string"] = "buffer_option";
        props.push_back(rest_prop);

        godot::Dictionary thr_prop;
        thr_prop["name"] = "tear_threshold";
        thr_prop["type"] = godot::Variant::FLOAT;
        thr_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        thr_prop["hint_string"] = "0.01,5.0,0.01";
        props.push_back(thr_prop);

        godot::Dictionary fbk_prop;
        fbk_prop["name"] = "fallback_rest_edge_length";
        fbk_prop["type"] = godot::Variant::FLOAT;
        fbk_prop["hint_string"] = "suffix:m";
        props.push_back(fbk_prop);

        godot::Dictionary rec_prop;
        rec_prop["name"] = "tear_recoil_offset";
        rec_prop["type"] = godot::Variant::FLOAT;
        rec_prop["hint_string"] = "suffix:m";
        props.push_back(rec_prop);

        godot::Dictionary max_prop;
        max_prop["name"] = "max_splits_per_frame";
        max_prop["type"] = godot::Variant::INT;
        max_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        max_prop["hint_string"] = "1,1024,1";
        props.push_back(max_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (vertex_buffer_id != INVALID_ID) ? vertex_buffer_id : index_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("rest_pose_buffer_id")) {
            rest_pose_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rest_pose_buffer_id"]));
        }
        if (p_props.has("tear_threshold")) {
            tear_threshold = std::max(1e-4f, static_cast<float>(p_props["tear_threshold"]));
        }
        if (p_props.has("fallback_rest_edge_length")) {
            fallback_rest_edge_length = std::max(1e-5f, static_cast<float>(p_props["fallback_rest_edge_length"]));
        }
        if (p_props.has("tear_recoil_offset")) {
            tear_recoil_offset = std::max(0.0f, static_cast<float>(p_props["tear_recoil_offset"]));
        }
        if (p_props.has("max_splits_per_frame")) {
            max_splits_per_frame = std::max<int32_t>(1, static_cast<int32_t>(p_props["max_splits_per_frame"]));
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
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, int32_t>) {
            GrantPartPOD* vtx_part = const_cast<GrantPartPOD*>(p_context.get_grant_part(vertex_buffer_id));
            GrantPartPOD* idx_part = const_cast<GrantPartPOD*>(p_context.get_grant_part(index_buffer_id));
            if (!vtx_part || !idx_part || !vtx_part->raw_base_ptr || !idx_part->raw_base_ptr) return;

            const GrantPartPOD* rest_part = (rest_pose_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(rest_pose_buffer_id)
                : nullptr;

            godot::Vector3* __restrict verts = reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr);
            const int32_t max_verts = static_cast<int32_t>(vtx_part->capacity_bytes / sizeof(godot::Vector3));
            int32_t cur_verts = (vtx_part->selection.element_count > 0)
                ? std::min<int32_t>(static_cast<int32_t>(vtx_part->selection.element_count), max_verts)
                : max_verts;

            const godot::Vector3* __restrict rest_verts = (rest_part && rest_part->raw_base_ptr && rest_part->capacity_bytes >= sizeof(godot::Vector3))
                ? reinterpret_cast<const godot::Vector3*>(rest_part->raw_base_ptr)
                : nullptr;
            const int32_t max_rest = rest_verts ? static_cast<int32_t>(rest_part->capacity_bytes / sizeof(godot::Vector3)) : 0;

            int32_t* __restrict indices = reinterpret_cast<int32_t*>(idx_part->raw_base_ptr);
            const int32_t n_indices = (idx_part->selection.element_count > 0)
                ? static_cast<int32_t>(idx_part->selection.element_count)
                : static_cast<int32_t>(idx_part->capacity_bytes / sizeof(int32_t));
            const int32_t n_tris = n_indices / 3;

            int32_t splits_done = 0;

            for (int32_t t = 0; t < n_tris && splits_done < max_splits_per_frame; ++t) {
                int32_t* tri = indices + t * 3;
                const int32_t i0 = tri[0], i1 = tri[1], i2 = tri[2];
                if (i0 < 0 || i0 >= cur_verts || i1 < 0 || i1 >= cur_verts || i2 < 0 || i2 >= cur_verts) continue;

                const godot::Vector3 centroid = (verts[i0] + verts[i1] + verts[i2]) * (1.0f / 3.0f);

                for (int e = 0; e < 3 && splits_done < max_splits_per_frame; ++e) {
                    const int32_t a = tri[e];
                    const int32_t b = tri[(e + 1) % 3];
                    const float cur_len = (verts[b] - verts[a]).length();

                    float rest_len = fallback_rest_edge_length;
                    if (rest_verts && a < max_rest && b < max_rest) {
                        rest_len = std::max(1e-5f, (rest_verts[b] - rest_verts[a]).length());
                    }

                    const float strain = (cur_len - rest_len) / rest_len;
                    if (strain > tear_threshold) {
                        const int32_t new_slot = _claim_unclaimed_slot(vtx_part->selection, cur_verts, max_verts);
                        if (new_slot < 0) break;

                        const godot::Vector3 recoil_dir = (centroid - verts[b]).normalized();
                        verts[new_slot] = verts[b] + recoil_dir * tear_recoil_offset;
                        if (vtx_part->selection.mode == SelectionMode::DENSE && vtx_part->selection.data.bitset && new_slot < vtx_part->selection.capacity) {
                            vtx_part->selection.data.bitset[new_slot >> 6] |= (1ULL << (new_slot & 63));
                        }
                        tri[(e + 1) % 3] = new_slot;
                        ++splits_done;
                    }
                }
            }

            vtx_part->selection.element_count = cur_verts;
        }
    }

private:
    static inline int32_t _claim_unclaimed_slot(MemoryBufferSelectionPOD& r_sel, int32_t& io_cur_verts, int32_t p_max_verts) noexcept {
        if (r_sel.unclaimed_mask && r_sel.capacity > 0) {
            const int32_t words = static_cast<int32_t>((r_sel.capacity + 63) / 64);
            const int32_t start_word = io_cur_verts / 64;
            for (int32_t w = start_word; w < words; ++w) {
                const uint64_t mask = r_sel.unclaimed_mask[w];
                if (mask == 0ULL) continue;
                const int32_t bit = std::countr_zero(mask);
                const int32_t slot = w * 64 + bit;
                if (slot >= io_cur_verts && slot < p_max_verts) {
                    r_sel.unclaimed_mask[w] &= ~(1ULL << bit);
                    io_cur_verts = std::max(io_cur_verts, slot + 1);
                    return slot;
                }
            }
        }
        if (io_cur_verts < p_max_verts) {
            return io_cur_verts++;
        }
        return -1;
    }
};

static_assert(sizeof(EdgeSplitTearTransformLogic<godot::Vector3>) == 64, "EdgeSplitTearTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<EdgeSplitTearTransformLogic<godot::Vector3>>, "EdgeSplitTearTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core