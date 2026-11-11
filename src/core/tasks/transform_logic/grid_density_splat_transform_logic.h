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
 * V6: GridDensitySplatTransformLogic
 * Particle-to-Grid (P2G) trilinear 8-corner scatter from 3D particle positions (Part 0)
 * onto a 3D scalar voxel density buffer (Part 1 or Part 2) for FogVolume density textures
 * or SDFMarchingCubesExtract (GEO8) fluid/metaball surfaces.
 */
struct GridDensitySplatTransformLogic {
    static constexpr std::string_view ui_name = "Particle-to-Grid (P2G) 3D Trilinear Density Splat";
    static constexpr std::string_view ui_category = "Particles/Fields";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::ATOMIC,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::SPATIAL_3D,
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::FLOAT32
    };

    int32_t grid_x = 32;
    int32_t grid_y = 32;
    int32_t grid_z = 32;
    godot::Vector3 grid_origin = godot::Vector3(-16.0f, -16.0f, -16.0f);
    godot::Vector3 voxel_size = godot::Vector3(1.0f, 1.0f, 1.0f);
    float splat_weight = 1.0f;
    float grid_decay = 0.0f; // 0 = clear grid before splatting; (0..1) = exponential persistence decay

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
        add_prop("grid_origin", godot::Variant::VECTOR3, grid_origin);
        add_prop("voxel_size", godot::Variant::VECTOR3, voxel_size);
        add_prop("splat_weight", godot::Variant::FLOAT, splat_weight);
        add_prop("grid_decay", godot::Variant::FLOAT, grid_decay);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("grid_x")) grid_x = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_x"]));
        if (p_props.has("grid_y")) grid_y = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_y"]));
        if (p_props.has("grid_z")) grid_z = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_z"]));
        if (p_props.has("grid_origin")) grid_origin = p_props["grid_origin"];
        if (p_props.has("voxel_size")) voxel_size = p_props["voxel_size"];
        if (p_props.has("splat_weight")) splat_weight = static_cast<float>(p_props["splat_weight"]);
        if (p_props.has("grid_decay")) grid_decay = std::clamp(static_cast<float>(p_props["grid_decay"]), 0.0f, 1.0f);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& pos_part = p_ctx.grant->parts[0];
        GrantPartPOD& grid_part = p_ctx.grant->parts[p_ctx.grant->part_count - 1];
        if (!pos_part.raw_base_ptr || !grid_part.raw_base_ptr) return;

        const GrantPartPOD* wgt_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        const int32_t nx = std::max<int32_t>(2, grid_x);
        const int32_t ny = std::max<int32_t>(2, grid_y);
        const int32_t nz = std::max<int32_t>(2, grid_z);
        const size_t total_voxels = static_cast<size_t>(nx) * ny * nz;
        if (grid_part.capacity_bytes / sizeof(float) < total_voxels) return;

        float* density = reinterpret_cast<float*>(grid_part.raw_base_ptr);
        for (size_t i = 0; i < total_voxels; ++i) {
            density[i] *= grid_decay;
        }

        const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part.element_stride);
        const int64_t n_particles = (pos_part.selection.element_count > 0)
            ? pos_part.selection.element_count
            : static_cast<int64_t>(pos_part.capacity_bytes / p_stride);

        const float* weights = wgt_part ? reinterpret_cast<const float*>(wgt_part->raw_base_ptr) : nullptr;
        const int64_t max_w = wgt_part ? static_cast<int64_t>(wgt_part->capacity_bytes / sizeof(float)) : 0;

        for (int64_t i = 0; i < n_particles; ++i) {
            const godot::Vector3& p = *reinterpret_cast<const godot::Vector3*>(pos_part.raw_base_ptr + static_cast<size_t>(i) * p_stride);
            const float gx = (p.x - grid_origin.x) / std::max(1e-5f, voxel_size.x);
            const float gy = (p.y - grid_origin.y) / std::max(1e-5f, voxel_size.y);
            const float gz = (p.z - grid_origin.z) / std::max(1e-5f, voxel_size.z);

            if (gx < 0.0f || gx >= static_cast<float>(nx - 1) ||
                gy < 0.0f || gy >= static_cast<float>(ny - 1) ||
                gz < 0.0f || gz >= static_cast<float>(nz - 1)) {
                continue;
            }

            const int32_t x0 = static_cast<int32_t>(std::floor(gx));
            const int32_t y0 = static_cast<int32_t>(std::floor(gy));
            const int32_t z0 = static_cast<int32_t>(std::floor(gz));

            const float tx = gx - static_cast<float>(x0);
            const float ty = gy - static_cast<float>(y0);
            const float tz = gz - static_cast<float>(z0);
            const float ox = 1.0f - tx, oy = 1.0f - ty, oz = 1.0f - tz;

            const float w = ((weights && i < max_w) ? weights[i] : 1.0f) * splat_weight;

            auto add_voxel = [&](int32_t ix, int32_t iy, int32_t iz, float val) noexcept {
                density[(static_cast<size_t>(iz) * ny + iy) * nx + ix] += val;
            };

            add_voxel(x0,     y0,     z0,     w * ox * oy * oz);
            add_voxel(x0 + 1, y0,     z0,     w * tx * oy * oz);
            add_voxel(x0,     y0 + 1, z0,     w * ox * ty * oz);
            add_voxel(x0 + 1, y0 + 1, z0,     w * tx * ty * oz);
            add_voxel(x0,     y0,     z0 + 1, w * ox * oy * tz);
            add_voxel(x0 + 1, y0,     z0 + 1, w * tx * oy * tz);
            add_voxel(x0,     y0 + 1, z0 + 1, w * ox * ty * tz);
            add_voxel(x0 + 1, y0 + 1, z0 + 1, w * tx * ty * tz);
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core