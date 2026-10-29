#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class MultiMeshRotationFormat : uint8_t {
    IDENTITY   = 0,
    QUATERNION = 1, // Reads Vector4(x, y, z, w) or Quaternion
    EULER_XYZ  = 2  // Reads Vector3 Euler angles (radians)
};

enum class MultiMeshScaleFormat : uint8_t {
    UNIFORM_DEFAULT = 0, // Uses uniform_scale_fixed scalar
    VECTOR3_SCALE   = 1, // Reads non-uniform Vector3 scale buffer
    FLOAT_UNIFORM   = 2  // Reads per-instance float scale buffer
};

/**
 * MultiMeshBufferSyncTransformLogic<T>
 * Interleaves SoA DOD entity columns (Position, Rotation, Scale, Color, CustomData)
 * into a 12-, 16-, or 20-float per-instance row-major Transform3D buffer and commits
 * directly to Godot's RenderingServer::multimesh_set_buffer().
 */
template <typename T = godot::Vector3>
struct alignas(64) MultiMeshBufferSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::FLOAT32 | DataType::VECTOR3 | DataType::VECTOR3D |
        DataType::VECTOR4 | DataType::VECTOR4D | DataType::COLOR;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "MultiMesh Buffer Sync";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t position_buffer_id    = INVALID_ID; // 4B: Source Vector3 instance positions
    uint32_t rotation_buffer_id    = INVALID_ID; // 4B: Optional Vector4 (Quaternion) or Vector3 (Euler) rotations
    uint32_t scale_buffer_id       = INVALID_ID; // 4B: Optional Vector3 or float scales
    uint32_t color_buffer_id       = INVALID_ID; // 4B: Optional Color or Vector4 instance colors
    uint32_t custom_data_buffer_id = INVALID_ID; // 4B: Optional Color or Vector4 custom data
    uint32_t output_buffer_id      = INVALID_ID; // 4B: Optional float DOD staging buffer

    MultiMeshRotationFormat rotation_format = MultiMeshRotationFormat::QUATERNION; // 1B
    MultiMeshScaleFormat scale_format       = MultiMeshScaleFormat::UNIFORM_DEFAULT; // 1B
    bool include_colors                     = false; // 1B: +4 floats per instance
    bool include_custom_data                = false; // 1B: +4 floats per instance
    bool sync_visible_count                 = true;  // 1B: Update multimesh_set_visible_instances()
    bool sync_to_server                     = true;  // 1B: Commit PackedFloat32Array to RenderingServer
    uint16_t uniform_scale_fixed            = 1000;  // 2B: Fixed-point uniform scale (1000 = 1.0f)

    godot::RID target_multimesh_rid;                 // 8B
    godot::Ref<godot::MultiMesh> target_multimesh;   // 8B

    // Mutable staging PackedFloat32Array to avoid per-frame Variant vector allocations
    mutable godot::PackedFloat32Array staging_floats; // 16B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary pos_prop;
        pos_prop["name"] = "position_buffer_id";
        pos_prop["type"] = godot::Variant::INT;
        pos_prop["hint_string"] = "buffer_option";
        props.push_back(pos_prop);

        godot::Dictionary rot_prop;
        rot_prop["name"] = "rotation_buffer_id";
        rot_prop["type"] = godot::Variant::INT;
        rot_prop["hint_string"] = "buffer_option";
        props.push_back(rot_prop);

        godot::Dictionary scl_prop;
        scl_prop["name"] = "scale_buffer_id";
        scl_prop["type"] = godot::Variant::INT;
        scl_prop["hint_string"] = "buffer_option";
        props.push_back(scl_prop);

        godot::Dictionary col_prop;
        col_prop["name"] = "color_buffer_id";
        col_prop["type"] = godot::Variant::INT;
        col_prop["hint_string"] = "buffer_option";
        props.push_back(col_prop);

        godot::Dictionary cust_prop;
        cust_prop["name"] = "custom_data_buffer_id";
        cust_prop["type"] = godot::Variant::INT;
        cust_prop["hint_string"] = "buffer_option";
        props.push_back(cust_prop);

        godot::Dictionary out_prop;
        out_prop["name"] = "output_buffer_id";
        out_prop["type"] = godot::Variant::INT;
        out_prop["hint_string"] = "buffer_option";
        props.push_back(out_prop);

        godot::Dictionary mm_prop;
        mm_prop["name"] = "target_multimesh";
        mm_prop["type"] = godot::Variant::OBJECT;
        mm_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        mm_prop["hint_string"] = "MultiMesh";
        props.push_back(mm_prop);

        godot::Dictionary rfmt_prop;
        rfmt_prop["name"] = "rotation_format";
        rfmt_prop["type"] = godot::Variant::INT;
        rfmt_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        rfmt_prop["hint_string"] = "Identity:0,Quaternion (Vector4):1,Euler XYZ (Vector3):2";
        props.push_back(rfmt_prop);

        godot::Dictionary sfmt_prop;
        sfmt_prop["name"] = "scale_format";
        sfmt_prop["type"] = godot::Variant::INT;
        sfmt_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        sfmt_prop["hint_string"] = "Uniform Default:0,Vector3 Scale:1,Float Uniform Scale:2";
        props.push_back(sfmt_prop);

        godot::Dictionary uscl_prop;
        uscl_prop["name"] = "uniform_scale";
        uscl_prop["type"] = godot::Variant::FLOAT;
        uscl_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        uscl_prop["hint_string"] = "0.001,65.0,0.01";
        props.push_back(uscl_prop);

        godot::Dictionary icol_prop;
        icol_prop["name"] = "include_colors";
        icol_prop["type"] = godot::Variant::BOOL;
        icol_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(icol_prop);

        godot::Dictionary icust_prop;
        icust_prop["name"] = "include_custom_data";
        icust_prop["type"] = godot::Variant::BOOL;
        icust_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(icust_prop);

        godot::Dictionary vis_prop;
        vis_prop["name"] = "sync_visible_count";
        vis_prop["type"] = godot::Variant::BOOL;
        vis_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(vis_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if constexpr (std::is_same_v<T, float>) {
            if (output_buffer_id != INVALID_ID) return output_buffer_id;
        }
        return (position_buffer_id != INVALID_ID) ? position_buffer_id : output_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("position_buffer_id")) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("rotation_buffer_id")) {
            rotation_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rotation_buffer_id"]));
        }
        if (p_props.has("scale_buffer_id")) {
            scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["scale_buffer_id"]));
        }
        if (p_props.has("color_buffer_id")) {
            color_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["color_buffer_id"]));
        }
        if (p_props.has("custom_data_buffer_id")) {
            custom_data_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["custom_data_buffer_id"]));
        }
        if (p_props.has("output_buffer_id")) {
            output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_buffer_id"]));
        }
        if (p_props.has("rotation_format")) {
            rotation_format = static_cast<MultiMeshRotationFormat>(static_cast<uint8_t>(static_cast<int64_t>(p_props["rotation_format"])));
        }
        if (p_props.has("scale_format")) {
            scale_format = static_cast<MultiMeshScaleFormat>(static_cast<uint8_t>(static_cast<int64_t>(p_props["scale_format"])));
        }
        if (p_props.has("uniform_scale")) {
            const float s = std::clamp(static_cast<float>(p_props["uniform_scale"]), 0.001f, 65.5f);
            uniform_scale_fixed = static_cast<uint16_t>(std::lround(s * 1000.0f));
        }
        if (p_props.has("include_colors")) {
            include_colors = static_cast<bool>(p_props["include_colors"]);
        }
        if (p_props.has("include_custom_data")) {
            include_custom_data = static_cast<bool>(p_props["include_custom_data"]);
        }
        if (p_props.has("sync_visible_count")) {
            sync_visible_count = static_cast<bool>(p_props["sync_visible_count"]);
        }
        if (p_props.has("sync_to_server")) {
            sync_to_server = static_cast<bool>(p_props["sync_to_server"]);
        }
        if (p_props.has("target_multimesh_rid")) {
            target_multimesh_rid = p_props["target_multimesh_rid"];
        }
        if (p_props.has("target_multimesh")) {
            target_multimesh = p_props["target_multimesh"];
            if (target_multimesh.is_valid()) {
                target_multimesh_rid = target_multimesh->get_rid();
            }
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
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* pos_part = p_context.get_grant_part(position_buffer_id);
            if (!pos_part || !pos_part->raw_base_ptr) return;

            const MemoryBufferSelectionPOD& sel = pos_part->selection;
            const int64_t active_count = sel.element_count;
            if (active_count <= 0) return;

            const godot::Vector3* __restrict positions = reinterpret_cast<const godot::Vector3*>(pos_part->raw_base_ptr);
            const int64_t pos_cap = static_cast<int64_t>(pos_part->capacity_bytes / sizeof(godot::Vector3));
            if (pos_cap <= 0) return;

            const GrantPartPOD* rot_part  = (rotation_buffer_id != INVALID_ID)    ? p_context.get_grant_part(rotation_buffer_id)    : nullptr;
            const GrantPartPOD* scl_part  = (scale_buffer_id != INVALID_ID)       ? p_context.get_grant_part(scale_buffer_id)       : nullptr;
            const GrantPartPOD* col_part  = (color_buffer_id != INVALID_ID)       ? p_context.get_grant_part(color_buffer_id)       : nullptr;
            const GrantPartPOD* cust_part = (custom_data_buffer_id != INVALID_ID) ? p_context.get_grant_part(custom_data_buffer_id) : nullptr;
            const GrantPartPOD* out_part  = (output_buffer_id != INVALID_ID)      ? p_context.get_grant_part(output_buffer_id)      : nullptr;

            const godot::Vector4* __restrict rot_quats  = (rot_part && rot_part->raw_base_ptr) ? reinterpret_cast<const godot::Vector4*>(rot_part->raw_base_ptr) : nullptr;
            const godot::Vector3* __restrict rot_eulers = (rot_part && rot_part->raw_base_ptr) ? reinterpret_cast<const godot::Vector3*>(rot_part->raw_base_ptr) : nullptr;
            const godot::Vector3* __restrict scl_vecs   = (scl_part && scl_part->raw_base_ptr) ? reinterpret_cast<const godot::Vector3*>(scl_part->raw_base_ptr) : nullptr;
            const float* __restrict scl_floats          = (scl_part && scl_part->raw_base_ptr) ? reinterpret_cast<const float*>(scl_part->raw_base_ptr)          : nullptr;
            const godot::Color* __restrict colors       = (col_part && col_part->raw_base_ptr) ? reinterpret_cast<const godot::Color*>(col_part->raw_base_ptr)   : nullptr;
            const godot::Vector4* __restrict customs    = (cust_part && cust_part->raw_base_ptr)? reinterpret_cast<const godot::Vector4*>(cust_part->raw_base_ptr): nullptr;

            const size_t floats_per_instance = 12u + (include_colors ? 4u : 0u) + (include_custom_data ? 4u : 0u);

            int64_t max_instances = active_count;
            if (target_multimesh.is_valid()) {
                const int32_t mm_count = target_multimesh->get_instance_count();
                if (mm_count > 0) {
                    max_instances = std::min<int64_t>(max_instances, mm_count);
                }
            }
            if (out_part && out_part->raw_base_ptr) {
                const int64_t out_cap_inst = static_cast<int64_t>(out_part->capacity_bytes / (floats_per_instance * sizeof(float)));
                if (out_cap_inst > 0) {
                    max_instances = std::min<int64_t>(max_instances, out_cap_inst);
                }
            }
            if (max_instances <= 0) return;

            const size_t total_floats = static_cast<size_t>(max_instances) * floats_per_instance;
            float* __restrict dst_buffer = nullptr;

            if (sync_to_server && target_multimesh_rid.is_valid()) {
                if (static_cast<size_t>(staging_floats.size()) != total_floats) {
                    staging_floats.resize(static_cast<int64_t>(total_floats));
                }
                dst_buffer = staging_floats.ptrw();
            } else if (out_part && out_part->raw_base_ptr) {
                dst_buffer = reinterpret_cast<float*>(out_part->raw_base_ptr);
            }
            if (!dst_buffer) return;

            const float default_scale = static_cast<float>(uniform_scale_fixed) * 0.001f;

            auto write_instance = [&](int64_t src_idx, int64_t out_inst_idx) noexcept {
                if (src_idx < 0 || src_idx >= pos_cap) return;

                const godot::Vector3 pos = positions[src_idx];
                godot::Basis basis;

                if (rot_part && rotation_format == MultiMeshRotationFormat::QUATERNION && rot_quats) {
                    const godot::Vector4& qv = rot_quats[src_idx];
                    basis = godot::Basis(godot::Quaternion(qv.x, qv.y, qv.z, qv.w));
                } else if (rot_part && rotation_format == MultiMeshRotationFormat::EULER_XYZ && rot_eulers) {
                    basis = godot::Basis::from_euler(rot_eulers[src_idx]);
                }

                godot::Vector3 scl(default_scale, default_scale, default_scale);
                if (scl_part) {
                    if (scale_format == MultiMeshScaleFormat::VECTOR3_SCALE && scl_vecs) {
                        scl = scl_vecs[src_idx];
                    } else if (scale_format == MultiMeshScaleFormat::FLOAT_UNIFORM && scl_floats) {
                        const float s = scl_floats[src_idx];
                        scl = godot::Vector3(s, s, s);
                    }
                }
                basis.scale(scl);

                float* __restrict ptr = dst_buffer + (static_cast<size_t>(out_inst_idx) * floats_per_instance);

                // Godot MultiMesh 3D transform buffer layout: 3x4 row-major (basis row 0 + origin.x, row 1 + origin.y, row 2 + origin.z)
                ptr[0]  = basis.rows[0].x; ptr[1]  = basis.rows[0].y; ptr[2]  = basis.rows[0].z; ptr[3]  = pos.x;
                ptr[4]  = basis.rows[1].x; ptr[5]  = basis.rows[1].y; ptr[6]  = basis.rows[1].z; ptr[7]  = pos.y;
                ptr[8]  = basis.rows[2].x; ptr[9]  = basis.rows[2].y; ptr[10] = basis.rows[2].z; ptr[11] = pos.z;

                size_t offset = 12;
                if (include_colors) {
                    const godot::Color c = colors ? colors[src_idx] : godot::Color(1.0f, 1.0f, 1.0f, 1.0f);
                    ptr[offset + 0] = c.r;
                    ptr[offset + 1] = c.g;
                    ptr[offset + 2] = c.b;
                    ptr[offset + 3] = c.a;
                    offset += 4;
                }
                if (include_custom_data) {
                    const godot::Vector4 cd = customs ? customs[src_idx] : godot::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
                    ptr[offset + 0] = cd.x;
                    ptr[offset + 1] = cd.y;
                    ptr[offset + 2] = cd.z;
                    ptr[offset + 3] = cd.w;
                }
            };

            int64_t written = 0;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                for (int64_t i = 0; i < sel.capacity && written < max_instances; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        write_instance(i, written++);
                    }
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count && written < max_instances; ++i) {
                    write_instance(sel.data.indices[i], written++);
                }
            } else {
                const int64_t end = sel.start_index + sel.element_count;
                for (int64_t i = sel.start_index; i < end && written < max_instances; ++i) {
                    write_instance(i, written++);
                }
            }

            // Mirror to optional DOD output_buffer_id if both Server and DOD output are active
            if (sync_to_server && target_multimesh_rid.is_valid() && out_part && out_part->raw_base_ptr) {
                const size_t copy_bytes = std::min<size_t>(total_floats * sizeof(float), static_cast<size_t>(out_part->capacity_bytes));
                std::memcpy(out_part->raw_base_ptr, dst_buffer, copy_bytes);
            }

            if (sync_to_server && target_multimesh_rid.is_valid()) {
                godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
                rs->multimesh_set_buffer(target_multimesh_rid, staging_floats);
                if (sync_visible_count) {
                    rs->multimesh_set_visible_instances(target_multimesh_rid, static_cast<int32_t>(written));
                }
            }
        }
    }
};

static_assert(sizeof(MultiMeshBufferSyncTransformLogic<>) == 64, "MultiMeshBufferSyncTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core