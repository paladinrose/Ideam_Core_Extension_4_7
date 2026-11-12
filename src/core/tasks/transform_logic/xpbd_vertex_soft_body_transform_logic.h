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
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * XPBDVertexSoftBodyTransformLogic<T> (M31)
 * Extended Position-Based Dynamics (XPBD) vertex solver with time-step invariant compliance
 * (alpha_tilde = compliance / dt^2), solving edge distance constraints, rest-pose shape matching,
 * and internal volume overpressure.
 */
template <typename T = godot::Vector3>
struct alignas(64) XPBDVertexSoftBodyTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    static constexpr int32_t MAX_XPBD_VERTS = 16384;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_XPBD_VERTS * sizeof(godot::Vector3) + 256;

    static constexpr std::string_view display_name = "XPBD Vertex Soft-Body Solver";
    static constexpr std::string_view ui_category  = "Model/Physics";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t position_buffer_id  = INVALID_ID; // 4B: Primary target vertex position buffer (Vector3)
    uint32_t velocity_buffer_id  = INVALID_ID; // 4B: Vertex velocity buffer (Vector3)
    uint32_t rest_pose_buffer_id = INVALID_ID; // 4B: Optional rest-pose position buffer (Vector3)
    uint32_t index_buffer_id     = INVALID_ID; // 4B: Optional triangle index buffer (int32_t triplets)

    godot::Vector3 gravity{0.0f, -9.81f, 0.0f}; // 12B: External acceleration vector

    float edge_compliance          = 0.0001f; // 4B: Inverse stiffness compliance alpha (m/N)
    float shape_matching_stiffness = 0.15f;   // 4B: Rest-pose shape restoration blend in [0, 1]
    float volume_pressure          = 0.0f;    // 4B: Radial internal overpressure impulse
    float velocity_damping         = 0.02f;   // 4B: Per-step velocity damping factor in [0, 1]
    int32_t solver_iterations      = 4;       // 4B: Gauss-Seidel constraint projection sub-steps
    uint8_t reserved_padding[16]   = {0};     // 16B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (const GrantPartPOD* pos_part = p_context.get_grant_part(position_buffer_id)) {
            if (pos_part->selection.capacity > 0) {
                const size_t n = std::min<size_t>(static_cast<size_t>(pos_part->selection.capacity), MAX_XPBD_VERTS);
                return n * sizeof(godot::Vector3) + 256;
            }
        }
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary pos_prop;
        pos_prop["name"] = "position_buffer_id";
        pos_prop["type"] = godot::Variant::INT;
        pos_prop["hint_string"] = "buffer_option";
        props.push_back(pos_prop);

        godot::Dictionary vel_prop;
        vel_prop["name"] = "velocity_buffer_id";
        vel_prop["type"] = godot::Variant::INT;
        vel_prop["hint_string"] = "buffer_option";
        props.push_back(vel_prop);

        godot::Dictionary rest_prop;
        rest_prop["name"] = "rest_pose_buffer_id";
        rest_prop["type"] = godot::Variant::INT;
        rest_prop["hint_string"] = "buffer_option";
        props.push_back(rest_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary grav_prop;
        grav_prop["name"] = "gravity";
        grav_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(grav_prop);

        godot::Dictionary iter_prop;
        iter_prop["name"] = "solver_iterations";
        iter_prop["type"] = godot::Variant::INT;
        iter_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        iter_prop["hint_string"] = "1,32,1";
        props.push_back(iter_prop);

        godot::Dictionary comp_prop;
        comp_prop["name"] = "edge_compliance";
        comp_prop["type"] = godot::Variant::FLOAT;
        comp_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        comp_prop["hint_string"] = "0.0,1.0,0.0001";
        props.push_back(comp_prop);

        godot::Dictionary shp_prop;
        shp_prop["name"] = "shape_matching_stiffness";
        shp_prop["type"] = godot::Variant::FLOAT;
        shp_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        shp_prop["hint_string"] = "0.0,1.0,0.01";
        props.push_back(shp_prop);

        godot::Dictionary pres_prop;
        pres_prop["name"] = "volume_pressure";
        pres_prop["type"] = godot::Variant::FLOAT;
        props.push_back(pres_prop);

        godot::Dictionary damp_prop;
        damp_prop["name"] = "velocity_damping";
        damp_prop["type"] = godot::Variant::FLOAT;
        damp_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        damp_prop["hint_string"] = "0.0,1.0,0.005";
        props.push_back(damp_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return position_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("position_buffer_id")) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        } else if (p_props.has("vertex_buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("velocity_buffer_id")) {
            velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["velocity_buffer_id"]));
        }
        if (p_props.has("rest_pose_buffer_id")) {
            rest_pose_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rest_pose_buffer_id"]));
        }
        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("gravity")) {
            gravity = static_cast<godot::Vector3>(p_props["gravity"]);
        }
        if (p_props.has("solver_iterations")) {
            solver_iterations = std::clamp<int32_t>(static_cast<int32_t>(p_props["solver_iterations"]), 1, 32);
        }
        if (p_props.has("edge_compliance")) {
            edge_compliance = std::max(0.0f, static_cast<float>(p_props["edge_compliance"]));
        }
        if (p_props.has("shape_matching_stiffness")) {
            shape_matching_stiffness = std::clamp(static_cast<float>(p_props["shape_matching_stiffness"]), 0.0f, 1.0f);
        }
        if (p_props.has("volume_pressure")) {
            volume_pressure = static_cast<float>(p_props["volume_pressure"]);
        }
        if (p_props.has("velocity_damping")) {
            velocity_damping = std::clamp(static_cast<float>(p_props["velocity_damping"]), 0.0f, 1.0f);
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
            if (!p_context.local_workspace) return;

            const GrantPartPOD* pos_part = p_context.get_grant_part(position_buffer_id);
            const GrantPartPOD* vel_part = p_context.get_grant_part(velocity_buffer_id);
            if (!pos_part || !vel_part || !pos_part->raw_base_ptr || !vel_part->raw_base_ptr) return;

            const GrantPartPOD* rest_part = (rest_pose_buffer_id != INVALID_ID) ? p_context.get_grant_part(rest_pose_buffer_id) : nullptr;
            const GrantPartPOD* idx_part  = (index_buffer_id != INVALID_ID)     ? p_context.get_grant_part(index_buffer_id)     : nullptr;

            T* __restrict vel = reinterpret_cast<T*>(vel_part->raw_base_ptr);
            const int32_t raw_n = (pos_part->selection.element_count > 0)
                ? static_cast<int32_t>(pos_part->selection.element_count)
                : static_cast<int32_t>(pos_part->capacity_bytes / sizeof(T));
            const int32_t n_verts = std::min<int32_t>({
                raw_n,
                static_cast<int32_t>(vel_part->capacity_bytes / sizeof(T)),
                MAX_XPBD_VERTS
            });
            if (n_verts <= 0) return;

            const T* __restrict rest_pos = (rest_part && rest_part->raw_base_ptr && rest_part->capacity_bytes >= static_cast<size_t>(n_verts) * sizeof(T))
                ? reinterpret_cast<const T*>(rest_part->raw_base_ptr)
                : nullptr;

            T* __restrict pred_pos = static_cast<T*>(p_context.local_workspace);
            const float dt = (p_context.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_context.delta)) : (1.0f / 60.0f);
            const float damp = std::max(0.0f, 1.0f - velocity_damping);
            const float alpha_tilde = edge_compliance / (dt * dt);

            // 1. Symplectic Euler prediction
            T centroid(0.0f, 0.0f, 0.0f);
            for (int32_t i = 0; i < n_verts; ++i) {
                const T cur_p = _read_view(p_view, i);
                vel[i] = (vel[i] + gravity * dt) * damp;
                pred_pos[i] = cur_p + vel[i] * dt;
                centroid += pred_pos[i];
            }
            centroid /= static_cast<float>(n_verts);

            // 2. XPBD Constraint Projection Iterations
            const int32_t* __restrict indices = (idx_part && idx_part->raw_base_ptr)
                ? reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr)
                : nullptr;
            const int32_t n_tris = indices
                ? static_cast<int32_t>(((idx_part->selection.element_count > 0)
                    ? idx_part->selection.element_count
                    : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t))) / 3)
                : 0;

            for (int32_t iter = 0; iter < solver_iterations; ++iter) {
                if (indices && n_tris > 0) {
                    for (int32_t t = 0; t < n_tris; ++t) {
                        const int32_t i0 = indices[t * 3 + 0];
                        const int32_t i1 = indices[t * 3 + 1];
                        const int32_t i2 = indices[t * 3 + 2];
                        if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;

                        const int32_t pairs[3][2] = {{i0, i1}, {i1, i2}, {i2, i0}};
                        for (int e = 0; e < 3; ++e) {
                            const int32_t a = pairs[e][0];
                            const int32_t b = pairs[e][1];
                            const T d = pred_pos[b] - pred_pos[a];
                            const float len = d.length();
                            if (len < 1e-6f) continue;

                            const float rest_len = rest_pos ? (rest_pos[b] - rest_pos[a]).length() : len;
                            const float C = len - rest_len;
                            const float dlambda = -C / (2.0f + alpha_tilde);
                            const T corr = (d / len) * dlambda;
                            pred_pos[a] -= corr;
                            pred_pos[b] += corr;
                        }
                    }
                }

                for (int32_t i = 0; i < n_verts; ++i) {
                    if (rest_pos && shape_matching_stiffness > 0.0f) {
                        pred_pos[i] = pred_pos[i].lerp(rest_pos[i], shape_matching_stiffness / static_cast<float>(solver_iterations));
                    }
                    if (std::abs(volume_pressure) > 1e-6f) {
                        const T radial = (pred_pos[i] - centroid).normalized();
                        pred_pos[i] += radial * (volume_pressure * dt / static_cast<float>(solver_iterations));
                    }
                }
            }

            // 3. Velocity update & position commit through view
            const float inv_dt = 1.0f / dt;
            for (int32_t i = 0; i < n_verts; ++i) {
                const T prev_p = _read_view(p_view, i);
                vel[i] = (pred_pos[i] - prev_p) * inv_dt;
                _write_view(p_view, i, pred_pos[i]);
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
    inline T _read_view(const T_View& p_view, int64_t idx) const noexcept {
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
    inline void _write_view(const T_View& p_view, int64_t idx, const T& value) const noexcept {
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

static_assert(sizeof(XPBDVertexSoftBodyTransformLogic<godot::Vector3>) == 64, "XPBDVertexSoftBodyTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<XPBDVertexSoftBodyTransformLogic<godot::Vector3>>, "XPBDVertexSoftBodyTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core