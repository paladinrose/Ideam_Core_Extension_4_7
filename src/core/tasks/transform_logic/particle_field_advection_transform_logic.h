#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
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

enum class AdvectionIntegratorOrder : uint8_t {
    EXPLICIT_EULER = 0,
    MIDPOINT_RK2   = 1,
    CLASSICAL_RK4  = 2
};

/**
 * ParticleFieldAdvectionTransformLogic<T> (V4)
 * Trilinearly samples a 3D Spatial3D velocity/curl grid and advects 3D particle positions
 * (and optional particle velocities) via Explicit Euler, Midpoint RK2, or Classical RK4.
 */
template <typename T = godot::Vector3>
struct alignas(64) ParticleFieldAdvectionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Particle 3D Field Trilinear RK2/RK4 Advection";
    static constexpr std::string_view ui_category  = "Particles/Fields";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t position_buffer_id = INVALID_ID; // 4B: Primary target particle position buffer (Vector3)
    uint32_t velocity_buffer_id = INVALID_ID; // 4B: Optional particle velocity buffer (Vector3)
    uint32_t field_buffer_id    = INVALID_ID; // 4B: 3D Vector3 velocity/curl grid buffer -> 12B

    godot::Vector3 grid_origin{-16.0f, -16.0f, -16.0f}; // 12B -> 24B
    godot::Vector3 voxel_size{1.0f, 1.0f, 1.0f};        // 12B -> 36B

    float advection_strength                  = 1.0f;  // 4B -> 40B
    float drag_coefficient                    = 0.05f; // 4B -> 44B
    uint16_t grid_x                           = 32;    // 2B
    uint16_t grid_y                           = 32;    // 2B
    uint16_t grid_z                           = 32;    // 2B -> 50B
    AdvectionIntegratorOrder integrator_order = AdvectionIntegratorOrder::MIDPOINT_RK2; // 1B
    uint8_t reserved_padding[13]              = {0};   // 13B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary pbuf_prop;
        pbuf_prop["name"] = "position_buffer_id";
        pbuf_prop["type"] = godot::Variant::INT;
        pbuf_prop["hint_string"] = "buffer_option";
        props.push_back(pbuf_prop);

        godot::Dictionary vbuf_prop;
        vbuf_prop["name"] = "velocity_buffer_id";
        vbuf_prop["type"] = godot::Variant::INT;
        vbuf_prop["hint_string"] = "buffer_option";
        props.push_back(vbuf_prop);

        godot::Dictionary fbuf_prop;
        fbuf_prop["name"] = "field_buffer_id";
        fbuf_prop["type"] = godot::Variant::INT;
        fbuf_prop["hint_string"] = "buffer_option";
        props.push_back(fbuf_prop);

        godot::Dictionary ord_prop;
        ord_prop["name"] = "integrator_order";
        ord_prop["type"] = godot::Variant::INT;
        ord_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        ord_prop["hint_string"] = "Explicit Euler (1st):0,Midpoint RK2 (2nd):1,Classical RK4 (4th):2";
        props.push_back(ord_prop);

        godot::Dictionary gx_prop;
        gx_prop["name"] = "grid_x";
        gx_prop["type"] = godot::Variant::INT;
        props.push_back(gx_prop);

        godot::Dictionary gy_prop;
        gy_prop["name"] = "grid_y";
        gy_prop["type"] = godot::Variant::INT;
        props.push_back(gy_prop);

        godot::Dictionary gz_prop;
        gz_prop["name"] = "grid_z";
        gz_prop["type"] = godot::Variant::INT;
        props.push_back(gz_prop);

        godot::Dictionary org_prop;
        org_prop["name"] = "grid_origin";
        org_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(org_prop);

        godot::Dictionary vsz_prop;
        vsz_prop["name"] = "voxel_size";
        vsz_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(vsz_prop);

        godot::Dictionary str_prop;
        str_prop["name"] = "advection_strength";
        str_prop["type"] = godot::Variant::FLOAT;
        props.push_back(str_prop);

        godot::Dictionary drg_prop;
        drg_prop["name"] = "drag_coefficient";
        drg_prop["type"] = godot::Variant::FLOAT;
        drg_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        drg_prop["hint_string"] = "0.0,1.0,0.005";
        props.push_back(drg_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return position_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("position_buffer_id")) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("velocity_buffer_id")) {
            velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["velocity_buffer_id"]));
        }
        if (p_props.has("field_buffer_id")) {
            field_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["field_buffer_id"]));
        }
        if (p_props.has("integrator_order")) {
            integrator_order = static_cast<AdvectionIntegratorOrder>(std::clamp<int64_t>(static_cast<int64_t>(p_props["integrator_order"]), 0, 2));
        }
        if (p_props.has("grid_x")) grid_x = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["grid_x"]), 2, 4096));
        if (p_props.has("grid_y")) grid_y = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["grid_y"]), 2, 4096));
        if (p_props.has("grid_z")) grid_z = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["grid_z"]), 2, 4096));
        if (p_props.has("grid_origin")) grid_origin = static_cast<godot::Vector3>(p_props["grid_origin"]);
        if (p_props.has("voxel_size"))  voxel_size  = static_cast<godot::Vector3>(p_props["voxel_size"]);
        if (p_props.has("advection_strength")) advection_strength = static_cast<float>(p_props["advection_strength"]);
        if (p_props.has("drag_coefficient"))   drag_coefficient   = std::clamp(static_cast<float>(p_props["drag_coefficient"]), 0.0f, 1.0f);
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const GrantPartPOD* pos_part   = p_context.get_grant_part(position_buffer_id);
            const GrantPartPOD* field_part = p_context.get_grant_part(field_buffer_id);
            if (!pos_part || !field_part || !pos_part->raw_base_ptr || !field_part->raw_base_ptr) return;

            const GrantPartPOD* vel_part = (velocity_buffer_id != INVALID_ID) ? p_context.get_grant_part(velocity_buffer_id) : nullptr;

            const int32_t nx = std::max<int32_t>(2, grid_x);
            const int32_t ny = std::max<int32_t>(2, grid_y);
            const int32_t nz = std::max<int32_t>(2, grid_z);
            if (field_part->capacity_bytes / sizeof(godot::Vector3) < static_cast<size_t>(nx) * ny * nz) return;

            const godot::Vector3* field = reinterpret_cast<const godot::Vector3*>(field_part->raw_base_ptr);
            const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part->element_stride);
            const int64_t n_particles = (pos_part->selection.element_count > 0)
                ? pos_part->selection.element_count
                : static_cast<int64_t>(pos_part->capacity_bytes / p_stride);

            const float dt = (p_context.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_context.delta)) : (1.0f / 60.0f);

            for (int64_t i = 0; i < n_particles; ++i) {
                godot::Vector3& p = *reinterpret_cast<godot::Vector3*>(pos_part->raw_base_ptr + static_cast<size_t>(i) * p_stride);

                godot::Vector3 adv_vel;
                if (integrator_order == AdvectionIntegratorOrder::CLASSICAL_RK4) {
                    const godot::Vector3 k1 = _sample_field_trilinear(field, nx, ny, nz, p);
                    const godot::Vector3 k2 = _sample_field_trilinear(field, nx, ny, nz, p + k1 * (0.5f * dt));
                    const godot::Vector3 k3 = _sample_field_trilinear(field, nx, ny, nz, p + k2 * (0.5f * dt));
                    const godot::Vector3 k4 = _sample_field_trilinear(field, nx, ny, nz, p + k3 * dt);
                    adv_vel = (k1 + k2 * 2.0f + k3 * 2.0f + k4) * (1.0f / 6.0f);
                } else if (integrator_order == AdvectionIntegratorOrder::MIDPOINT_RK2) {
                    const godot::Vector3 k1 = _sample_field_trilinear(field, nx, ny, nz, p);
                    adv_vel = _sample_field_trilinear(field, nx, ny, nz, p + k1 * (0.5f * dt));
                } else {
                    adv_vel = _sample_field_trilinear(field, nx, ny, nz, p);
                }

                if (vel_part && vel_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(godot::Vector3) <= vel_part->capacity_bytes) {
                    godot::Vector3& v = reinterpret_cast<godot::Vector3*>(vel_part->raw_base_ptr)[i];
                    v = v.lerp(adv_vel, std::clamp(drag_coefficient, 0.0f, 1.0f));
                    p += v * dt;
                } else {
                    p += adv_vel * dt;
                }
            }
        }
    }

private:
    godot::Vector3 _sample_field_trilinear(
        const godot::Vector3* p_field, int32_t p_nx, int32_t p_ny, int32_t p_nz, const godot::Vector3& p_world_p) const noexcept
    {
        const float gx = std::clamp((p_world_p.x - grid_origin.x) / std::max(1e-5f, voxel_size.x), 0.0f, static_cast<float>(p_nx - 1));
        const float gy = std::clamp((p_world_p.y - grid_origin.y) / std::max(1e-5f, voxel_size.y), 0.0f, static_cast<float>(p_ny - 1));
        const float gz = std::clamp((p_world_p.z - grid_origin.z) / std::max(1e-5f, voxel_size.z), 0.0f, static_cast<float>(p_nz - 1));

        const int32_t x0 = std::min<int32_t>(p_nx - 2, static_cast<int32_t>(std::floor(gx)));
        const int32_t y0 = std::min<int32_t>(p_ny - 2, static_cast<int32_t>(std::floor(gy)));
        const int32_t z0 = std::min<int32_t>(p_nz - 2, static_cast<int32_t>(std::floor(gz)));
        const int32_t x1 = x0 + 1, y1 = y0 + 1, z1 = z0 + 1;

        const float tx = gx - static_cast<float>(x0);
        const float ty = gy - static_cast<float>(y0);
        const float tz = gz - static_cast<float>(z0);

        auto at = [&](int32_t ix, int32_t iy, int32_t iz) noexcept -> const godot::Vector3& {
            return p_field[(static_cast<size_t>(iz) * p_ny + iy) * p_nx + ix];
        };

        const godot::Vector3 c00 = at(x0, y0, z0).lerp(at(x1, y0, z0), tx);
        const godot::Vector3 c10 = at(x0, y1, z0).lerp(at(x1, y1, z0), tx);
        const godot::Vector3 c01 = at(x0, y0, z1).lerp(at(x1, y0, z1), tx);
        const godot::Vector3 c11 = at(x0, y1, z1).lerp(at(x1, y1, z1), tx);

        const godot::Vector3 c0 = c00.lerp(c10, ty);
        const godot::Vector3 c1 = c01.lerp(c11, ty);
        return c0.lerp(c1, tz) * advection_strength;
    }
};

static_assert(sizeof(ParticleFieldAdvectionTransformLogic<>) == 64, "ParticleFieldAdvectionTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ParticleFieldAdvectionTransformLogic<>>, "ParticleFieldAdvectionTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core