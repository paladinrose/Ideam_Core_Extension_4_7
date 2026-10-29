#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector4_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * MeshSurfaceExportTransformLogic<T>
 * Commits a modified or procedurally generated DOD surface buffer back into a
 * godot::Ref<godot::ArrayMesh> surface slot.
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshSurfaceExportTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3 | DataType::ANY_VECTOR4 |
        DataType::COLOR | DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Mesh Surface Export";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t source_buffer_id = INVALID_ID;
    int32_t surface_index     = 0;
    uint8_t array_type        = static_cast<uint8_t>(godot::Mesh::ARRAY_VERTEX);
    uint8_t primitive_type    = static_cast<uint8_t>(godot::Mesh::PRIMITIVE_TRIANGLES);
    bool clear_existing       = false;
    uint8_t reserved_flags    = 0;

    godot::Ref<godot::ArrayMesh> target_mesh;

    uint8_t reserved_padding[40] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "source_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary mesh_prop;
        mesh_prop["name"] = "target_mesh";
        mesh_prop["type"] = godot::Variant::OBJECT;
        mesh_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        mesh_prop["hint_string"] = "ArrayMesh";
        props.push_back(mesh_prop);

        godot::Dictionary surf_prop;
        surf_prop["name"] = "surface_index";
        surf_prop["type"] = godot::Variant::INT;
        surf_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        surf_prop["hint_string"] = "0,16,1";
        props.push_back(surf_prop);

        godot::Dictionary arr_prop;
        arr_prop["name"] = "array_type";
        arr_prop["type"] = godot::Variant::INT;
        arr_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        arr_prop["hint_string"] = "Vertex:0,Normal:1,Tangent:2,Color:3,TexUV:4,TexUV2:5,Custom0:6,Custom1:7,Custom2:8,Custom3:9,Bones:10,Weights:11,Index:12";
        props.push_back(arr_prop);

        godot::Dictionary prim_prop;
        prim_prop["name"] = "primitive_type";
        prim_prop["type"] = godot::Variant::INT;
        prim_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        prim_prop["hint_string"] = "Points:0,Lines:1,LineStrip:2,Triangles:3,TriangleStrip:4";
        props.push_back(prim_prop);

        godot::Dictionary clr_prop;
        clr_prop["name"] = "clear_existing";
        clr_prop["type"] = godot::Variant::BOOL;
        clr_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clr_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return source_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        } else if (p_props.has("buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }
        if (p_props.has("surface_index")) {
            surface_index = static_cast<int32_t>(static_cast<int64_t>(p_props["surface_index"]));
        }
        if (p_props.has("array_type")) {
            array_type = static_cast<uint8_t>(std::clamp<int64_t>(
                static_cast<int64_t>(p_props["array_type"]), 0, godot::Mesh::ARRAY_MAX - 1));
        }
        if (p_props.has("primitive_type")) {
            primitive_type = static_cast<uint8_t>(std::clamp<int64_t>(
                static_cast<int64_t>(p_props["primitive_type"]), 0, godot::Mesh::PRIMITIVE_TRIANGLE_STRIP - 1));
        }
        if (p_props.has("clear_existing")) {
            clear_existing = static_cast<bool>(p_props["clear_existing"]);
        }
        if (p_props.has("target_mesh")) {
            target_mesh = p_props["target_mesh"];
        } else if (p_props.has("mesh")) {
            target_mesh = p_props["mesh"];
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!target_mesh.is_valid()) return;

            const GrantPartPOD* part = p_context.get_grant_part(source_buffer_id);
            if (!part || !part->raw_base_ptr) return;

            const size_t elem_count = (part->selection.element_count > 0)
                ? static_cast<size_t>(part->selection.element_count)
                : (part->capacity_bytes / sizeof(T));
            if (elem_count == 0) return;

            godot::Array arrays;
            const int32_t surf_count = target_mesh->get_surface_count();
            const bool has_existing_surface = (!clear_existing && surface_index >= 0 && surface_index < surf_count);

            if (has_existing_surface) {
                arrays = target_mesh->surface_get_arrays(surface_index);
            }
            if (arrays.size() != godot::Mesh::ARRAY_MAX) {
                arrays.resize(godot::Mesh::ARRAY_MAX);
            }

            if constexpr (std::is_same_v<T, godot::Vector3>) {
                godot::PackedVector3Array out_arr;
                out_arr.resize(static_cast<int64_t>(elem_count));
                _export_elements(p_view, part, out_arr.ptrw(), elem_count);
                arrays[array_type] = out_arr;
            } else if constexpr (std::is_same_v<T, godot::Vector3i>) {
                godot::PackedVector3Array out_arr;
                out_arr.resize(static_cast<int64_t>(elem_count));
                godot::Vector3* dst = out_arr.ptrw();
                for (size_t i = 0; i < elem_count; ++i) {
                    const godot::Vector3i v = _read_view(p_view, static_cast<int64_t>(i));
                    dst[i] = godot::Vector3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
                }
                arrays[array_type] = out_arr;
            } else if constexpr (std::is_same_v<T, godot::Vector2>) {
                godot::PackedVector2Array out_arr;
                out_arr.resize(static_cast<int64_t>(elem_count));
                _export_elements(p_view, part, out_arr.ptrw(), elem_count);
                arrays[array_type] = out_arr;
            } else if constexpr (std::is_same_v<T, godot::Vector2i>) {
                godot::PackedVector2Array out_arr;
                out_arr.resize(static_cast<int64_t>(elem_count));
                godot::Vector2* dst = out_arr.ptrw();
                for (size_t i = 0; i < elem_count; ++i) {
                    const godot::Vector2i v = _read_view(p_view, static_cast<int64_t>(i));
                    dst[i] = godot::Vector2(static_cast<float>(v.x), static_cast<float>(v.y));
                }
                arrays[array_type] = out_arr;
            } else if constexpr (std::is_same_v<T, godot::Vector4>) {
                if (array_type == godot::Mesh::ARRAY_TANGENT || array_type == godot::Mesh::ARRAY_WEIGHTS) {
                    godot::PackedFloat32Array out_arr;
                    out_arr.resize(static_cast<int64_t>(elem_count * 4));
                    float* dst = out_arr.ptrw();
                    for (size_t i = 0; i < elem_count; ++i) {
                        const godot::Vector4 v = _read_view(p_view, static_cast<int64_t>(i));
                        const size_t base = i * 4;
                        dst[base]     = v.x;
                        dst[base + 1] = v.y;
                        dst[base + 2] = v.z;
                        dst[base + 3] = v.w;
                    }
                    arrays[array_type] = out_arr;
                } else if (array_type == godot::Mesh::ARRAY_COLOR) {
                    godot::PackedColorArray out_arr;
                    out_arr.resize(static_cast<int64_t>(elem_count));
                    godot::Color* dst = out_arr.ptrw();
                    for (size_t i = 0; i < elem_count; ++i) {
                        const godot::Vector4 v = _read_view(p_view, static_cast<int64_t>(i));
                        dst[i] = godot::Color(v.x, v.y, v.z, v.w);
                    }
                    arrays[array_type] = out_arr;
                } else {
                    godot::PackedVector4Array out_arr;
                    out_arr.resize(static_cast<int64_t>(elem_count));
                    _export_elements(p_view, part, out_arr.ptrw(), elem_count);
                    arrays[array_type] = out_arr;
                }
            } else if constexpr (std::is_same_v<T, godot::Vector4i>) {
                godot::PackedInt32Array out_arr;
                out_arr.resize(static_cast<int64_t>(elem_count * 4));
                int32_t* dst = out_arr.ptrw();
                for (size_t i = 0; i < elem_count; ++i) {
                    const godot::Vector4i v = _read_view(p_view, static_cast<int64_t>(i));
                    const size_t base = i * 4;
                    dst[base]     = v.x;
                    dst[base + 1] = v.y;
                    dst[base + 2] = v.z;
                    dst[base + 3] = v.w;
                }
                arrays[array_type] = out_arr;
            } else if constexpr (std::is_same_v<T, godot::Color>) {
                godot::PackedColorArray out_arr;
                out_arr.resize(static_cast<int64_t>(elem_count));
                _export_elements(p_view, part, out_arr.ptrw(), elem_count);
                arrays[array_type] = out_arr;
            } else if constexpr (std::is_same_v<T, float>) {
                godot::PackedFloat32Array out_arr;
                out_arr.resize(static_cast<int64_t>(elem_count));
                _export_elements(p_view, part, out_arr.ptrw(), elem_count);
                arrays[array_type] = out_arr;
            } else if constexpr (std::is_same_v<T, int32_t>) {
                godot::PackedInt32Array out_arr;
                out_arr.resize(static_cast<int64_t>(elem_count));
                _export_elements(p_view, part, out_arr.ptrw(), elem_count);
                arrays[array_type] = out_arr;
            } else if constexpr (std::is_same_v<T, uint8_t>) {
                godot::PackedByteArray out_arr;
                out_arr.resize(static_cast<int64_t>(elem_count));
                _export_elements(p_view, part, out_arr.ptrw(), elem_count);
                arrays[array_type] = out_arr;
            } else {
                return;
            }

            // Ensure ARRAY_VERTEX is valid before committing surface to ArrayMesh
            if (arrays[godot::Mesh::ARRAY_VERTEX].get_type() == godot::Variant::NIL) {
                return;
            }

            if (clear_existing) {
                target_mesh->clear_surfaces();
            } else if (has_existing_surface && surface_index == surf_count - 1) {
                target_mesh->surface_remove(surface_index);
            } else if (has_existing_surface) {
                target_mesh->clear_surfaces();
            }

            target_mesh->add_surface_from_arrays(
                static_cast<godot::Mesh::PrimitiveType>(primitive_type),
                arrays
            );
        }
    }

private:
    template <typename T_View>
    static inline void _export_elements(const T_View& p_view, const GrantPartPOD* p_part, T* p_dst, size_t p_count) noexcept {
        if (!p_dst || p_count == 0) return;
        if (p_part->element_stride == sizeof(T) && p_part->selection.mode == SelectionMode::RANGE) {
            const size_t start_offset = static_cast<size_t>(p_part->selection.start_index) * sizeof(T);
            if (start_offset + p_count * sizeof(T) <= p_part->capacity_bytes) {
                std::memcpy(p_dst, p_part->raw_base_ptr + start_offset, p_count * sizeof(T));
                return;
            }
        }
        for (size_t i = 0; i < p_count; ++i) {
            p_dst[i] = _read_view(p_view, static_cast<int64_t>(i));
        }
    }

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
};

static_assert(sizeof(MeshSurfaceExportTransformLogic<>) == 64, "MeshSurfaceExportTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core