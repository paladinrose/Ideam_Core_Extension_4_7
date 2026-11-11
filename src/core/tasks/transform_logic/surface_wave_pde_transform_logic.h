#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * M32: SurfaceWavePDETransformLogic
 * Solves the damped 2D wave equation d^2u/dt^2 + damping * du/dt = c^2 * Laplace_Beltrami(u)
 * directly on manifold 3D triangle meshes, displacing vertices along surface normals.
 */
struct SurfaceWavePDETransformLogic {
    static constexpr std::string_view ui_name = "Manifold Surface Wave PDE Solver";
    static constexpr std::string_view ui_category = "Model/SurfacePDE";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SWAP,
        MemoryView::STENCIL
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::VECTOR3,
        DataType::VECTOR2,
        DataType::FLOAT32
    };

    static constexpr int32_t MAX_WAVE_VERTS = 16384;

    float wave_speed = 4.0f;
    float wave_damping = 0.85f;
    float displacement_scale = 1.0f;
    godot::Vector3 impulse_origin = godot::Vector3(0.0f, 0.0f, 0.0f);
    float impulse_radius = 0.0f;
    float impulse_strength = 0.0f;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("wave_speed", godot::Variant::FLOAT, wave_speed);
        add_prop("wave_damping", godot::Variant::FLOAT, wave_damping);
        add_prop("displacement_scale", godot::Variant::FLOAT, displacement_scale);
        add_prop("impulse_origin", godot::Variant::VECTOR3, impulse_origin);
        add_prop("impulse_radius", godot::Variant::FLOAT, impulse_radius);
        add_prop("impulse_strength", godot::Variant::FLOAT, impulse_strength);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("wave_speed")) wave_speed = std::max(0.0f, static_cast<float>(p_props["wave_speed"]));
        if (p_props.has("wave_damping")) wave_damping = std::max(0.0f, static_cast<float>(p_props["wave_damping"]));
        if (p_props.has("displacement_scale")) displacement_scale = static_cast<float>(p_props["displacement_scale"]);
        if (p_props.has("impulse_origin")) impulse_origin = p_props["impulse_origin"];
        if (p_props.has("impulse_radius")) impulse_radius = std::max(0.0f, static_cast<float>(p_props["impulse_radius"]));
        if (p_props.has("impulse_strength")) impulse_strength = static_cast<float>(p_props["impulse_strength"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_WAVE_VERTS * sizeof(float) * 2 + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_WAVE_VERTS * sizeof(float) * 2 + 256;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 3 || !p_ctx.local_workspace) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        const GrantPartPOD& nrm_part = p_ctx.grant->parts[1];
        GrantPartPOD& wave_part = p_ctx.grant->parts[2]; // Vector2(height, velocity) per vertex
        if (!vtx_part.raw_base_ptr || !nrm_part.raw_base_ptr || !wave_part.raw_base_ptr) return;

        const GrantPartPOD* idx_part = (p_ctx.grant->part_count > 3 && p_ctx.grant->parts[3].raw_base_ptr)
            ? &p_ctx.grant->parts[3]
            : nullptr;

        godot::Vector3* verts = reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr);
        const godot::Vector3* norms = reinterpret_cast<const godot::Vector3*>(nrm_part.raw_base_ptr);
        godot::Vector2* hv = reinterpret_cast<godot::Vector2*>(wave_part.raw_base_ptr);

        const int32_t raw_n = (vtx_part.selection.element_count > 0)
            ? static_cast<int32_t>(vtx_part.selection.element_count)
            : static_cast<int32_t>(vtx_part.capacity_bytes / sizeof(godot::Vector3));
        const int32_t n_verts = std::min<int32_t>({
            raw_n,
            static_cast<int32_t>(nrm_part.capacity_bytes / sizeof(godot::Vector3)),
            static_cast<int32_t>(wave_part.capacity_bytes / sizeof(godot::Vector2)),
            MAX_WAVE_VERTS
        });
        if (n_verts <= 0) return;

        float* lap_u = static_cast<float*>(p_ctx.local_workspace);
        float* deg_u = lap_u + MAX_WAVE_VERTS;
        for (int32_t i = 0; i < n_verts; ++i) {
            lap_u[i] = 0.0f;
            deg_u[i] = 0.0f;
        }

        if (idx_part) {
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
            // 1D ring fallback if no index buffer is bound
            for (int32_t i = 0; i < n_verts; ++i) {
                const int32_t prev = (i + n_verts - 1) % n_verts;
                const int32_t next = (i + 1) % n_verts;
                lap_u[i] = (hv[prev].x - hv[i].x) + (hv[next].x - hv[i].x);
                deg_u[i] = 2.0f;
            }
        }

        const float dt = (p_ctx.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_ctx.delta)) : (1.0f / 60.0f);
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

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core