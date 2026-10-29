#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * TangentRecalcTransformLogic<T>
 * Recomputes orthonormal vertex tangents (godot::Vector4(tx, ty, tz, binormal_sign))
 * using Lengyel / MikkTSpace-Lite triangle UV gradient equations and Gram-Schmidt
 * orthogonalization against the vertex normal.
 */
template <typename T = godot::Vector4>
struct alignas(64) TangentRecalcTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::FLAT | BufferLayoutType::SOA;
    static constexpr DataType required_types              = DataType::VECTOR4 | DataType::VECTOR4D;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    // Base transient flag (>0 triggers dynamic get_transient_requirement query in TaskGraphDOD)
    static constexpr size_t transient_workspace_bytes = 64;

    static constexpr std::string_view display_name = "Tangent Recalc";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t tangent_buffer_id   = INVALID_ID; // Primary target buffer (Vector4: xyz = tangent, w = sign)
    uint32_t vertex_buffer_id    = INVALID_ID; // Source deformed positions (Vector3)
    uint32_t normal_buffer_id    = INVALID_ID; // Source vertex normals (Vector3)
    uint32_t uv_buffer_id        = INVALID_ID; // Source texture coordinates (Vector2)
    uint32_t index_buffer_id     = INVALID_ID; // Triangle indices (int32_t triplets)
    uint32_t bitangent_buffer_id = INVALID_ID; // Optional scratch Vector3 buffer for exact handedness sign
    bool invert_handedness       = false;
    uint8_t reserved_padding[39] = {0};

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (const GrantPartPOD* tan_part = p_context.get_grant_part(tangent_buffer_id)) {
            if (tan_part->selection.capacity > 0) {
                return static_cast<size_t>(tan_part->selection.capacity) * sizeof(godot::Vector3);
            }
        }
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tan_prop;
        tan_prop["name"] = "tangent_buffer_id";
        tan_prop["type"] = godot::Variant::INT;
        tan_prop["hint_string"] = "buffer_option";
        props.push_back(tan_prop);

        godot::Dictionary vert_prop;
        vert_prop["name"] = "vertex_buffer_id";
        vert_prop["type"] = godot::Variant::INT;
        vert_prop["hint_string"] = "buffer_option";
        props.push_back(vert_prop);

        godot::Dictionary norm_prop;
        norm_prop["name"] = "normal_buffer_id";
        norm_prop["type"] = godot::Variant::INT;
        norm_prop["hint_string"] = "buffer_option";
        props.push_back(norm_prop);

        godot::Dictionary uv_prop;
        uv_prop["name"] = "uv_buffer_id";
        uv_prop["type"] = godot::Variant::INT;
        uv_prop["hint_string"] = "buffer_option";
        props.push_back(uv_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary bitan_prop;
        bitan_prop["name"] = "bitangent_buffer_id";
        bitan_prop["type"] = godot::Variant::INT;
        bitan_prop["hint_string"] = "buffer_option";
        props.push_back(bitan_prop);

        godot::Dictionary inv_prop;
        inv_prop["name"] = "invert_handedness";
        inv_prop["type"] = godot::Variant::BOOL;
        inv_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(inv_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return tangent_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("tangent_buffer_id")) {
            tangent_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["tangent_buffer_id"]));
        } else if (p_props.has("target_buffer_id")) {
            tangent_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        }
        if (p_props.has("normal_buffer_id")) {
            normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        }
        if (p_props.has("uv_buffer_id")) {
            uv_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["uv_buffer_id"]));
        }
        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("bitangent_buffer_id")) {
            bitangent_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["bitangent_buffer_id"]));
        }
        if (p_props.has("invert_handedness")) {
            invert_handedness = static_cast<bool>(p_props["invert_handedness"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_tangent_view) const {
        if constexpr (std::is_same_v<T, godot::Vector4>) {
            const GrantPartPOD* tan_part  = p_context.get_grant_part(tangent_buffer_id);
            const GrantPartPOD* vert_part = p_context.get_grant_part(vertex_buffer_id);
            const GrantPartPOD* norm_part = p_context.get_grant_part(normal_buffer_id);
            const GrantPartPOD* uv_part   = p_context.get_grant_part(uv_buffer_id);
            const GrantPartPOD* idx_part  = p_context.get_grant_part(index_buffer_id);
            if (!tan_part || !vert_part || !norm_part || !uv_part || !idx_part) return;

            godot::Vector4* __restrict tangents       = reinterpret_cast<godot::Vector4*>(tan_part->raw_base_ptr);
            const godot::Vector3* __restrict vertices = reinterpret_cast<const godot::Vector3*>(vert_part->raw_base_ptr);
            const godot::Vector3* __restrict normals  = reinterpret_cast<const godot::Vector3*>(norm_part->raw_base_ptr);
            const godot::Vector2* __restrict uvs      = reinterpret_cast<const godot::Vector2*>(uv_part->raw_base_ptr);
            const int32_t* __restrict indices         = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
            if (!tangents || !vertices || !normals || !uvs || !indices) return;

            const int64_t vert_count = std::min({
                tan_part->selection.capacity,
                vert_part->selection.capacity,
                norm_part->selection.capacity,
                uv_part->selection.capacity
            });
            if (vert_count <= 0) return;

            const int64_t idx_count = idx_part->selection.element_count > 0
                ? idx_part->selection.element_count
                : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t));

            // Optional bitangent accumulator in bitangent_buffer_id or local_workspace
            godot::Vector3* __restrict bitangents = nullptr;
            if (bitangent_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* bitan_part = p_context.get_grant_part(bitangent_buffer_id)) {
                    if (bitan_part->capacity_bytes >= static_cast<size_t>(vert_count) * sizeof(godot::Vector3)) {
                        bitangents = reinterpret_cast<godot::Vector3*>(bitan_part->raw_base_ptr);
                    }
                }
            }
            if (!bitangents && p_context.local_workspace) {
                bitangents = reinterpret_cast<godot::Vector3*>(p_context.local_workspace);
            }

            // 1. Zero-initialize accumulator buffers
            std::memset(static_cast<void*>(tangents), 0, static_cast<size_t>(vert_count) * sizeof(godot::Vector4));
            if (bitangents) {
                std::memset(static_cast<void*>(bitangents), 0, static_cast<size_t>(vert_count) * sizeof(godot::Vector3));
            }

            // 2. Accumulate UV-gradient tangent (and bitangent/handedness) across triangles
            for (int64_t t = 0; t + 2 < idx_count; t += 3) {
                const int32_t i0 = indices[t];
                const int32_t i1 = indices[t + 1];
                const int32_t i2 = indices[t + 2];

                if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= vert_count || i1 >= vert_count || i2 >= vert_count) continue;

                const godot::Vector3& v0 = vertices[i0];
                const godot::Vector3& v1 = vertices[i1];
                const godot::Vector3& v2 = vertices[i2];

                const godot::Vector2& w0 = uvs[i0];
                const godot::Vector2& w1 = uvs[i1];
                const godot::Vector2& w2 = uvs[i2];

                const godot::Vector3 dp1 = v1 - v0;
                const godot::Vector3 dp2 = v2 - v0;

                const float du1 = w1.x - w0.x;
                const float dv1 = w1.y - w0.y;
                const float du2 = w2.x - w0.x;
                const float dv2 = w2.y - w0.y;

                const float det = du1 * dv2 - du2 * dv1;
                if (std::abs(det) <= 1e-10f) continue;

                const float inv_det = 1.0f / det;
                const godot::Vector3 sdir = (dp1 * dv2 - dp2 * dv1) * inv_det;
                const godot::Vector3 tdir = (dp2 * du1 - dp1 * du2) * inv_det;
                const float face_sign     = (det < 0.0f) ? -1.0f : 1.0f;

                tangents[i0].x += sdir.x; tangents[i0].y += sdir.y; tangents[i0].z += sdir.z; tangents[i0].w += face_sign;
                tangents[i1].x += sdir.x; tangents[i1].y += sdir.y; tangents[i1].z += sdir.z; tangents[i1].w += face_sign;
                tangents[i2].x += sdir.x; tangents[i2].y += sdir.y; tangents[i2].z += sdir.z; tangents[i2].w += face_sign;

                if (bitangents) {
                    bitangents[i0] += tdir;
                    bitangents[i1] += tdir;
                    bitangents[i2] += tdir;
                }
            }

            // 3. Gram-Schmidt orthogonalization and handedness sign evaluation
            const float hand_flip = invert_handedness ? -1.0f : 1.0f;
            for (int64_t v = 0; v < vert_count; ++v) {
                const godot::Vector3 n = normals[v];
                const godot::Vector3 t_accum(tangents[v].x, tangents[v].y, tangents[v].z);

                // Orthogonalize tangent against vertex normal: t' = normalize(t - n * (n . t))
                godot::Vector3 ortho_t = t_accum - n * n.dot(t_accum);
                const float len_sq = ortho_t.length_squared();

                if (len_sq > 1e-12f) {
                    ortho_t *= (1.0f / std::sqrt(len_sq));
                } else {
                    // Fallback orthonormal tangent when UV gradient is degenerate
                    const godot::Vector3 axis = (std::abs(n.x) < 0.9f)
                        ? godot::Vector3(1.0f, 0.0f, 0.0f)
                        : godot::Vector3(0.0f, 1.0f, 0.0f);
                    ortho_t = n.cross(axis).normalized();
                }

                float w_sign = 1.0f;
                if (bitangents) {
                    w_sign = (n.cross(t_accum).dot(bitangents[v]) < 0.0f) ? -1.0f : 1.0f;
                } else {
                    w_sign = (tangents[v].w < 0.0f) ? -1.0f : 1.0f;
                }
                w_sign *= hand_flip;

                tangents[v] = godot::Vector4(ortho_t.x, ortho_t.y, ortho_t.z, w_sign);
            }
        }
    }
};

static_assert(sizeof(TangentRecalcTransformLogic<>) == 64, "TangentRecalcTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core