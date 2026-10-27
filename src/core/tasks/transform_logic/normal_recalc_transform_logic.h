#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/vector3.hpp>
#include <cstring>
#include <string_view>

namespace ideam::core {

/**
 * NormalRecalcTransformLogic<T>
 * Recomputes area-weighted smooth vertex normals from a Vector3 vertex buffer
 * and an int32_t triangle index buffer.
 * Notice: Cross-product magnitude is 2x triangle area, yielding exact area-weighting
 * without any per-face square root overhead.
 */
template <typename T = godot::Vector3>
struct alignas(64) NormalRecalcTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::FLAT | BufferLayoutType::SOA;
    static constexpr DataType required_types              = DataType::VECTOR3;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Normal Recalc";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t normal_buffer_id = INVALID_ID; // Primary target buffer
    uint32_t vertex_buffer_id = INVALID_ID; // Source deformed positions
    uint32_t index_buffer_id  = INVALID_ID; // Triangle indices (int32_t triplets)
    bool invert_winding       = false;
    uint8_t reserved_padding[51] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary norm_prop;
        norm_prop["name"] = "normal_buffer_id";
        norm_prop["type"] = godot::Variant::INT;
        norm_prop["hint_string"] = "buffer_option";
        props.push_back(norm_prop);

        godot::Dictionary vert_prop;
        vert_prop["name"] = "vertex_buffer_id";
        vert_prop["type"] = godot::Variant::INT;
        vert_prop["hint_string"] = "buffer_option";
        props.push_back(vert_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary inv_prop;
        inv_prop["name"] = "invert_winding";
        inv_prop["type"] = godot::Variant::BOOL;
        props.push_back(inv_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return normal_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("normal_buffer_id")) {
            normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        }
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        }
        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("invert_winding")) {
            invert_winding = static_cast<bool>(p_props["invert_winding"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_normal_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
        const GrantPartPOD* norm_part = p_context.get_grant_part(normal_buffer_id);
        const GrantPartPOD* vert_part = p_context.get_grant_part(vertex_buffer_id);
        const GrantPartPOD* idx_part  = p_context.get_grant_part(index_buffer_id);
        if (!norm_part || !vert_part || !idx_part) return;

        T* __restrict normals        = reinterpret_cast<T*>(norm_part->raw_base_ptr);
        const T* __restrict vertices = reinterpret_cast<const T*>(vert_part->raw_base_ptr);
        const int32_t* __restrict indices = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
        if (!normals || !vertices || !indices) return;

        const int64_t vert_count = vert_part->selection.capacity;
        const int64_t idx_count  = idx_part->selection.element_count > 0
            ? idx_part->selection.element_count
            : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t));

        // 1. Zero-initialize accumulator buffer
        std::memset(normals, 0, static_cast<size_t>(vert_count) * sizeof(T));

        // 2. Accumulate area-weighted face normals across triangles
        const float winding_sign = invert_winding ? -1.0f : 1.0f;
        for (int64_t t = 0; t + 2 < idx_count; t += 3) {
            const int32_t i0 = indices[t];
            const int32_t i1 = indices[t + 1];
            const int32_t i2 = indices[t + 2];

            if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= vert_count || i1 >= vert_count || i2 >= vert_count) continue;

            const T e1 = vertices[i1] - vertices[i0];
            const T e2 = vertices[i2] - vertices[i0];
            // Godot uses clockwise/counter-clockwise convention: (e2 x e1) or (e1 x e2)
            const T face_n = e2.cross(e1) * winding_sign;

            normals[i0] += face_n;
            normals[i1] += face_n;
            normals[i2] += face_n;
        }

        // 3. Linear SIMD-friendly normalization pass
        for (int64_t v = 0; v < vert_count; ++v) {
            const float len_sq = normals[v].length_squared();
            if (len_sq > 1e-12f) {
                normals[v] *= (1.0f / std::sqrt(len_sq));
            } else {
                normals[v] = T(0.0f, 1.0f, 0.0f);
            }
        }
        }
    }
};

static_assert(sizeof(NormalRecalcTransformLogic<godot::Vector3>) == 64, "NormalRecalcTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core
