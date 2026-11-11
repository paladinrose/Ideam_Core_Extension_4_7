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
 * V3: VectorFieldCurlNoiseTransformLogic
 * Computes a divergence-free 3D curl velocity field (v = curl(psi)) across a 3D spatial grid
 * either via central differences of an input 3D vector potential buffer (Part 0 -> Part 1)
 * or by analytically differentiating a 3D trigonometric/hash potential field in-place.
 */
struct VectorFieldCurlNoiseTransformLogic {
    static constexpr std::string_view ui_name = "3D Divergence-Free Curl Noise Velocity Field";
    static constexpr std::string_view ui_category = "Particles/Fields";

    static constexpr std::array<MemoryView, 3> supported_views = {
        MemoryView::STATIC_STENCIL,
        MemoryView::STENCIL,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::SPATIAL_3D,
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 1> supported_types = {
        DataType::VECTOR3
    };

    int32_t grid_x = 32;
    int32_t grid_y = 32;
    int32_t grid_z = 32;
    float cell_size = 1.0f;
    float noise_frequency = 0.15f;
    float curl_amplitude = 4.0f;
    float time_offset = 0.0f;
    bool use_input_potential_buffer = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("grid_x", godot::Variant::INT, grid_x);
        add_prop("grid_y", godot::Variant::INT, grid_y);
        add_prop("grid_z", godot::Variant::INT, grid_z);
        add_prop("cell_size", godot::Variant::FLOAT, cell_size);
        add_prop("noise_frequency", godot::Variant::FLOAT, noise_frequency);
        add_prop("curl_amplitude", godot::Variant::FLOAT, curl_amplitude);
        add_prop("time_offset", godot::Variant::FLOAT, time_offset);
        add_prop("use_input_potential_buffer", godot::Variant::BOOL, use_input_potential_buffer);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("grid_x")) grid_x = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_x"]));
        if (p_props.has("grid_y")) grid_y = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_y"]));
        if (p_props.has("grid_z")) grid_z = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_z"]));
        if (p_props.has("cell_size")) cell_size = std::max(1e-4f, static_cast<float>(p_props["cell_size"]));
        if (p_props.has("noise_frequency")) noise_frequency = std::max(1e-5f, static_cast<float>(p_props["noise_frequency"]));
        if (p_props.has("curl_amplitude")) curl_amplitude = static_cast<float>(p_props["curl_amplitude"]);
        if (p_props.has("time_offset")) time_offset = static_cast<float>(p_props["time_offset"]);
        if (p_props.has("use_input_potential_buffer")) use_input_potential_buffer = static_cast<bool>(p_props["use_input_potential_buffer"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    inline godot::Vector3 eval_procedural_potential(float x, float y, float z) const noexcept {
        const float fx = x * noise_frequency + time_offset;
        const float fy = y * noise_frequency + time_offset * 0.73f;
        const float fz = z * noise_frequency - time_offset * 0.51f;
        return godot::Vector3(
            std::sin(fy) * std::cos(fz),
            std::sin(fz) * std::cos(fx),
            std::sin(fx) * std::cos(fy)
        );
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        const GrantPartPOD& src_part = p_ctx.grant->parts[0];
        GrantPartPOD& dst_part = (p_ctx.grant->part_count > 1) ? p_ctx.grant->parts[1] : p_ctx.grant->parts[0];
        if (!src_part.raw_base_ptr || !dst_part.raw_base_ptr) return;

        const int32_t nx = std::max<int32_t>(2, grid_x);
        const int32_t ny = std::max<int32_t>(2, grid_y);
        const int32_t nz = std::max<int32_t>(2, grid_z);
        const size_t total = static_cast<size_t>(nx) * ny * nz;

        if (dst_part.capacity_bytes / sizeof(godot::Vector3) < total) return;
        godot::Vector3* out_vel = reinterpret_cast<godot::Vector3*>(dst_part.raw_base_ptr);

        const bool has_separate_potential = (use_input_potential_buffer && p_ctx.grant->part_count > 1 &&
                                             src_part.capacity_bytes / sizeof(godot::Vector3) >= total);
        const godot::Vector3* psi_grid = has_separate_potential
            ? reinterpret_cast<const godot::Vector3*>(src_part.raw_base_ptr)
            : nullptr;

        auto sample_psi = [&](int32_t x, int32_t y, int32_t z) noexcept -> godot::Vector3 {
            const int32_t cx = std::clamp(x, 0, nx - 1);
            const int32_t cy = std::clamp(y, 0, ny - 1);
            const int32_t cz = std::clamp(z, 0, nz - 1);
            if (psi_grid) {
                return psi_grid[(static_cast<size_t>(cz) * ny + cy) * nx + cx];
            }
            return eval_procedural_potential(
                static_cast<float>(cx) * cell_size,
                static_cast<float>(cy) * cell_size,
                static_cast<float>(cz) * cell_size
            );
        };

        const float scale = curl_amplitude / (2.0f * cell_size);

        for (int32_t z = 0; z < nz; ++z) {
            for (int32_t y = 0; y < ny; ++y) {
                for (int32_t x = 0; x < nx; ++x) {
                    const godot::Vector3 px1 = sample_psi(x + 1, y, z);
                    const godot::Vector3 px0 = sample_psi(x - 1, y, z);
                    const godot::Vector3 py1 = sample_psi(x, y + 1, z);
                    const godot::Vector3 py0 = sample_psi(x, y - 1, z);
                    const godot::Vector3 pz1 = sample_psi(x, y, z + 1);
                    const godot::Vector3 pz0 = sample_psi(x, y, z - 1);

                    const float dpsi_z_dy = py1.z - py0.z;
                    const float dpsi_y_dz = pz1.y - pz0.y;
                    const float dpsi_x_dz = pz1.x - pz0.x;
                    const float dpsi_z_dx = px1.z - px0.z;
                    const float dpsi_y_dx = px1.y - px0.y;
                    const float dpsi_x_dy = py1.x - py0.x;

                    out_vel[(static_cast<size_t>(z) * ny + y) * nx + x] = godot::Vector3(
                        (dpsi_z_dy - dpsi_y_dz) * scale,
                        (dpsi_x_dz - dpsi_z_dx) * scale,
                        (dpsi_y_dx - dpsi_x_dy) * scale
                    );
                }
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core