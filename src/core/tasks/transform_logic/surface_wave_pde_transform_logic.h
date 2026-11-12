#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * SurfaceWavePDETransformLogic<T> (M32)
 * Solves the damped 2D wave equation d^2u/dt^2 + damping * du/dt = c^2 * Laplace_Beltrami(u)
 * directly on manifold 3D triangle meshes, displacing vertices along surface normals.
 */
template <typename T = godot::Vector3>
struct alignas(64) SurfaceWavePDETransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    static constexpr int32_t MAX_WAVE_VERTS = 16384;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::VECTOR2 | DataType::FLOAT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_WAVE_VERTS * sizeof(float) * 2 + 256;

    static constexpr std::string_view display_name = "Manifold Surface Wave PDE Solver";
    static constexpr std::string_view ui_category  = "Model/SurfacePDE";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id     = INVALID_ID; // 4B: Primary target vertex buffer (Vector3)
    uint32_t normal_buffer_id     = INVALID_ID; // 4B: Vertex normal buffer (Vector3)
    uint32_t wave_state_buffer_id = INVALID_ID; // 4B: Wave state Vector2(height, velocity) buffer
    uint32_t index_buffer_id      = INVALID_ID; // 4B: Optional triangle index buffer (int32_t triplets)

    godot::Vector3 impulse_origin{0.0f, 0.0f, 0.0f}; // 12B: Local-space wave impulse center

    float wave_speed           = 4.0f;  // 4B: Phase propagation speed c
    float wave_damping         = 0.85f; // 4B: Velocity damping coefficient
    float displacement_scale   = 1.0f;  // 4B: Normal displacement multiplier
    float impulse_radius       = 0.0f;  // 4B: Radial impulse radius (0 = no impulse)
    float impulse_strength     = 0.0f;  // 4B: Impulse velocity injection amplitude
    uint8_t reserved_padding[16] = {0}; // 16B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (const GrantPartPOD* vtx_part = p_context.get_grant_part(vertex_buffer_id)) {
            if (vtx_part->selection.capacity > 0) {
                const size_t n = std::min<size_t>(static_cast<size_t>(vtx_part->selection.capacity), MAX_WAVE_VERTS);
                return n * sizeof(float) * 2 + 256;
            }
        }
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary vtx_prop;
        vtx_prop["name"] = "vertex_buffer_id";
        vtx_prop["type"] = godot::Variant::INT;
        vtx_prop["hint_string"] = "buffer_option";
        props.push_back(vtx_prop);

        godot::Dictionary nrm_prop;
        nrm_prop["name"] = "normal_buffer_id";
        nrm_prop["type"] = godot::Variant::INT;
        nrm_prop["hint_string"] = "buffer_option";
        props.push_back(nrm_prop);

        godot::Dictionary wav_prop;
        wav_prop["name"] = "wave_state_buffer_id";
        wav_prop["type"] = godot::Variant::INT;
        wav_prop["hint_string"] = "buffer_option";
        props.push_back(wav_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary spd_prop;
        spd_prop["name"] = "wave_speed";
        spd_prop["type"] = godot::Variant::FLOAT;
        spd_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        spd_prop["hint_string"] = "0.0,100.0,0.1";
        props.push_back(spd_prop);

        godot::Dictionary dmp_prop;
        dmp_prop["name"] = "wave_damping";
        dmp_prop["type"] = godot::Variant::FLOAT;
        dmp_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        dmp_prop["hint_string"] = "0.0,20.0,0.05";
        props.push_back(dmp_prop);

        godot::Dictionary scl_prop;
        scl_prop["name"] = "displacement_scale";
        scl_prop["type"] = godot::Variant::FLOAT;
        props.push_back(scl_prop);

        godot::Dictionary orig_prop;
        orig_prop["name"] = "impulse_origin";
        orig_prop["type"] = godot::Variant::VECTOR3;
        orig_prop["hint_string"] = "suffix:m";
        props.push_back(orig_prop);

        godot::Dictionary rad_prop;
        rad_prop["name"] = "impulse_radius";
        rad_prop["type"] = godot::Variant::FLOAT;
        rad_prop["hint_string"] = "suffix:m";
        props.push_back(rad_prop);

        godot::Dictionary str_prop;
        str_prop["name"] = "impulse_strength";
        str_prop["type"] = godot::Variant::FLOAT;
        props.push_back(str_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (vertex_buffer_id != INVALID_ID) ? vertex_buffer_id : wave_state_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("normal_buffer_id")) {
            normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        }
        if (p_props.has("wave_state_buffer_id")) {
            wave_state_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["wave_state_buffer_id"]));
        }
        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("wave_speed")) {
            wave_speed = std::max(0.0f, static_cast<float>(p_props["wave_speed"]));
        }
        if (p_props.has("wave_damping")) {
            wave_damping = std::max(0.0f, static_cast<float>(p_props["wave_damping"]));
        }
        if (p_props.has("displacement_scale")) {
            displacement_scale = static_cast<float>(p_props["displacement_scale"]);
        }
        if (p_props.has("impulse_origin")) {
            impulse_origin = static_cast<godot::Vector3>(p_props["impulse_origin"]);
        }
        if (p_props.has("impulse_radius")) {
            impulse_radius = std::max(0.0f, static_cast<float>(p_props["impulse_radius"]));
        }
        if (p_props.has("impulse_strength")) {
            impulse_strength = static_cast<float>(p_props["impulse_strength"]);
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
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, godot::Vector2> || std::is_same_v<T, float>) {
            if (!p_context.local_workspace) return;

            const GrantPartPOD* vtx_part  = p_context.get_grant_part(vertex_buffer_id);
            const GrantPartPOD* nrm_part  = p_context.get_grant_part(normal_buffer_id);
            const GrantPartPOD* wave_part = p_context.get_grant_part(wave_state_buffer_id);
            if (!vtx_part || !nrm_part || !wave_part || !vtx_part->raw_base_ptr || !nrm_part->raw_base_ptr || !wave_part->raw_base_ptr) return;

            const GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID) ? p_context.get_grant_part(index_buffer_id) : nullptr;

            godot::Vector3* __restrict verts      = reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr);
            const godot::Vector3* __restrict norms = reinterpret_cast<const godot::Vector3*>(nrm_part->raw_base_ptr);
            godot::Vector2* __restrict hv          = reinterpret_cast<godot::Vector2*>(wave_part->raw_base_ptr);

            const int32_t raw_n = (vtx_part->selection.element_count > 0)
                ? static_cast<int32_t>(vtx_part->selection.element_count)
                : static_cast<int32_t>(vtx_part->capacity_bytes / sizeof(godot::Vector3));
            const int32_t n_verts = std::min<int32_t>({
                raw_n,
                static_cast<int32_t>(nrm_part->capacity_bytes / sizeof(godot::Vector3)),
                static_cast<int32_t>(wave_part->capacity_bytes / sizeof(godot::Vector2)),
                MAX_WAVE_VERTS
            });
            if (n_verts <= 0) return;

            float* lap_u = static_cast<float*>(p_context.local_workspace);
            float* deg_u = lap_u + n_verts;
            for (int32_t i = 0; i < n_verts; ++i) {
                lap_u[i] = 0.0f;
                deg_u[i] = 0.0f;
            }

            if (idx_part && idx_part->raw_base_ptr) {
                const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
                const int32_t n_tris = static_cast<int32_t>(((idx_part->selection.element_count > 0)
                    ? idx_part->selection.element_count
                    : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t))) / 3);
                for (int32_t t = 0; t < n_tris; ++t) {
                    const int32_t i0 = indices[t * 3 + 0];
                    const int32_t i1 = indices[t * 3 + 1];
                    const int32_t i2 = indices[t * 3 + 2];
                    if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;

                    const float u0 = hv[i0].x, u1 = hv[i1].x, u2 = hv[i2].x;
                    lap_u[i0] += (u1 - u0) + (u2 - u0); deg_u[i0] += 2.0f;
                    lap_u[i1] += (u0 - u1) + (u2 - u1); deg_u[i1] += 2.0f;
                    lap_u[i2] += (u0 - u2) + (u1 - u2); deg_u[i2] += 2.0f;
                }
            } else {
                for (int32_t i = 0; i < n_verts; ++i) {
                    const int32_t prev = (i + n_verts - 1) % n_verts;
                    const int32_t next = (i + 1) % n_verts;
                    lap_u[i] = (hv[prev].x - hv[i].x) + (hv[next].x - hv[i].x);
                    deg_u[i] = 2.0f;
                }
            }

            const float dt = (p_context.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_context.delta)) : (1.0f / 60.0f);
            const float c2 = wave_speed * wave_speed;
            const float r2 = impulse_radius * impulse_radius;

            for (int32_t i = 0; i < n_verts; ++i) {
                const float old_h = hv[i].x;
                float v = hv[i].y;

                if (impulse_radius > 0.0f && std::abs(impulse_strength) > 1e-6f) {
                    const float d2 = (verts[i] - impulse_origin).length_squared();
                    if (d2 < r2) {
                        const float w = 1.0f - std::sqrt(d2) / impulse_radius;
                        v += impulse_strength * w * dt;
                    }
                }

                const float avg_lap = (deg_u[i] > 0.0f) ? (lap_u[i] / deg_u[i]) : 0.0f;
                const float accel = c2 * avg_lap - wave_damping * v;
                v += accel * dt;
                const float new_h = old_h + v * dt;

                hv[i] = godot::Vector2(new_h, v);
                verts[i] += norms[i] * ((new_h - old_h) * displacement_scale);
            }
        }
    }
};

static_assert(sizeof(SurfaceWavePDETransformLogic<godot::Vector3>) == 64, "SurfaceWavePDETransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SurfaceWavePDETransformLogic<godot::Vector3>>, "SurfaceWavePDETransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core