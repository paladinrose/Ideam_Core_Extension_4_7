#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/vector3.hpp>
#include <algorithm>
#include <cmath>
#include <string_view>

namespace ideam::core {

enum class MeshDeformMode : uint8_t {
    RADIAL_PLASTIC_DENT = 0,
    DIRECTIONAL_GOUGE   = 1,
    RADIAL_SHOCKWAVE    = 2,
    ELASTO_PLASTIC_RELAX = 3
};

/**
 * MeshVertexDeformTransformLogic<T>
 * High-speed C++26 vertex deformation kernel for localized ballistic dents,
 * grazing plasma gouges, shockwave ripples, and elasto-plastic relaxation.
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshVertexDeformTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    // Restricted to floating-point 3D vectors (excludes Vector3i so normalized()/dot() are always valid)
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Mesh Vertex Deform";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id    = INVALID_ID;
    uint32_t normal_buffer_id    = INVALID_ID; // Optional: blends dent along -normal
    uint32_t rest_pose_buffer_id = INVALID_ID; // Optional: used for ELASTO_PLASTIC_RELAX

    // Stored as godot::Vector3 so TransformLogicSubRegistry can instantiate the class body
    // across all 17 MemoryTypes (bool, int32_t, void*, etc.) before checking required_types.
    godot::Vector3 impact_point{0.0f, 0.0f, 0.0f};      // 12B: Local-space impact center
    godot::Vector3 impact_direction{0.0f, -1.0f, 0.0f}; // 12B: Normalized projectile/blast direction

    float radius        = 1.0f;  // 4B: Influence radius R
    float intensity     = 0.25f; // 4B: Max displacement depth (meters)
    float wave_freq     = 12.0f; // 4B: Shockwave spatial frequency or gouge length
    float wave_phase    = 0.0f;  // 4B: Temporal phase or elastic recovery rate
    MeshDeformMode mode = MeshDeformMode::RADIAL_PLASTIC_DENT; // 1B
    uint8_t reserved[3] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary mode_prop;
        mode_prop["name"] = "mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Radial Plastic Dent,Directional Gouge,Radial Shockwave,Elasto-Plastic Relax";
        props.push_back(mode_prop);

        godot::Dictionary pt_prop;
        pt_prop["name"] = "impact_point";
        pt_prop["type"] = godot::Variant::VECTOR3;
        pt_prop["hint_string"] = "suffix:m";
        props.push_back(pt_prop);

        godot::Dictionary dir_prop;
        dir_prop["name"] = "impact_direction";
        dir_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(dir_prop);

        godot::Dictionary rad_prop;
        rad_prop["name"] = "radius";
        rad_prop["type"] = godot::Variant::FLOAT;
        rad_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rad_prop["hint_string"] = "0.01,20.0,0.01,suffix:m";
        props.push_back(rad_prop);

        godot::Dictionary int_prop;
        int_prop["name"] = "intensity";
        int_prop["type"] = godot::Variant::FLOAT;
        int_prop["hint_string"] = "suffix:m";
        props.push_back(int_prop);

        godot::Dictionary norm_prop;
        norm_prop["name"] = "normal_buffer_id";
        norm_prop["type"] = godot::Variant::INT;
        norm_prop["hint_string"] = "buffer_option";
        props.push_back(norm_prop);

        godot::Dictionary rest_prop;
        rest_prop["name"] = "rest_pose_buffer_id";
        rest_prop["type"] = godot::Variant::INT;
        rest_prop["hint_string"] = "buffer_option";
        props.push_back(rest_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        }
        if (p_props.has("normal_buffer_id")) {
            normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        }
        if (p_props.has("rest_pose_buffer_id")) {
            rest_pose_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rest_pose_buffer_id"]));
        }
        if (p_props.has("mode")) {
            mode = static_cast<MeshDeformMode>(static_cast<uint8_t>(static_cast<int64_t>(p_props["mode"])));
        }
        if (p_props.has("impact_point")) {
            impact_point = static_cast<godot::Vector3>(p_props["impact_point"]);
        }
        if (p_props.has("impact_direction")) {
            impact_direction = static_cast<godot::Vector3>(p_props["impact_direction"]);
        }
        if (p_props.has("radius")) {
            radius = std::max(1e-5f, static_cast<float>(p_props["radius"]));
        }
        if (p_props.has("intensity")) {
            intensity = static_cast<float>(p_props["intensity"]);
        }
        if (p_props.has("wave_freq")) {
            wave_freq = static_cast<float>(p_props["wave_freq"]);
        }
        if (p_props.has("wave_phase")) {
            wave_phase = static_cast<float>(p_props["wave_phase"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
        const GrantPartPOD* vert_part = p_context.get_grant_part(vertex_buffer_id);
        if (!vert_part) return;

        const T* __restrict normals = nullptr;
        if (normal_buffer_id != INVALID_ID) {
            if (const GrantPartPOD* norm_part = p_context.get_grant_part(normal_buffer_id)) {
                normals = reinterpret_cast<const T*>(norm_part->raw_base_ptr);
            }
        }

        const T* __restrict rest_positions = nullptr;
        if (rest_pose_buffer_id != INVALID_ID) {
            if (const GrantPartPOD* rest_part = p_context.get_grant_part(rest_pose_buffer_id)) {
                rest_positions = reinterpret_cast<const T*>(rest_part->raw_base_ptr);
            }
        }

        const float r_sq = radius * radius;
        const float inv_r_sq = 1.0f / r_sq;
        const float dt = static_cast<float>(p_context.delta);

        auto process_vertex = [&](int64_t idx) noexcept {
            T pos = _read_view(p_view, idx);
            const T n = normals ? normals[idx] : -impact_direction;

            if (mode == MeshDeformMode::ELASTO_PLASTIC_RELAX) {
                if (rest_positions) {
                    const T delta_rest = rest_positions[idx] - pos;
                    const float dist_sq = delta_rest.length_squared();
                    // Only recover elastic deformations smaller than radius (plastic yield threshold)
                    if (dist_sq > 1e-8f && dist_sq < r_sq) {
                        pos = pos + delta_rest * std::min(1.0f, intensity * dt);
                        _write_view(p_view, idx, pos);
                    }
                }
                return;
            }

            const T diff = pos - impact_point;
            const float d_sq = diff.length_squared();
            if (d_sq >= r_sq) return;

            // Smooth C2 Wyvill falloff: (1 - d^2 / R^2)^3
            const float u = 1.0f - (d_sq * inv_r_sq);
            const float falloff = u * u * u;

            if (mode == MeshDeformMode::RADIAL_PLASTIC_DENT) {
                // Blend between projectile direction and inward surface normal
                const T push_dir = (impact_direction * 0.65f - n * 0.35f).normalized();
                pos = pos + push_dir * (intensity * falloff);
            } else if (mode == MeshDeformMode::DIRECTIONAL_GOUGE) {
                // Project diff onto grazing impact_direction to form an elongated trench with raised lateral ridges
                const float axial = diff.dot(impact_direction);
                const T lateral_vec = diff - (impact_direction * axial);
                const float lat_sq = lateral_vec.length_squared();
                const float trench_profile = (1.0f - 2.5f * lat_sq * inv_r_sq);
                pos = pos - n * (intensity * falloff * trench_profile);
            } else if (mode == MeshDeformMode::RADIAL_SHOCKWAVE) {
                const float d = std::sqrt(d_sq);
                const float ripple = std::sin(d * wave_freq - wave_phase) * falloff;
                pos = pos + n * (intensity * ripple);
            }

            _write_view(p_view, idx, pos);
        };

        const MemoryBufferSelectionPOD& sel = vert_part->selection;
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
    inline T _read_view(const T_View& p_view, int64_t idx) const {
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
    inline void _write_view(const T_View& p_view, int64_t idx, const T& value) const {
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

static_assert(sizeof(MeshVertexDeformTransformLogic<godot::Vector3>) <= 64, "MeshVertexDeformTransformLogic exceeded 64B cache line!");

} // namespace ideam::core
