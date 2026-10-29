#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "mesh_stream_unpack_transform_logic.h"

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

/**
 * MeshStreamPackTransformLogic<T>
 * Encodes and interleaves planar DOD buffers (Vector3 positions/normals, Vector4 tangents,
 * Vector2 UVs, or Colors—including fast branchless octahedral uint16x2 encoding) into a
 * raw GPU staging byte stream for MeshVertexRegionCommit / MeshAttributeRegionCommit.
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshStreamPackTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::FLAT | BufferLayoutType::SOA | BufferLayoutType::AOS;
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

    static constexpr std::string_view display_name = "Mesh Stream Pack";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t target_stream_buffer_id = INVALID_ID; // Target interleaved staging byte buffer
    uint32_t source_planar_buffer_id = INVALID_ID; // Source planar DOD attribute buffer
    uint32_t stream_stride           = 16;         // Byte stride per vertex in target stream
    uint32_t element_byte_offset     = 0;          // Offset of attribute within each vertex stride
    MeshStreamSemantic semantic      = MeshStreamSemantic::POSITION;
    bool use_octahedral_compression  = false;      // Encode normal/tangent as octahedral uint16x2
    uint8_t reserved_padding[46]     = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary dst_prop;
        dst_prop["name"] = "target_stream_buffer_id";
        dst_prop["type"] = godot::Variant::INT;
        dst_prop["hint_string"] = "buffer_option";
        props.push_back(dst_prop);

        godot::Dictionary src_prop;
        src_prop["name"] = "source_planar_buffer_id";
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
        return (source_planar_buffer_id != INVALID_ID) ? source_planar_buffer_id : target_stream_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_stream_buffer_id")) {
            target_stream_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_stream_buffer_id"]));
        } else if (p_props.has("target_buffer_id")) {
            target_stream_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("source_planar_buffer_id")) {
            source_planar_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_planar_buffer_id"]));
        } else if (p_props.has("source_buffer_id")) {
            source_planar_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
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
            const GrantPartPOD* dst_part = p_context.get_grant_part(target_stream_buffer_id);
            const GrantPartPOD* src_part = p_context.get_grant_part(source_planar_buffer_id);
            if (!dst_part || !src_part || !dst_part->raw_base_ptr || !src_part->raw_base_ptr) return;

            uint8_t* __restrict dst_base = dst_part->raw_base_ptr;
            const size_t dst_cap_bytes   = dst_part->capacity_bytes;
            const size_t stride          = static_cast<size_t>(stream_stride);
            const size_t attr_offset     = static_cast<size_t>(element_byte_offset);

            if (attr_offset >= stride || dst_cap_bytes < stride) return;

            const size_t max_dst_verts = dst_cap_bytes / stride;
            const size_t max_src_verts = src_part->capacity_bytes / sizeof(T);
            const size_t max_verts     = std::min(max_dst_verts, max_src_verts);

            const T* __restrict src_raw = reinterpret_cast<const T*>(src_part->raw_base_ptr);
            const MemoryBufferSelectionPOD& sel = src_part->selection;

            auto pack_vertex = [&](int64_t idx) noexcept {
                if (idx < 0 || static_cast<size_t>(idx) >= max_verts) return;
                uint8_t* dst_ptr = dst_base + (static_cast<size_t>(idx) * stride) + attr_offset;
                const T& val     = src_raw[idx];

                if constexpr (std::is_same_v<T, godot::Vector3>) {
                    if (use_octahedral_compression && (semantic == MeshStreamSemantic::NORMAL || semantic == MeshStreamSemantic::TANGENT)) {
                        const uint32_t packed = _octahedral_encode_normal(val);
                        std::memcpy(dst_ptr, &packed, sizeof(uint32_t));
                    } else {
                        std::memcpy(dst_ptr, &val, sizeof(godot::Vector3));
                    }
                } else if constexpr (std::is_same_v<T, godot::Vector4>) {
                    if (use_octahedral_compression && semantic == MeshStreamSemantic::TANGENT) {
                        const uint32_t packed = _octahedral_encode_tangent(val);
                        std::memcpy(dst_ptr, &packed, sizeof(uint32_t));
                    } else if (semantic == MeshStreamSemantic::COLOR8) {
                        const uint8_t rgba[4] = {
                            static_cast<uint8_t>(std::clamp(val.x * 255.0f + 0.5f, 0.0f, 255.0f)),
                            static_cast<uint8_t>(std::clamp(val.y * 255.0f + 0.5f, 0.0f, 255.0f)),
                            static_cast<uint8_t>(std::clamp(val.z * 255.0f + 0.5f, 0.0f, 255.0f)),
                            static_cast<uint8_t>(std::clamp(val.w * 255.0f + 0.5f, 0.0f, 255.0f))
                        };
                        std::memcpy(dst_ptr, rgba, 4);
                    } else {
                        std::memcpy(dst_ptr, &val, sizeof(godot::Vector4));
                    }
                } else if constexpr (std::is_same_v<T, godot::Color>) {
                    if (semantic == MeshStreamSemantic::COLOR8) {
                        const uint8_t rgba[4] = {
                            static_cast<uint8_t>(std::clamp(val.r * 255.0f + 0.5f, 0.0f, 255.0f)),
                            static_cast<uint8_t>(std::clamp(val.g * 255.0f + 0.5f, 0.0f, 255.0f)),
                            static_cast<uint8_t>(std::clamp(val.b * 255.0f + 0.5f, 0.0f, 255.0f)),
                            static_cast<uint8_t>(std::clamp(val.a * 255.0f + 0.5f, 0.0f, 255.0f))
                        };
                        std::memcpy(dst_ptr, rgba, 4);
                    } else {
                        std::memcpy(dst_ptr, &val, sizeof(godot::Color));
                    }
                } else {
                    std::memcpy(dst_ptr, &val, sizeof(T));
                }
            };

            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                const int64_t limit = std::min<int64_t>(sel.capacity, static_cast<int64_t>(max_verts));
                for (int64_t i = 0; i < limit; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) pack_vertex(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    pack_vertex(sel.data.indices[i]);
                }
            } else {
                const int64_t end = std::min<int64_t>(sel.start_index + sel.element_count, static_cast<int64_t>(max_verts));
                for (int64_t i = sel.start_index; i < end; ++i) {
                    pack_vertex(i);
                }
            }
        }
    }

private:
    [[nodiscard]] static inline uint32_t _octahedral_encode_normal(const godot::Vector3& p_n) noexcept {
        const float l1 = std::abs(p_n.x) + std::abs(p_n.y) + std::abs(p_n.z);
        if (l1 < 1e-8f) return 0x7FFF7FFFu;

        float nx = p_n.x / l1;
        float ny = p_n.y / l1;
        if (p_n.z < 0.0f) {
            const float ox = (1.0f - std::abs(ny)) * (nx >= 0.0f ? 1.0f : -1.0f);
            const float oy = (1.0f - std::abs(nx)) * (ny >= 0.0f ? 1.0f : -1.0f);
            nx = ox;
            ny = oy;
        }
        const uint16_t u = static_cast<uint16_t>(std::clamp((nx * 0.5f + 0.5f) * 65535.0f + 0.5f, 0.0f, 65535.0f));
        const uint16_t v = static_cast<uint16_t>(std::clamp((ny * 0.5f + 0.5f) * 65535.0f + 0.5f, 0.0f, 65535.0f));
        return static_cast<uint32_t>(u) | (static_cast<uint32_t>(v) << 16);
    }

    [[nodiscard]] static inline uint32_t _octahedral_encode_tangent(const godot::Vector4& p_t) noexcept {
        uint32_t packed = _octahedral_encode_normal(godot::Vector3(p_t.x, p_t.y, p_t.z));
        uint16_t u = static_cast<uint16_t>(packed & 0xFFFFu);
        uint16_t v = static_cast<uint16_t>((packed >> 16) & 0xFFFFu);
        // Preserve binormal sign in LSB of v channel
        if (p_t.w >= 0.0f) {
            v |= 1u;
        } else {
            v &= ~1u;
        }
        return static_cast<uint32_t>(u) | (static_cast<uint32_t>(v) << 16);
    }
};

static_assert(sizeof(MeshStreamPackTransformLogic<>) == 64, "MeshStreamPackTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core