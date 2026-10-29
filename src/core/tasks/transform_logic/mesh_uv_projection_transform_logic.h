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
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class MeshUVProjectionMode : uint8_t {
    PLANAR               = 0, // Projects 3D positions onto orthogonal plane of dominant axis (0=X, 1=Y, 2=Z)
    TRIPLANAR_BOX        = 1, // Selects projection plane per-vertex using dominant axis of vertex normal
    CYLINDRICAL          = 2, // Azimuthal angle around axis + axial height
    SPHERICAL            = 3, // Equirectangular (longitude/latitude) projection around projection_origin
    AFFINE_SCROLL_ROTATE = 4  // 2D scale, rotation, and animated UV scrolling on existing UV coordinates
};

/**
 * MeshUVProjectionTransformLogic<T>
 * Computes or transforms Vector2 UV coordinates in DOD memory from 3D vertex positions
 * and normals using Planar, Triplanar Box, Cylindrical, Spherical, or Affine Scroll/Rotate modes.
 */
template <typename T = godot::Vector2>
struct alignas(64) MeshUVProjectionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR2 | DataType::VECTOR2D;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Mesh UV Projection";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t uv_buffer_id     = INVALID_ID; // 4B: Primary target UV buffer (Vector2)
    uint32_t vertex_buffer_id = INVALID_ID; // 4B: Source 3D vertex positions (Vector3)
    uint32_t normal_buffer_id = INVALID_ID; // 4B: Optional vertex normals (Vector3, used by TRIPLANAR_BOX)

    godot::Vector3 projection_origin{0.0f, 0.0f, 0.0f}; // 12B: Local-space projection center
    godot::Vector2 uv_scale{1.0f, 1.0f};                // 8B:  UV tiling multiplier
    godot::Vector2 uv_offset{0.0f, 0.0f};               // 8B:  Static UV offset or scroll velocity (when animate_scroll == true)
    float rotation_rad        = 0.0f;                   // 4B:  2D UV rotation angle in radians

    MeshUVProjectionMode mode = MeshUVProjectionMode::TRIPLANAR_BOX; // 1B
    uint8_t dominant_axis     = 1;                      // 1B: 0 = X, 1 = Y (Up), 2 = Z
    bool animate_scroll       = false;                  // 1B: Multiply uv_offset by p_context.delta and accumulate
    uint8_t reserved_padding[17] = {0};                 // 17B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary uv_prop;
        uv_prop["name"] = "uv_buffer_id";
        uv_prop["type"] = godot::Variant::INT;
        uv_prop["hint_string"] = "buffer_option";
        props.push_back(uv_prop);

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

        godot::Dictionary mode_prop;
        mode_prop["name"] = "mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Planar:0,Triplanar Box:1,Cylindrical:2,Spherical:3,Affine Scroll & Rotate:4";
        props.push_back(mode_prop);

        godot::Dictionary axis_prop;
        axis_prop["name"] = "dominant_axis";
        axis_prop["type"] = godot::Variant::INT;
        axis_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        axis_prop["hint_string"] = "X Axis:0,Y Axis:1,Z Axis:2";
        props.push_back(axis_prop);

        godot::Dictionary orig_prop;
        orig_prop["name"] = "projection_origin";
        orig_prop["type"] = godot::Variant::VECTOR3;
        orig_prop["hint_string"] = "suffix:m";
        props.push_back(orig_prop);

        godot::Dictionary scl_prop;
        scl_prop["name"] = "uv_scale";
        scl_prop["type"] = godot::Variant::VECTOR2;
        props.push_back(scl_prop);

        godot::Dictionary off_prop;
        off_prop["name"] = "uv_offset";
        off_prop["type"] = godot::Variant::VECTOR2;
        props.push_back(off_prop);

        godot::Dictionary rot_prop;
        rot_prop["name"] = "rotation_rad";
        rot_prop["type"] = godot::Variant::FLOAT;
        rot_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rot_prop["hint_string"] = "-6.283185,6.283185,0.01,suffix:rad";
        props.push_back(rot_prop);

        godot::Dictionary anim_prop;
        anim_prop["name"] = "animate_scroll";
        anim_prop["type"] = godot::Variant::BOOL;
        anim_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(anim_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return uv_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("uv_buffer_id")) {
            uv_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["uv_buffer_id"]));
        } else if (p_props.has("target_buffer_id")) {
            uv_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id")) {
            uv_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        }
        if (p_props.has("normal_buffer_id")) {
            normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        }
        if (p_props.has("mode")) {
            mode = static_cast<MeshUVProjectionMode>(static_cast<uint8_t>(static_cast<int64_t>(p_props["mode"])));
        }
        if (p_props.has("dominant_axis")) {
            dominant_axis = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["dominant_axis"]), 0, 2));
        }
        if (p_props.has("projection_origin")) {
            projection_origin = static_cast<godot::Vector3>(p_props["projection_origin"]);
        }
        if (p_props.has("uv_scale")) {
            uv_scale = static_cast<godot::Vector2>(p_props["uv_scale"]);
        }
        if (p_props.has("uv_offset")) {
            uv_offset = static_cast<godot::Vector2>(p_props["uv_offset"]);
        }
        if (p_props.has("rotation_rad")) {
            rotation_rad = static_cast<float>(p_props["rotation_rad"]);
        }
        if (p_props.has("animate_scroll")) {
            animate_scroll = static_cast<bool>(p_props["animate_scroll"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_uv_view) const {
        if constexpr (std::is_same_v<T, godot::Vector2>) {
            const GrantPartPOD* uv_part = p_context.get_grant_part(uv_buffer_id);
            if (!uv_part || !uv_part->raw_base_ptr) return;

            const GrantPartPOD* vert_part = (vertex_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(vertex_buffer_id)
                : nullptr;
            const godot::Vector3* __restrict vertices = (vert_part && vert_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector3*>(vert_part->raw_base_ptr)
                : nullptr;

            const GrantPartPOD* norm_part = (normal_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(normal_buffer_id)
                : nullptr;
            const godot::Vector3* __restrict normals = (norm_part && norm_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector3*>(norm_part->raw_base_ptr)
                : nullptr;

            if (mode != MeshUVProjectionMode::AFFINE_SCROLL_ROTATE && !vertices) return;

            const float cos_r = std::cos(rotation_rad);
            const float sin_r = std::sin(rotation_rad);
            const float dt    = static_cast<float>(p_context.delta);
            const godot::Vector2 eff_offset = animate_scroll
                ? godot::Vector2(uv_offset.x * dt, uv_offset.y * dt)
                : uv_offset;

            constexpr float inv_tau = 1.0f / (2.0f * std::numbers::pi_v<float>);
            constexpr float inv_pi  = 1.0f / std::numbers::pi_v<float>;

            auto apply_2d_xform = [&](godot::Vector2 uv) noexcept -> godot::Vector2 {
                uv.x *= uv_scale.x;
                uv.y *= uv_scale.y;
                if (std::abs(rotation_rad) > 1e-6f) {
                    const float rx = uv.x * cos_r - uv.y * sin_r;
                    const float ry = uv.x * sin_r + uv.y * cos_r;
                    uv.x = rx;
                    uv.y = ry;
                }
                uv.x += eff_offset.x;
                uv.y += eff_offset.y;
                return uv;
            };

            auto project_planar = [](const godot::Vector3& p, uint8_t axis) noexcept -> godot::Vector2 {
                if (axis == 0) return godot::Vector2(p.z, p.y); // YZ plane
                if (axis == 1) return godot::Vector2(p.x, p.z); // XZ plane
                return godot::Vector2(p.x, p.y);                // XY plane
            };

            auto process_vertex = [&](int64_t idx) noexcept {
                if (mode == MeshUVProjectionMode::AFFINE_SCROLL_ROTATE) {
                    const godot::Vector2 cur_uv = _read_view(p_uv_view, idx);
                    _write_view(p_uv_view, idx, apply_2d_xform(cur_uv));
                    return;
                }

                const godot::Vector3 rel = vertices[idx] - projection_origin;
                godot::Vector2 base_uv{0.0f, 0.0f};

                switch (mode) {
                    case MeshUVProjectionMode::PLANAR: {
                        base_uv = project_planar(rel, dominant_axis);
                        break;
                    }
                    case MeshUVProjectionMode::TRIPLANAR_BOX: {
                        const godot::Vector3 n = normals ? normals[idx] : rel;
                        const float ax = std::abs(n.x);
                        const float ay = std::abs(n.y);
                        const float az = std::abs(n.z);
                        const uint8_t box_axis = (ax >= ay && ax >= az) ? 0 : ((ay >= az) ? 1 : 2);
                        base_uv = project_planar(rel, box_axis);
                        break;
                    }
                    case MeshUVProjectionMode::CYLINDRICAL: {
                        float u_angle = 0.0f;
                        float v_height = 0.0f;
                        if (dominant_axis == 0) {
                            u_angle  = std::atan2(rel.z, rel.y) * inv_tau + 0.5f;
                            v_height = rel.x;
                        } else if (dominant_axis == 1) {
                            u_angle  = std::atan2(rel.x, rel.z) * inv_tau + 0.5f;
                            v_height = rel.y;
                        } else {
                            u_angle  = std::atan2(rel.x, rel.y) * inv_tau + 0.5f;
                            v_height = rel.z;
                        }
                        base_uv = godot::Vector2(u_angle, v_height);
                        break;
                    }
                    case MeshUVProjectionMode::SPHERICAL: {
                        const float len = rel.length();
                        if (len > 1e-6f) {
                            const godot::Vector3 dir = rel * (1.0f / len);
                            const float u = std::atan2(dir.x, dir.z) * inv_tau + 0.5f;
                            const float v = std::asin(std::clamp(dir.y, -1.0f, 1.0f)) * inv_pi + 0.5f;
                            base_uv = godot::Vector2(u, v);
                        }
                        break;
                    }
                    default:
                        break;
                }

                _write_view(p_uv_view, idx, apply_2d_xform(base_uv));
            };

            const MemoryBufferSelectionPOD& sel = uv_part->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                for (int64_t i = 0; i < sel.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) process_vertex(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    process_vertex(sel.data.indices[i]);
                }
            } else {
                const int64_t end = sel.start_index + sel.element_count;
                for (int64_t i = sel.start_index; i < end; ++i) {
                    process_vertex(i);
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

static_assert(sizeof(MeshUVProjectionTransformLogic<>) == 64, "MeshUVProjectionTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core