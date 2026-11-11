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
 * V4: ParticleFieldAdvectionTransformLogic
 * Trilinearly samples a 3D Spatial3D velocity/curl grid and advects 3D particle positions
 * (and optional particle velocities) via Explicit Euler, Midpoint RK2, or Classical RK4.
 */
struct ParticleFieldAdvectionTransformLogic {
    static constexpr std::string_view ui_name = "Particle 3D Field Trilinear RK2/RK4 Advection";
    static constexpr std::string_view ui_category = "Particles/Fields";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 1> supported_types = {
        DataType::VECTOR3
    };

    int32_t integrator_order = 2; // 1 = Euler, 2 = Midpoint RK2, 4 = Classical RK4
    int32_t grid_x = 32;
    int32_t grid_y = 32;
    int32_t grid_z = 32;
    godot::Vector3 grid_origin = godot::Vector3(-16.0f, -16.0f, -16.0f);
    godot::Vector3 voxel_size = godot::Vector3(1.0f, 1.0f, 1.0f);
    float advection_strength = 1.0f;
    float drag_coefficient = 0.05f;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            list.push_back(d);
        };
        add_prop("integrator_order", godot::Variant::INT, integrator_order, 2, "Euler (1st),Midpoint RK2 (2nd),Runge-Kutta RK4 (4th)");
        add_prop("grid_x", godot::Variant::INT, grid_x);
        add_prop("grid_y", godot::Variant::INT, grid_y);
        add_prop("grid_z", godot::Variant::INT, grid_z);
        add_prop("grid_origin", godot::Variant::VECTOR3, grid_origin);
        add_prop("voxel_size", godot::Variant::VECTOR3, voxel_size);
        add_prop("advection_strength", godot::Variant::FLOAT, advection_strength);
        add_prop("drag_coefficient", godot::Variant::FLOAT, drag_coefficient);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("integrator_order")) integrator_order = std::clamp<int32_t>(static_cast<int32_t>(p_props["integrator_order"]), 0, 4);
        if (p_props.has("grid_x")) grid_x = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_x"]));
        if (p_props.has("grid_y")) grid_y = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_y"]));
        if (p_props.has("grid_z")) grid_z = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_z"]));
        if (p_props.has("grid_origin")) grid_origin = p_props["grid_origin"];
        if (p_props.has("voxel_size")) voxel_size = p_props["voxel_size"];
        if (p_props.has("advection_strength")) advection_strength = static_cast<float>(p_props["advection_strength"]);
        if (p_props.has("drag_coefficient")) drag_coefficient = std::clamp(static_cast<float>(p_props["drag_coefficient"]), 0.0f, 1.0f);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    godot::Vector3 sample_field_trilinear(
        const godot::Vector3* field, int32_t nx, int32_t ny, int32_t nz, const godot::Vector3& world_p) const noexcept
    {
        const float gx = std::clamp((world_p.x - grid_origin.x) / std::max(1e-5f, voxel_size.x), 0.0f, static_cast<float>(nx - 1));
        const float gy = std::clamp((world_p.y - grid_origin.y) / std::max(1e-5f, voxel_size.y), 0.0f, static_cast<float>(ny - 1));
        const float gz = std::clamp((world_p.z - grid_origin.z) / std::max(1e-5f, voxel_size.z), 0.0f, static_cast<float>(nz - 1));

        const int32_t x0 = std::min<int32_t>(nx - 2, static_cast<int32_t>(std::floor(gx)));
        const int32_t y0 = std::min<int32_t>(ny - 2, static_cast<int32_t>(std::floor(gy)));
        const int32_t z0 = std::min<int32_t>(nz - 2, static_cast<int32_t>(std::floor(gz)));
        const int32_t x1 = x0 + 1, y1 = y0 + 1, z1 = z0 + 1;

        const float tx = gx - static_cast<float>(x0);
        const float ty = gy - static_cast<float>(y0);
        const float tz = gz - static_cast<float>(z0);

        auto at = [&](int32_t ix, int32_t iy, int32_t iz) noexcept -> const godot::Vector3& {
            return field[(static_cast<size_t>(iz) * ny + iy) * nx + ix];
        };

        const godot::Vector3 c00 = at(x0, y0, z0).lerp(at(x1, y0, z0), tx);
        const godot::Vector3 c10 = at(x0, y1, z0).lerp(at(x1, y1, z0), tx);
        const godot::Vector3 c01 = at(x0, y0, z1).lerp(at(x1, y0, z1), tx);
        const godot::Vector3 c11 = at(x0, y1, z1).lerp(at(x1, y1, z1), tx);

        const godot::Vector3 c0 = c00.lerp(c10, ty);
        const godot::Vector3 c1 = c01.lerp(c11, ty);
        return c0.lerp(c1, tz) * advection_strength;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        GrantPartPOD& pos_part = p_ctx.grant->parts[0];
        const GrantPartPOD& field_part = p_ctx.grant->parts[p_ctx.grant->part_count - 1];
        if (!pos_part.raw_base_ptr || !field_part.raw_base_ptr) return;

        GrantPartPOD* vel_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        const int32_t nx = std::max<int32_t>(2, grid_x);
        const int32_t ny = std::max<int32_t>(2, grid_y);
        const int32_t nz = std::max<int32_t>(2, grid_z);
        if (field_part.capacity_bytes / sizeof(godot::Vector3) < static_cast<size_t>(nx) * ny * nz) return;

        const godot::Vector3* field = reinterpret_cast<const godot::Vector3*>(field_part.raw_base_ptr);
        const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part.element_stride);
        const int64_t n_particles = (pos_part.selection.element_count > 0)
            ? pos_part.selection.element_count
            : static_cast<int64_t>(pos_part.capacity_bytes / p_stride);

        const float dt = (p_ctx.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_ctx.delta)) : (1.0f / 60.0f);

        for (int64_t i = 0; i < n_particles; ++i) {
            godot::Vector3& p = *reinterpret_cast<godot::Vector3*>(pos_part.raw_base_ptr + static_cast<size_t>(i) * p_stride);

            godot::Vector3 adv_vel;
            if (integrator_order >= 2) {
                // RK4 integration
                const godot::Vector3 k1 = sample_field_trilinear(field, nx, ny, nz, p);
                const godot::Vector3 k2 = sample_field_trilinear(field, nx, ny, nz, p + k1 * (0.5f * dt));
                const godot::Vector3 k3 = sample_field_trilinear(field, nx, ny, nz, p + k2 * (0.5f * dt));
                const godot::Vector3 k4 = sample_field_trilinear(field, nx, ny, nz, p + k3 * dt);
                adv_vel = (k1 + k2 * 2.0f + k3 * 2.0f + k4) / 6.0f;
            } else if (integrator_order == 1) {
                // Midpoint RK2 integration
                const godot::Vector3 k1 = sample_field_trilinear(field, nx, ny, nz, p);
                adv_vel = sample_field_trilinear(field, nx, ny, nz, p + k1 * (0.5f * dt));
            } else {
                // Explicit Euler
                adv_vel = sample_field_trilinear(field, nx, ny, nz, p);
            }

            if (vel_part && static_cast<size_t>(i + 1) * sizeof(godot::Vector3) <= vel_part->capacity_bytes) {
                godot::Vector3& v = reinterpret_cast<godot::Vector3*>(vel_part->raw_base_ptr)[i];
                v = v.lerp(adv_vel, std::clamp(drag_coefficient, 0.0f, 1.0f));
                p += v * dt;
            } else {
                p += adv_vel * dt;
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core