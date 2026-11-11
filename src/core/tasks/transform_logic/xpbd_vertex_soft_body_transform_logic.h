#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * M31: XPBDVertexSoftBodyTransformLogic
 * Extended Position-Based Dynamics (XPBD) vertex solver with time-step invariant compliance
 * (alpha_tilde = compliance / dt^2), solving edge distance constraints, rest-pose shape matching,
 * and internal volume overpressure.
 */
struct XPBDVertexSoftBodyTransformLogic {
    static constexpr std::string_view ui_name = "XPBD Vertex Soft-Body Solver";
    static constexpr std::string_view ui_category = "Model/Physics";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SWAP,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::INT32
    };

    static constexpr int32_t MAX_XPBD_VERTS = 16384;

    godot::Vector3 gravity = godot::Vector3(0.0f, -9.81f, 0.0f);
    int32_t solver_iterations = 4;
    float edge_compliance = 0.0001f;
    float shape_matching_stiffness = 0.15f;
    float volume_pressure = 0.0f;
    float velocity_damping = 0.02f;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("gravity", godot::Variant::VECTOR3, gravity);
        add_prop("solver_iterations", godot::Variant::INT, solver_iterations);
        add_prop("edge_compliance", godot::Variant::FLOAT, edge_compliance);
        add_prop("shape_matching_stiffness", godot::Variant::FLOAT, shape_matching_stiffness);
        add_prop("volume_pressure", godot::Variant::FLOAT, volume_pressure);
        add_prop("velocity_damping", godot::Variant::FLOAT, velocity_damping);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("gravity")) gravity = p_props["gravity"];
        if (p_props.has("solver_iterations")) solver_iterations = std::clamp<int32_t>(static_cast<int32_t>(p_props["solver_iterations"]), 1, 32);
        if (p_props.has("edge_compliance")) edge_compliance = std::max(0.0f, static_cast<float>(p_props["edge_compliance"]));
        if (p_props.has("shape_matching_stiffness")) shape_matching_stiffness = std::clamp(static_cast<float>(p_props["shape_matching_stiffness"]), 0.0f, 1.0f);
        if (p_props.has("volume_pressure")) volume_pressure = static_cast<float>(p_props["volume_pressure"]);
        if (p_props.has("velocity_damping")) velocity_damping = std::clamp(static_cast<float>(p_props["velocity_damping"]), 0.0f, 1.0f);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_XPBD_VERTS * sizeof(godot::Vector3) + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_XPBD_VERTS * sizeof(godot::Vector3) + 256;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2 || !p_ctx.local_workspace) return;

        GrantPartPOD& pos_part = p_ctx.grant->parts[0];
        GrantPartPOD& vel_part = p_ctx.grant->parts[1];
        if (!pos_part.raw_base_ptr || !vel_part.raw_base_ptr) return;

        const GrantPartPOD* rest_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;
        const GrantPartPOD* idx_part = (p_ctx.grant->part_count > 3 && p_ctx.grant->parts[3].raw_base_ptr)
            ? &p_ctx.grant->parts[3]
            : nullptr;

        godot::Vector3* pos = reinterpret_cast<godot::Vector3*>(pos_part.raw_base_ptr);
        godot::Vector3* vel = reinterpret_cast<godot::Vector3*>(vel_part.raw_base_ptr);
        const int32_t raw_n = (pos_part.selection.element_count > 0)
            ? static_cast<int32_t>(pos_part.selection.element_count)
            : static_cast<int32_t>(pos_part.capacity_bytes / sizeof(godot::Vector3));
        const int32_t n_verts = std::min<int32_t>({
            raw_n,
            static_cast<int32_t>(vel_part.capacity_bytes / sizeof(godot::Vector3)),
            MAX_XPBD_VERTS
        });
        if (n_verts <= 0) return;

        const godot::Vector3* rest_pos = (rest_part && rest_part->capacity_bytes >= static_cast<size_t>(n_verts) * sizeof(godot::Vector3))
            ? reinterpret_cast<const godot::Vector3*>(rest_part->raw_base_ptr)
            : nullptr;

        godot::Vector3* pred_pos = static_cast<godot::Vector3*>(p_ctx.local_workspace);
        const float dt = (p_ctx.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_ctx.delta)) : (1.0f / 60.0f);
        const float damp = std::max(0.0f, 1.0f - velocity_damping);
        const float alpha_tilde = edge_compliance / (dt * dt);

        // 1. Symplectic Euler prediction
        godot::Vector3 centroid(0.0f, 0.0f, 0.0f);
        for (int32_t i = 0; i < n_verts; ++i) {
            vel[i] = (vel[i] + gravity * dt) * damp;
            pred_pos[i] = pos[i] + vel[i] * dt;
            centroid += pred_pos[i];
        }
        centroid /= static_cast<float>(n_verts);

        // 2. XPBD Constraint Projection Iterations
        const int32_t* indices = idx_part ? reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr) : nullptr;
        const int32_t n_tris = idx_part
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
                        const godot::Vector3 d = pred_pos[b] - pred_pos[a];
                        const float len = d.length();
                        if (len < 1e-6f) continue;

                        const float rest_len = rest_pos ? (rest_pos[b] - rest_pos[a]).length() : len;
                        const float C = len - rest_len;
                        const float dlambda = -C / (2.0f + alpha_tilde);
                        const godot::Vector3 corr = (d / len) * dlambda;
                        pred_pos[a] -= corr;
                        pred_pos[b] += corr;
                    }
                }
            }

            // Rest-pose shape matching + radial balloon pressure
            for (int32_t i = 0; i < n_verts; ++i) {
                if (rest_pos && shape_matching_stiffness > 0.0f) {
                    pred_pos[i] = pred_pos[i].lerp(rest_pos[i], shape_matching_stiffness / static_cast<float>(solver_iterations));
                }
                if (std::abs(volume_pressure) > 1e-6f) {
                    const godot::Vector3 radial = (pred_pos[i] - centroid).normalized();
                    pred_pos[i] += radial * (volume_pressure * dt / static_cast<float>(solver_iterations));
                }
            }
        }

        // 3. Velocity update & position commit
        const float inv_dt = 1.0f / dt;
        for (int32_t i = 0; i < n_verts; ++i) {
            vel[i] = (pred_pos[i] - pos[i]) * inv_dt;
            pos[i] = pred_pos[i];
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core