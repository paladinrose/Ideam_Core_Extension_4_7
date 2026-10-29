#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector4_array.hpp>
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
 * MeshSurfaceImportTransformLogic<T>
 * Extracts a single surface array (ARRAY_VERTEX, ARRAY_NORMAL, ARRAY_TANGENT,
 * ARRAY_COLOR, ARRAY_TEX_UV, ARRAY_TEX_UV2, ARRAY_BONES, ARRAY_WEIGHTS, or ARRAY_INDEX)
 * from a godot::Ref<godot::Mesh> into a target DOD MemoryBuffer grant.
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshSurfaceImportTransformLogic {
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

    static constexpr std::string_view display_name = "Mesh Surface Import";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id      = INVALID_ID;
    int32_t surface_index          = 0;
    uint8_t array_type             = static_cast<uint8_t>(godot::Mesh::ARRAY_VERTEX);
    bool one_shot                  = true;
    mutable bool already_imported  = false;
    uint8_t reserved_flags         = 0;

    godot::Ref<godot::Mesh> source_mesh;

    uint8_t reserved_padding[40] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "target_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary mesh_prop;
        mesh_prop["name"] = "source_mesh";
        mesh_prop["type"] = godot::Variant::OBJECT;
        mesh_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        mesh_prop["hint_string"] = "Mesh";
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

        godot::Dictionary oneshot_prop;
        oneshot_prop["name"] = "one_shot";
        oneshot_prop["type"] = godot::Variant::BOOL;
        oneshot_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(oneshot_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }
        if (p_props.has("surface_index")) {
            surface_index = static_cast<int32_t>(static_cast<int64_t>(p_props["surface_index"]));
        }
        if (p_props.has("array_type")) {
            array_type = static_cast<uint8_t>(std::clamp<int64_t>(
                static_cast<int64_t>(p_props["array_type"]), 0, godot::Mesh::ARRAY_MAX - 1));
        }
        if (p_props.has("one_shot")) {
            one_shot = static_cast<bool>(p_props["one_shot"]);
        }
        if (p_props.has("source_mesh")) {
            source_mesh = p_props["source_mesh"];
        } else if (p_props.has("mesh")) {
            source_mesh = p_props["mesh"];
        }
        already_imported = false;
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (one_shot && already_imported) return;
            if (!source_mesh.is_valid()) return;
            if (surface_index < 0 || surface_index >= source_mesh->get_surface_count()) return;

            const GrantPartPOD* part = p_context.get_grant_part(target_buffer_id);
            if (!part || !part->raw_base_ptr) return;

            const godot::Array arrays = source_mesh->surface_get_arrays(surface_index);
            if (array_type >= arrays.size()) return;

            const godot::Variant& slot_var = arrays[array_type];
            if (slot_var.get_type() == godot::Variant::NIL) return;

            const uint8_t* src_bytes = nullptr;
            size_t src_byte_count    = 0;
            size_t src_elem_count    = 0;

            switch (slot_var.get_type()) {
                case godot::Variant::PACKED_VECTOR3_ARRAY: {
                    const godot::PackedVector3Array arr = slot_var;
                    src_elem_count = static_cast<size_t>(arr.size());
                    src_byte_count = src_elem_count * sizeof(godot::Vector3);
                    src_bytes      = reinterpret_cast<const uint8_t*>(arr.ptr());
                    if (_try_raw_copy(part, src_bytes, src_byte_count, sizeof(godot::Vector3))) {
                        already_imported = true;
                        return;
                    }
                    if constexpr (std::is_same_v<T, godot::Vector3>) {
                        _copy_typed_elements(p_view, part, arr.ptr(), src_elem_count);
                        already_imported = true;
                    } else if constexpr (std::is_same_v<T, godot::Vector3i>) {
                        _convert_vec3_to_vec3i(p_view, part, arr.ptr(), src_elem_count);
                        already_imported = true;
                    }
                    return;
                }
                case godot::Variant::PACKED_VECTOR2_ARRAY: {
                    const godot::PackedVector2Array arr = slot_var;
                    src_elem_count = static_cast<size_t>(arr.size());
                    src_byte_count = src_elem_count * sizeof(godot::Vector2);
                    src_bytes      = reinterpret_cast<const uint8_t*>(arr.ptr());
                    if (_try_raw_copy(part, src_bytes, src_byte_count, sizeof(godot::Vector2))) {
                        already_imported = true;
                        return;
                    }
                    if constexpr (std::is_same_v<T, godot::Vector2>) {
                        _copy_typed_elements(p_view, part, arr.ptr(), src_elem_count);
                        already_imported = true;
                    } else if constexpr (std::is_same_v<T, godot::Vector2i>) {
                        _convert_vec2_to_vec2i(p_view, part, arr.ptr(), src_elem_count);
                        already_imported = true;
                    }
                    return;
                }
                case godot::Variant::PACKED_VECTOR4_ARRAY: {
                    const godot::PackedVector4Array arr = slot_var;
                    src_elem_count = static_cast<size_t>(arr.size());
                    src_byte_count = src_elem_count * sizeof(godot::Vector4);
                    src_bytes      = reinterpret_cast<const uint8_t*>(arr.ptr());
                    if (_try_raw_copy(part, src_bytes, src_byte_count, sizeof(godot::Vector4))) {
                        already_imported = true;
                        return;
                    }
                    if constexpr (std::is_same_v<T, godot::Vector4>) {
                        _copy_typed_elements(p_view, part, arr.ptr(), src_elem_count);
                        already_imported = true;
                    }
                    return;
                }
                case godot::Variant::PACKED_COLOR_ARRAY: {
                    const godot::PackedColorArray arr = slot_var;
                    src_elem_count = static_cast<size_t>(arr.size());
                    src_byte_count = src_elem_count * sizeof(godot::Color);
                    src_bytes      = reinterpret_cast<const uint8_t*>(arr.ptr());
                    if (_try_raw_copy(part, src_bytes, src_byte_count, sizeof(godot::Color))) {
                        already_imported = true;
                        return;
                    }
                    if constexpr (std::is_same_v<T, godot::Color>) {
                        _copy_typed_elements(p_view, part, arr.ptr(), src_elem_count);
                        already_imported = true;
                    } else if constexpr (std::is_same_v<T, godot::Vector4>) {
                        const godot::Color* c_ptr = arr.ptr();
                        const size_t limit = _get_element_limit(part, src_elem_count);
                        for (size_t i = 0; i < limit; ++i) {
                            _write_view(p_view, static_cast<int64_t>(i), godot::Vector4(c_ptr[i].r, c_ptr[i].g, c_ptr[i].b, c_ptr[i].a));
                        }
                        already_imported = true;
                    }
                    return;
                }
                case godot::Variant::PACKED_FLOAT32_ARRAY: {
                    const godot::PackedFloat32Array arr = slot_var;
                    src_elem_count = static_cast<size_t>(arr.size());
                    src_byte_count = src_elem_count * sizeof(float);
                    src_bytes      = reinterpret_cast<const uint8_t*>(arr.ptr());
                    if (_try_raw_copy(part, src_bytes, src_byte_count, sizeof(float))) {
                        already_imported = true;
                        return;
                    }
                    if constexpr (std::is_same_v<T, float>) {
                        _copy_typed_elements(p_view, part, arr.ptr(), src_elem_count);
                        already_imported = true;
                    } else if constexpr (std::is_same_v<T, godot::Vector4>) {
                        // Support importing 4-float tangents or 4-float bone weights directly into Vector4
                        const float* f_ptr = arr.ptr();
                        const size_t vec4_count = src_elem_count / 4;
                        const size_t limit = _get_element_limit(part, vec4_count);
                        for (size_t i = 0; i < limit; ++i) {
                            const size_t base = i * 4;
                            _write_view(p_view, static_cast<int64_t>(i), godot::Vector4(f_ptr[base], f_ptr[base + 1], f_ptr[base + 2], f_ptr[base + 3]));
                        }
                        already_imported = true;
                    }
                    return;
                }
                case godot::Variant::PACKED_INT32_ARRAY: {
                    const godot::PackedInt32Array arr = slot_var;
                    src_elem_count = static_cast<size_t>(arr.size());
                    src_byte_count = src_elem_count * sizeof(int32_t);
                    src_bytes      = reinterpret_cast<const uint8_t*>(arr.ptr());
                    if (_try_raw_copy(part, src_bytes, src_byte_count, sizeof(int32_t))) {
                        already_imported = true;
                        return;
                    }
                    if constexpr (std::is_same_v<T, int32_t>) {
                        _copy_typed_elements(p_view, part, arr.ptr(), src_elem_count);
                        already_imported = true;
                    } else if constexpr (std::is_same_v<T, godot::Vector4i>) {
                        // Support importing 4-int32 bone indices directly into Vector4i
                        const int32_t* i_ptr = arr.ptr();
                        const size_t vec4_count = src_elem_count / 4;
                        const size_t limit = _get_element_limit(part, vec4_count);
                        for (size_t i = 0; i < limit; ++i) {
                            const size_t base = i * 4;
                            _write_view(p_view, static_cast<int64_t>(i), godot::Vector4i(i_ptr[base], i_ptr[base + 1], i_ptr[base + 2], i_ptr[base + 3]));
                        }
                        already_imported = true;
                    }
                    return;
                }
                case godot::Variant::PACKED_BYTE_ARRAY: {
                    const godot::PackedByteArray arr = slot_var;
                    src_byte_count = static_cast<size_t>(arr.size());
                    src_bytes      = arr.ptr();
                    if (_try_raw_copy(part, src_bytes, src_byte_count, 1)) {
                        already_imported = true;
                    }
                    return;
                }
                default:
                    return;
            }
        }
    }

private:
    static inline size_t _get_element_limit(const GrantPartPOD* p_part, size_t p_src_count) noexcept {
        const size_t cap = (p_part->selection.element_count > 0)
            ? static_cast<size_t>(p_part->selection.element_count)
            : (p_part->capacity_bytes / sizeof(T));
        return std::min(p_src_count, cap);
    }

    static inline bool _try_raw_copy(const GrantPartPOD* p_part, const uint8_t* p_src, size_t p_src_bytes, size_t p_src_elem_size) noexcept {
        if (!p_src || p_src_bytes == 0) return false;
        if constexpr (std::is_same_v<T, uint8_t>) {
            uint8_t* dst = p_part->raw_base_ptr;
            size_t avail = p_part->capacity_bytes;
            if (p_part->selection.mode == SelectionMode::RANGE && p_part->selection.element_count > 0) {
                const size_t offset = static_cast<size_t>(p_part->selection.start_index) * p_part->element_stride;
                if (offset < p_part->capacity_bytes) {
                    dst += offset;
                    avail = std::min(avail - offset, static_cast<size_t>(p_part->selection.element_count) * p_part->element_stride);
                }
            }
            const size_t to_copy = std::min(p_src_bytes, avail);
            if (to_copy > 0) {
                std::memcpy(dst, p_src, to_copy);
                return true;
            }
        } else if (sizeof(T) == p_src_elem_size && p_part->element_stride == sizeof(T) && p_part->selection.mode == SelectionMode::RANGE) {
            uint8_t* dst = p_part->raw_base_ptr;
            size_t avail = p_part->capacity_bytes;
            if (p_part->selection.element_count > 0) {
                const size_t offset = static_cast<size_t>(p_part->selection.start_index) * sizeof(T);
                if (offset < p_part->capacity_bytes) {
                    dst += offset;
                    avail = std::min(avail - offset, static_cast<size_t>(p_part->selection.element_count) * sizeof(T));
                }
            }
            const size_t to_copy = std::min(p_src_bytes, avail);
            if (to_copy > 0) {
                std::memcpy(dst, p_src, to_copy);
                return true;
            }
        }
        return false;
    }

    template <typename T_View>
    static inline void _copy_typed_elements(T_View& p_view, const GrantPartPOD* p_part, const T* p_src, size_t p_src_count) noexcept {
        if (!p_src) return;
        const size_t limit = _get_element_limit(p_part, p_src_count);
        for (size_t i = 0; i < limit; ++i) {
            _write_view(p_view, static_cast<int64_t>(i), p_src[i]);
        }
    }

    template <typename T_View>
    static inline void _convert_vec3_to_vec3i(T_View& p_view, const GrantPartPOD* p_part, const godot::Vector3* p_src, size_t p_src_count) noexcept {
        if (!p_src) return;
        const size_t limit = _get_element_limit(p_part, p_src_count);
        for (size_t i = 0; i < limit; ++i) {
            const godot::Vector3& v = p_src[i];
            _write_view(p_view, static_cast<int64_t>(i), godot::Vector3i(
                static_cast<int32_t>(std::lround(v.x)),
                static_cast<int32_t>(std::lround(v.y)),
                static_cast<int32_t>(std::lround(v.z))
            ));
        }
    }

    template <typename T_View>
    static inline void _convert_vec2_to_vec2i(T_View& p_view, const GrantPartPOD* p_part, const godot::Vector2* p_src, size_t p_src_count) noexcept {
        if (!p_src) return;
        const size_t limit = _get_element_limit(p_part, p_src_count);
        for (size_t i = 0; i < limit; ++i) {
            const godot::Vector2& v = p_src[i];
            _write_view(p_view, static_cast<int64_t>(i), godot::Vector2i(
                static_cast<int32_t>(std::lround(v.x)),
                static_cast<int32_t>(std::lround(v.y))
            ));
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

static_assert(sizeof(MeshSurfaceImportTransformLogic<>) == 64, "MeshSurfaceImportTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core