#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class MeshStreamSemantic : uint8_t {
    POSITION = 0,
    NORMAL   = 1,
    TANGENT  = 2,
    UV       = 3,
    COLOR8   = 4
};

/**
 * MeshStreamUnpackTransformLogic<T>
 * De-interleaves and decodes raw GPU surface vertex/attribute byte streams
 * (including Godot 4's octahedral uint16x2 normals/tangents and RGBA8 colors)
 * into planar SIMD-friendly DOD buffers (Vector3, Vector4, Vector2, Color).
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshStreamUnpackTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::FLAT | BufferLayoutType::SOA;
    static constexpr DataType required_types              =
        DataType::VECTOR2 | DataType::VECTOR2D |
        DataType::VECTOR3 | DataType::VECTOR3D |
        DataType::VECTOR4 | DataType::VECTOR4D |
        DataType::COLOR | DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Mesh Stream Unpack";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t target_planar_buffer_id = INVALID_ID; // Primary write buffer (planar attribute column)
    uint32_t source_stream_buffer_id = INVALID_ID; // Source interleaved raw byte buffer
    uint32_t stream_stride           = 16;         // Byte stride per vertex in source stream
    uint32_t element_byte_offset     = 0;          // Offset of attribute within each vertex stride
    MeshStreamSemantic semantic      = MeshStreamSemantic::POSITION;
    bool use_octahedral_compression  = false;      // Decode uint16x2 octahedral normal/tangent
    uint8_t reserved_padding[46]     = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary dst_prop;
        dst_prop["name"] = "target_planar_buffer_id";
        dst_prop["type"] = godot::Variant::INT;
        dst_prop["hint_string"] = "buffer_option";
        props.push_back(dst_prop);

        godot::Dictionary src_prop;
        src_prop["name"] = "source_stream_buffer_id";
        src_prop["type"] = godot::Variant::INT;
        src_prop["hint_string"] = "buffer_option";
        props.push_back(src_prop);

        godot::Dictionary sem_prop;
        sem_prop["name"] = "semantic";
        sem_prop["type"] = godot::Variant::INT;
        sem_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        sem_prop["hint_string"] = "Position:0,Normal:1,Tangent:2,UV:3,Color8:4";
        props.push_back(sem_prop);

        godot::Dictionary stride_prop;
        stride_prop["name"] = "stream_stride";
        stride_prop["type"] = godot::Variant::INT;
        stride_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        stride_prop["hint_string"] = "1,128,1,suffix:B";
        props.push_back(stride_prop);

        godot::Dictionary off_prop;
        off_prop["name"] = "element_byte_offset";
        off_prop["type"] = godot::Variant::INT;
        off_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        off_prop["hint_string"] = "0,124,1,suffix:B";
        props.push_back(off_prop);

        godot::Dictionary oct_prop;
        oct_prop["name"] = "use_octahedral_compression";
        oct_prop["type"] = godot::Variant::BOOL;
        oct_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(oct_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_planar_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_planar_buffer_id")) {
            target_planar_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_planar_buffer_id"]));
        } else if (p_props.has("target_buffer_id")) {
            target_planar_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("source_stream_buffer_id")) {
            source_stream_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_stream_buffer_id"]));
        } else if (p_props.has("source_buffer_id")) {
            source_stream_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        }
        if (p_props.has("stream_stride")) {
            stream_stride = std::max<uint32_t>(1u, static_cast<uint32_t>(static_cast<int64_t>(p_props["stream_stride"])));
        }
        if (p_props.has("element_byte_offset")) {
            element_byte_offset = static_cast<uint32_t>(static_cast<int64_t>(p_props["element_byte_offset"]));
        }
        if (p_props.has("semantic")) {
            semantic = static_cast<MeshStreamSemantic>(static_cast<uint8_t>(static_cast<int64_t>(p_props["semantic"])));
        }
        if (p_props.has("use_octahedral_compression")) {
            use_octahedral_compression = static_cast<bool>(p_props["use_octahedral_compression"]);
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
            const GrantPartPOD* dst_part = p_context.get_grant_part(target_planar_buffer_id);
            const GrantPartPOD* src_part = p_context.get_grant_part(source_stream_buffer_id);
            if (!dst_part || !src_part || !dst_part->raw_base_ptr || !src_part->raw_base_ptr) return;

            const uint8_t* __restrict src_base = src_part->raw_base_ptr;
            const size_t src_cap_bytes = src_part->capacity_bytes;
            const size_t stride        = static_cast<size_t>(stream_stride);
            const size_t attr_offset   = static_cast<size_t>(element_byte_offset);

            if (attr_offset >= stride || src_cap_bytes < stride) return;

            const size_t max_src_verts = src_cap_bytes / stride;
            const MemoryBufferSelectionPOD& sel = dst_part->selection;

            auto unpack_vertex = [&](int64_t idx) noexcept {
                if (idx < 0 || static_cast<size_t>(idx) >= max_src_verts) return;
                const uint8_t* ptr = src_base + (static_cast<size_t>(idx) * stride) + attr_offset;

                if constexpr (std::is_same_v<T, godot::Vector3>) {
                    if (use_octahedral_compression && (semantic == MeshStreamSemantic::NORMAL || semantic == MeshStreamSemantic::TANGENT)) {
                        uint16_t oct[2] = {0, 0};
                        std::memcpy(oct, ptr, sizeof(oct));
                        _write_view(p_view, idx, _octahedral_decode_normal(oct[0], oct[1]));
                    } else {
                        godot::Vector3 val;
                        std::memcpy(&val, ptr, sizeof(godot::Vector3));
                        _write_view(p_view, idx, val);
                    }
                } else if constexpr (std::is_same_v<T, godot::Vector4>) {
                    if (use_octahedral_compression && semantic == MeshStreamSemantic::TANGENT) {
                        uint16_t oct[2] = {0, 0};
                        std::memcpy(oct, ptr, sizeof(oct));
                        _write_view(p_view, idx, _octahedral_decode_tangent(oct[0], oct[1]));
                    } else if (semantic == MeshStreamSemantic::COLOR8) {
                        uint8_t rgba[4] = {0, 0, 0, 255};
                        std::memcpy(rgba, ptr, 4);
                        constexpr float inv_255 = 1.0f / 255.0f;
                        _write_view(p_view, idx, godot::Vector4(
                            rgba[0] * inv_255, rgba[1] * inv_255, rgba[2] * inv_255, rgba[3] * inv_255));
                    } else {
                        godot::Vector4 val;
                        std::memcpy(&val, ptr, sizeof(godot::Vector4));
                        _write_view(p_view, idx, val);
                    }
                } else if constexpr (std::is_same_v<T, godot::Color>) {
                    if (semantic == MeshStreamSemantic::COLOR8) {
                        uint8_t rgba[4] = {0, 0, 0, 255};
                        std::memcpy(rgba, ptr, 4);
                        constexpr float inv_255 = 1.0f / 255.0f;
                        _write_view(p_view, idx, godot::Color(
                            rgba[0] * inv_255, rgba[1] * inv_255, rgba[2] * inv_255, rgba[3] * inv_255));
                    } else {
                        godot::Color val;
                        std::memcpy(&val, ptr, sizeof(godot::Color));
                        _write_view(p_view, idx, val);
                    }
                } else if constexpr (std::is_same_v<T, godot::Vector2>) {
                    godot::Vector2 val;
                    std::memcpy(&val, ptr, sizeof(godot::Vector2));
                    _write_view(p_view, idx, val);
                } else {
                    T val{};
                    std::memcpy(&val, ptr, sizeof(T));
                    _write_view(p_view, idx, val);
                }
            };

            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                const int64_t limit = std::min<int64_t>(sel.capacity, static_cast<int64_t>(max_src_verts));
                for (int64_t i = 0; i < limit; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) unpack_vertex(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    unpack_vertex(sel.data.indices[i]);
                }
            } else {
                const int64_t end = std::min<int64_t>(sel.start_index + sel.element_count, static_cast<int64_t>(max_src_verts));
                for (int64_t i = sel.start_index; i < end; ++i) {
                    unpack_vertex(i);
                }
            }
        }
    }

private:
    [[nodiscard]] static inline godot::Vector3 _octahedral_decode_normal(uint16_t p_u, uint16_t p_v) noexcept {
        const float f_x = (static_cast<float>(p_u) / 65535.0f) * 2.0f - 1.0f;
        const float f_y = (static_cast<float>(p_v) / 65535.0f) * 2.0f - 1.0f;
        godot::Vector3 n(f_x, f_y, 1.0f - std::abs(f_x) - std::abs(f_y));
        const float t = std::max(-n.z, 0.0f);
        n.x += (n.x >= 0.0f) ? -t : t;
        n.y += (n.y >= 0.0f) ? -t : t;
        return n.normalized();
    }

    [[nodiscard]] static inline godot::Vector4 _octahedral_decode_tangent(uint16_t p_u, uint16_t p_v) noexcept {
        // Godot 4 octahedral tangent packs binormal sign in the high bit or second channel sign
        float sign = 1.0f;
        uint16_t v_raw = p_v;
        if (v_raw == 65535u && p_u == 65535u) {
            sign = -1.0f;
        } else {
            sign = (v_raw & 1u) ? 1.0f : -1.0f;
        }
        const godot::Vector3 t = _octahedral_decode_normal(p_u, v_raw);
        return godot::Vector4(t.x, t.y, t.z, sign);
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

static_assert(sizeof(MeshStreamUnpackTransformLogic<>) == 64, "MeshStreamUnpackTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core