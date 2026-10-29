#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/vector3.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * FaceCenterAndAreaTransformLogic<T>
 * Streams Vertex (Vector3) and Index (int32_t, stride = 3) buffers to compute
 * per-triangle centroids (Vector3), unit face normals (Vector3), and surface areas (float).
 */
template <typename T = godot::Vector3>
struct alignas(64) FaceCenterAndAreaTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::FLAT | BufferLayoutType::SOA;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Face Center & Area";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t face_center_buffer_id = INVALID_ID; // Optional/Primary Vector3 per-face centroid output
    uint32_t face_normal_buffer_id = INVALID_ID; // Optional Vector3 per-face unit normal output
    uint32_t face_area_buffer_id   = INVALID_ID; // Optional float per-face surface area output
    uint32_t vertex_buffer_id      = INVALID_ID; // Source Vector3 vertex positions
    uint32_t index_buffer_id       = INVALID_ID; // Source int32_t triangle indices (optional if unindexed triangle soup)
    bool invert_winding            = false;
    uint8_t reserved_padding[43]   = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary ctr_prop;
        ctr_prop["name"] = "face_center_buffer_id";
        ctr_prop["type"] = godot::Variant::INT;
        ctr_prop["hint_string"] = "buffer_option";
        props.push_back(ctr_prop);

        godot::Dictionary norm_prop;
        norm_prop["name"] = "face_normal_buffer_id";
        norm_prop["type"] = godot::Variant::INT;
        norm_prop["hint_string"] = "buffer_option";
        props.push_back(norm_prop);

        godot::Dictionary area_prop;
        area_prop["name"] = "face_area_buffer_id";
        area_prop["type"] = godot::Variant::INT;
        area_prop["hint_string"] = "buffer_option";
        props.push_back(area_prop);

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
        inv_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(inv_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if constexpr (std::is_same_v<T, float>) {
            if (face_area_buffer_id != INVALID_ID) return face_area_buffer_id;
        }
        if (face_center_buffer_id != INVALID_ID) return face_center_buffer_id;
        if (face_normal_buffer_id != INVALID_ID) return face_normal_buffer_id;
        return face_area_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("face_center_buffer_id")) {
            face_center_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["face_center_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && face_center_buffer_id == INVALID_ID) {
            face_center_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("face_normal_buffer_id")) {
            face_normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["face_normal_buffer_id"]));
        }
        if (p_props.has("face_area_buffer_id")) {
            face_area_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["face_area_buffer_id"]));
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
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, float>) {
            const GrantPartPOD* vert_part = p_context.get_grant_part(vertex_buffer_id);
            if (!vert_part || !vert_part->raw_base_ptr) return;

            const godot::Vector3* __restrict vertices = reinterpret_cast<const godot::Vector3*>(vert_part->raw_base_ptr);
            const int64_t vert_count = vert_part->selection.capacity;
            if (vert_count < 3) return;

            const GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID) ? p_context.get_grant_part(index_buffer_id) : nullptr;
            const int32_t* __restrict indices = (idx_part && idx_part->raw_base_ptr)
                ? reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr)
                : nullptr;

            int64_t face_count = 0;
            if (indices) {
                const int64_t idx_count = idx_part->selection.element_count > 0
                    ? idx_part->selection.element_count
                    : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t));
                face_count = idx_count / 3;
            } else {
                const int64_t active_verts = vert_part->selection.element_count > 0
                    ? vert_part->selection.element_count
                    : vert_count;
                face_count = active_verts / 3;
            }
            if (face_count <= 0) return;

            godot::Vector3* __restrict out_centers = nullptr;
            if (face_center_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* ctr_part = p_context.get_grant_part(face_center_buffer_id)) {
                    out_centers = reinterpret_cast<godot::Vector3*>(ctr_part->raw_base_ptr);
                    face_count = std::min<int64_t>(face_count, static_cast<int64_t>(ctr_part->capacity_bytes / sizeof(godot::Vector3)));
                }
            }

            godot::Vector3* __restrict out_normals = nullptr;
            if (face_normal_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* norm_part = p_context.get_grant_part(face_normal_buffer_id)) {
                    out_normals = reinterpret_cast<godot::Vector3*>(norm_part->raw_base_ptr);
                    face_count = std::min<int64_t>(face_count, static_cast<int64_t>(norm_part->capacity_bytes / sizeof(godot::Vector3)));
                }
            }

            float* __restrict out_areas = nullptr;
            if (face_area_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* area_part = p_context.get_grant_part(face_area_buffer_id)) {
                    out_areas = reinterpret_cast<float*>(area_part->raw_base_ptr);
                    face_count = std::min<int64_t>(face_count, static_cast<int64_t>(area_part->capacity_bytes / sizeof(float)));
                }
            }

            if (!out_centers && !out_normals && !out_areas) return;

            constexpr float one_third = 1.0f / 3.0f;
            const float winding_sign  = invert_winding ? -1.0f : 1.0f;

            for (int64_t f = 0; f < face_count; ++f) {
                const int64_t base_idx = f * 3;
                const int32_t i0 = indices ? indices[base_idx]     : static_cast<int32_t>(base_idx);
                const int32_t i1 = indices ? indices[base_idx + 1] : static_cast<int32_t>(base_idx + 1);
                const int32_t i2 = indices ? indices[base_idx + 2] : static_cast<int32_t>(base_idx + 2);

                if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= vert_count || i1 >= vert_count || i2 >= vert_count) {
                    if (out_centers) out_centers[f] = godot::Vector3(0.0f, 0.0f, 0.0f);
                    if (out_normals) out_normals[f] = godot::Vector3(0.0f, 1.0f, 0.0f);
                    if (out_areas)   out_areas[f]   = 0.0f;
                    continue;
                }

                const godot::Vector3& v0 = vertices[i0];
                const godot::Vector3& v1 = vertices[i1];
                const godot::Vector3& v2 = vertices[i2];

                if (out_centers) {
                    out_centers[f] = (v0 + v1 + v2) * one_third;
                }

                if (out_normals || out_areas) {
                    const godot::Vector3 e1 = v1 - v0;
                    const godot::Vector3 e2 = v2 - v0;
                    const godot::Vector3 cross_vec = e2.cross(e1) * winding_sign;
                    const float cross_len = std::sqrt(cross_vec.length_squared());

                    if (out_areas) {
                        out_areas[f] = 0.5f * cross_len;
                    }
                    if (out_normals) {
                        out_normals[f] = (cross_len > 1e-12f)
                            ? (cross_vec * (1.0f / cross_len))
                            : godot::Vector3(0.0f, 1.0f, 0.0f);
                    }
                }
            }
        }
    }
};

static_assert(sizeof(FaceCenterAndAreaTransformLogic<>) == 64, "FaceCenterAndAreaTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core