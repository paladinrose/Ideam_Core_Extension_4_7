#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <godot_cpp/variant/vector4i.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * CPULinearBlendSkinningTransformLogic<T>
 * Performs 4-influence SIMD-friendly Linear Blend Skinning (LBS) on the CPU:
 *   v_skinned[i] = sum_{j=0..3} w_{i,j} * (M_{bone[i,j]} * v_rest[i])
 * where M_b = G_b * B_b^-1 is pre-multiplied into local_workspace (or read directly from
 * global_pose_buffer_id when inv_bind_pose_buffer_id is INVALID_ID).
 * Also optionally skins vertex normals (rest_normal_buffer_id -> out_normal_buffer_id)
 * so downstream slicing, collision, and raycast tasks read exact animated world geometry.
 */
template <typename T = godot::Vector3>
struct alignas(64) CPULinearBlendSkinningTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    // Base transient flag (>0 triggers dynamic get_transient_requirement query in TaskGraphDOD)
    static constexpr size_t transient_workspace_bytes = 64;

    static constexpr std::string_view display_name = "CPU Linear Blend Skinning";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t out_vertex_buffer_id     = INVALID_ID; // 4B: Primary target buffer (skinned Vector3 positions)
    uint32_t rest_vertex_buffer_id    = INVALID_ID; // 4B: Source rest-pose Vector3 positions
    uint32_t bone_indices_buffer_id   = INVALID_ID; // 4B: Per-vertex 4x int32_t (or Vector4i) bone indices (ARRAY_BONES)
    uint32_t bone_weights_buffer_id   = INVALID_ID; // 4B: Per-vertex 4x float (or Vector4) bone weights (ARRAY_WEIGHTS)
    uint32_t global_pose_buffer_id    = INVALID_ID; // 4B: Bone global pose Transform3D buffer (G_b)
    uint32_t inv_bind_pose_buffer_id  = INVALID_ID; // 4B: Optional bone inverse-bind Transform3D buffer (B_b^-1)
    uint32_t out_normal_buffer_id     = INVALID_ID; // 4B: Optional output skinned Vector3 normals
    uint32_t rest_normal_buffer_id    = INVALID_ID; // 4B: Optional input rest-pose Vector3 normals

    bool normalize_weights            = true;       // 1B: Renormalize 4 weights to sum to 1.0
    bool normalize_normals            = true;       // 1B: Normalize output skinned normals
    uint8_t reserved_padding[30]      = {0};        // 30B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (inv_bind_pose_buffer_id == INVALID_ID) return 0;
        if (const GrantPartPOD* g_part = p_context.get_grant_part(global_pose_buffer_id)) {
            const size_t bone_count = g_part->capacity_bytes / sizeof(godot::Transform3D);
            if (bone_count > 0) {
                return bone_count * sizeof(godot::Transform3D);
            }
        }
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary overt_prop;
        overt_prop["name"] = "out_vertex_buffer_id";
        overt_prop["type"] = godot::Variant::INT;
        overt_prop["hint_string"] = "buffer_option";
        props.push_back(overt_prop);

        godot::Dictionary rvert_prop;
        rvert_prop["name"] = "rest_vertex_buffer_id";
        rvert_prop["type"] = godot::Variant::INT;
        rvert_prop["hint_string"] = "buffer_option";
        props.push_back(rvert_prop);

        godot::Dictionary bidx_prop;
        bidx_prop["name"] = "bone_indices_buffer_id";
        bidx_prop["type"] = godot::Variant::INT;
        bidx_prop["hint_string"] = "buffer_option";
        props.push_back(bidx_prop);

        godot::Dictionary bwgt_prop;
        bwgt_prop["name"] = "bone_weights_buffer_id";
        bwgt_prop["type"] = godot::Variant::INT;
        bwgt_prop["hint_string"] = "buffer_option";
        props.push_back(bwgt_prop);

        godot::Dictionary gpose_prop;
        gpose_prop["name"] = "global_pose_buffer_id";
        gpose_prop["type"] = godot::Variant::INT;
        gpose_prop["hint_string"] = "buffer_option";
        props.push_back(gpose_prop);

        godot::Dictionary ibind_prop;
        ibind_prop["name"] = "inv_bind_pose_buffer_id";
        ibind_prop["type"] = godot::Variant::INT;
        ibind_prop["hint_string"] = "buffer_option";
        props.push_back(ibind_prop);

        godot::Dictionary onorm_prop;
        onorm_prop["name"] = "out_normal_buffer_id";
        onorm_prop["type"] = godot::Variant::INT;
        onorm_prop["hint_string"] = "buffer_option";
        props.push_back(onorm_prop);

        godot::Dictionary rnorm_prop;
        rnorm_prop["name"] = "rest_normal_buffer_id";
        rnorm_prop["type"] = godot::Variant::INT;
        rnorm_prop["hint_string"] = "buffer_option";
        props.push_back(rnorm_prop);

        godot::Dictionary nw_prop;
        nw_prop["name"] = "normalize_weights";
        nw_prop["type"] = godot::Variant::BOOL;
        nw_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(nw_prop);

        godot::Dictionary nn_prop;
        nn_prop["name"] = "normalize_normals";
        nn_prop["type"] = godot::Variant::BOOL;
        nn_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(nn_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return out_vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("out_vertex_buffer_id")) {
            out_vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["out_vertex_buffer_id"]));
        } else if (p_props.has("target_buffer_id")) {
            out_vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("rest_vertex_buffer_id")) {
            rest_vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rest_vertex_buffer_id"]));
        }
        if (p_props.has("bone_indices_buffer_id")) {
            bone_indices_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["bone_indices_buffer_id"]));
        }
        if (p_props.has("bone_weights_buffer_id")) {
            bone_weights_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["bone_weights_buffer_id"]));
        }
        if (p_props.has("global_pose_buffer_id")) {
            global_pose_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["global_pose_buffer_id"]));
        }
        if (p_props.has("inv_bind_pose_buffer_id")) {
            inv_bind_pose_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["inv_bind_pose_buffer_id"]));
        }
        if (p_props.has("out_normal_buffer_id")) {
            out_normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["out_normal_buffer_id"]));
        }
        if (p_props.has("rest_normal_buffer_id")) {
            rest_normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rest_normal_buffer_id"]));
        }
        if (p_props.has("normalize_weights")) {
            normalize_weights = static_cast<bool>(p_props["normalize_weights"]);
        }
        if (p_props.has("normalize_normals")) {
            normalize_normals = static_cast<bool>(p_props["normalize_normals"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_out_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const GrantPartPOD* out_part   = p_context.get_grant_part(out_vertex_buffer_id);
            const GrantPartPOD* bidx_part  = p_context.get_grant_part(bone_indices_buffer_id);
            const GrantPartPOD* bwgt_part  = p_context.get_grant_part(bone_weights_buffer_id);
            const GrantPartPOD* gpose_part = p_context.get_grant_part(global_pose_buffer_id);
            if (!out_part || !bidx_part || !bwgt_part || !gpose_part) return;
            if (!out_part->raw_base_ptr || !bidx_part->raw_base_ptr || !bwgt_part->raw_base_ptr || !gpose_part->raw_base_ptr) return;

            const GrantPartPOD* rvert_part = (rest_vertex_buffer_id != INVALID_ID && rest_vertex_buffer_id != out_vertex_buffer_id)
                ? p_context.get_grant_part(rest_vertex_buffer_id)
                : nullptr;
            const godot::Vector3* __restrict rest_verts = (rvert_part && rvert_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector3*>(rvert_part->raw_base_ptr)
                : nullptr;

            const int64_t vert_cap = static_cast<int64_t>(out_part->capacity_bytes / sizeof(godot::Vector3));
            const int64_t bidx_cap = static_cast<int64_t>(bidx_part->capacity_bytes / (4 * sizeof(int32_t)));
            const int64_t bwgt_cap = static_cast<int64_t>(bwgt_part->capacity_bytes / (4 * sizeof(float)));
            const int64_t max_verts = std::min({vert_cap, bidx_cap, bwgt_cap});
            if (max_verts <= 0) return;

            const int32_t* __restrict bone_indices = reinterpret_cast<const int32_t*>(bidx_part->raw_base_ptr);
            const float* __restrict bone_weights   = reinterpret_cast<const float*>(bwgt_part->raw_base_ptr);

            const godot::Transform3D* __restrict global_poses = reinterpret_cast<const godot::Transform3D*>(gpose_part->raw_base_ptr);
            int64_t bone_count = static_cast<int64_t>(gpose_part->capacity_bytes / sizeof(godot::Transform3D));
            if (bone_count <= 0) return;

            // Pre-multiply skinning palette M_b = G_b * B_b^-1 into local_workspace if inv_bind_pose_buffer_id is provided
            const godot::Transform3D* __restrict skin_palette = global_poses;
            if (inv_bind_pose_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* ibind_part = p_context.get_grant_part(inv_bind_pose_buffer_id)) {
                    if (ibind_part->raw_base_ptr && p_context.local_workspace) {
                        const godot::Transform3D* __restrict inv_binds = reinterpret_cast<const godot::Transform3D*>(ibind_part->raw_base_ptr);
                        const int64_t ibind_count = static_cast<int64_t>(ibind_part->capacity_bytes / sizeof(godot::Transform3D));
                        bone_count = std::min(bone_count, ibind_count);

                        godot::Transform3D* __restrict palette_ws = reinterpret_cast<godot::Transform3D*>(p_context.local_workspace);
                        for (int64_t b = 0; b < bone_count; ++b) {
                            palette_ws[b] = global_poses[b] * inv_binds[b];
                        }
                        skin_palette = palette_ws;
                    }
                }
            }

            // Optional normal skinning buffers
            const GrantPartPOD* rnorm_part = (rest_normal_buffer_id != INVALID_ID) ? p_context.get_grant_part(rest_normal_buffer_id) : nullptr;
            const GrantPartPOD* onorm_part = (out_normal_buffer_id != INVALID_ID)  ? p_context.get_grant_part(out_normal_buffer_id)  : nullptr;
            const godot::Vector3* __restrict rest_normals = (rnorm_part && rnorm_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector3*>(rnorm_part->raw_base_ptr)
                : nullptr;
            godot::Vector3* __restrict out_normals = (onorm_part && onorm_part->raw_base_ptr)
                ? reinterpret_cast<godot::Vector3*>(onorm_part->raw_base_ptr)
                : nullptr;
            const int64_t norm_cap = (rest_normals && out_normals)
                ? std::min<int64_t>(
                    static_cast<int64_t>(rnorm_part->capacity_bytes / sizeof(godot::Vector3)),
                    static_cast<int64_t>(onorm_part->capacity_bytes / sizeof(godot::Vector3)))
                : 0;

            auto skin_vertex = [&](int64_t i) noexcept {
                if (i < 0 || i >= max_verts) return;

                const godot::Vector3 v_rest = rest_verts ? rest_verts[i] : _read_view(p_out_view, i);
                const int64_t base4 = i * 4;

                const int32_t b0 = bone_indices[base4 + 0];
                const int32_t b1 = bone_indices[base4 + 1];
                const int32_t b2 = bone_indices[base4 + 2];
                const int32_t b3 = bone_indices[base4 + 3];

                float w0 = bone_weights[base4 + 0];
                float w1 = bone_weights[base4 + 1];
                float w2 = bone_weights[base4 + 2];
                float w3 = bone_weights[base4 + 3];

                const float w_sum = w0 + w1 + w2 + w3;
                if (w_sum <= 1e-6f) {
                    _write_view(p_out_view, i, v_rest);
                    if (out_normals && i < norm_cap) out_normals[i] = rest_normals[i];
                    return;
                }
                if (normalize_weights && std::abs(w_sum - 1.0f) > 1e-5f) {
                    const float inv_sum = 1.0f / w_sum;
                    w0 *= inv_sum; w1 *= inv_sum; w2 *= inv_sum; w3 *= inv_sum;
                }

                godot::Vector3 v_skinned(0.0f, 0.0f, 0.0f);
                if (w0 > 0.0f && b0 >= 0 && b0 < bone_count) v_skinned += skin_palette[b0].xform(v_rest) * w0;
                if (w1 > 0.0f && b1 >= 0 && b1 < bone_count) v_skinned += skin_palette[b1].xform(v_rest) * w1;
                if (w2 > 0.0f && b2 >= 0 && b2 < bone_count) v_skinned += skin_palette[b2].xform(v_rest) * w2;
                if (w3 > 0.0f && b3 >= 0 && b3 < bone_count) v_skinned += skin_palette[b3].xform(v_rest) * w3;

                _write_view(p_out_view, i, v_skinned);

                if (out_normals && i < norm_cap) {
                    const godot::Vector3 n_rest = rest_normals[i];
                    godot::Vector3 n_skinned(0.0f, 0.0f, 0.0f);
                    if (w0 > 0.0f && b0 >= 0 && b0 < bone_count) n_skinned += skin_palette[b0].basis.xform(n_rest) * w0;
                    if (w1 > 0.0f && b1 >= 0 && b1 < bone_count) n_skinned += skin_palette[b1].basis.xform(n_rest) * w1;
                    if (w2 > 0.0f && b2 >= 0 && b2 < bone_count) n_skinned += skin_palette[b2].basis.xform(n_rest) * w2;
                    if (w3 > 0.0f && b3 >= 0 && b3 < bone_count) n_skinned += skin_palette[b3].basis.xform(n_rest) * w3;
                    if (normalize_normals) {
                        n_skinned = n_skinned.normalized();
                    }
                    out_normals[i] = n_skinned;
                }
            };

            const MemoryBufferSelectionPOD& sel = out_part->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                const int64_t limit = std::min<int64_t>(sel.capacity, max_verts);
                for (int64_t i = 0; i < limit; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) skin_vertex(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    skin_vertex(sel.data.indices[i]);
                }
            } else {
                const int64_t end = std::min<int64_t>(sel.start_index + sel.element_count, max_verts);
                for (int64_t i = sel.start_index; i < end; ++i) {
                    skin_vertex(i);
                }
            }
        }
    }

private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline T _read_view(const T_View& p_view, int64_t idx) noexcept {
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<T>(p_view[idx].read());
        } else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<T>(p_view[idx].load());
        } else if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else {
            return static_cast<T>(p_view[idx]);
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _write_view(const T_View& p_view, int64_t idx, const T& value) noexcept {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            *reinterpret_cast<T*>(p_view[idx]) = value;
        } else if constexpr (requires { p_view[idx].write(); }) {
            p_view[idx] = value;
        } else if constexpr (requires { p_view[idx].store(value); }) {
            p_view[idx].store(value);
        } else {
            (void)(p_view[idx] = value);
        }
    }
};

static_assert(sizeof(CPULinearBlendSkinningTransformLogic<>) == 64, "CPULinearBlendSkinningTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core