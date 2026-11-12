#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/atomic_view.h"
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
 * GridDensitySplatTransformLogic<T, T_Strategy> (V6)
 * Particle-to-Grid (P2G) trilinear 8-corner scatter from 3D particle positions (position_buffer_id)
 * onto a 3D scalar voxel density buffer (density_grid_buffer_id) for FogVolume density textures
 * or SDFMarchingCubesExtract (GEO8) fluid/metaball surfaces.
 */
template <typename T = float, typename T_Strategy = Spatial3DStrategy>
struct alignas(64) GridDensitySplatTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = AtomicView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL | BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Particle-to-Grid (P2G) 3D Trilinear Density Splat";
    static constexpr std::string_view ui_category  = "Particles/Fields";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t density_grid_buffer_id = INVALID_ID; // 4B: Primary target 3D float voxel density buffer
    uint32_t position_buffer_id     = INVALID_ID; // 4B: Source Vector3 particle positions buffer
    uint32_t weight_buffer_id       = INVALID_ID; // 4B: Optional per-particle float weight buffer -> 12B

    godot::Vector3 grid_origin{-16.0f, -16.0f, -16.0f}; // 12B -> 24B
    godot::Vector3 voxel_size{1.0f, 1.0f, 1.0f};        // 12B -> 36B

    float splat_weight           = 1.0f; // 4B -> 40B
    float grid_decay             = 0.0f; // 4B: 0 = clear grid before splat; (0..1) = persistence decay -> 44B
    uint16_t grid_x              = 32;   // 2B
    uint16_t grid_y              = 32;   // 2B
    uint16_t grid_z              = 32;   // 2B -> 50B
    uint8_t reserved_padding[14] = {0};  // 14B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary gbuf_prop;
        gbuf_prop["name"] = "density_grid_buffer_id";
        gbuf_prop["type"] = godot::Variant::INT;
        gbuf_prop["hint_string"] = "buffer_option";
        props.push_back(gbuf_prop);

        godot::Dictionary pbuf_prop;
        pbuf_prop["name"] = "position_buffer_id";
        pbuf_prop["type"] = godot::Variant::INT;
        pbuf_prop["hint_string"] = "buffer_option";
        props.push_back(pbuf_prop);

        godot::Dictionary wbuf_prop;
        wbuf_prop["name"] = "weight_buffer_id";
        wbuf_prop["type"] = godot::Variant::INT;
        wbuf_prop["hint_string"] = "buffer_option";
        props.push_back(wbuf_prop);

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

        godot::Dictionary sw_prop;
        sw_prop["name"] = "splat_weight";
        sw_prop["type"] = godot::Variant::FLOAT;
        props.push_back(sw_prop);

        godot::Dictionary dec_prop;
        dec_prop["name"] = "grid_decay";
        dec_prop["type"] = godot::Variant::FLOAT;
        dec_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        dec_prop["hint_string"] = "0.0,1.0,0.01";
        props.push_back(dec_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (density_grid_buffer_id != INVALID_ID) ? density_grid_buffer_id : position_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("density_grid_buffer_id")) {
            density_grid_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["density_grid_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && density_grid_buffer_id == INVALID_ID) {
            density_grid_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("grid_buffer_id") && density_grid_buffer_id == INVALID_ID) {
            density_grid_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_buffer_id"]));
        }

        if (p_props.has("position_buffer_id")) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        }
        if (p_props.has("weight_buffer_id")) {
            weight_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["weight_buffer_id"]));
        }
        if (p_props.has("grid_x")) grid_x = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["grid_x"]), 2, 4096));
        if (p_props.has("grid_y")) grid_y = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["grid_y"]), 2, 4096));
        if (p_props.has("grid_z")) grid_z = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["grid_z"]), 2, 4096));
        if (p_props.has("grid_origin")) grid_origin = static_cast<godot::Vector3>(p_props["grid_origin"]);
        if (p_props.has("voxel_size"))  voxel_size  = static_cast<godot::Vector3>(p_props["voxel_size"]);
        if (p_props.has("splat_weight")) splat_weight = static_cast<float>(p_props["splat_weight"]);
        if (p_props.has("grid_decay"))   grid_decay   = std::clamp(static_cast<float>(p_props["grid_decay"]), 0.0f, 1.0f);
    }

    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& /*p_context*/, const GrantPartPOD* p_part) const noexcept {
        const size_t stride = (p_part && p_part->element_stride > 0) ? p_part->element_stride : sizeof(T);
        const int64_t nx = std::max<int32_t>(2, grid_x);
        const int64_t ny = std::max<int32_t>(2, grid_y);
        if constexpr (requires { r_view.strategy.stride_y; }) {
            r_view.strategy.stride_y = nx * static_cast<int64_t>(stride);
        }
        if constexpr (requires { r_view.strategy.stride_z; }) {
            r_view.strategy.stride_z = nx * ny * static_cast<int64_t>(stride);
        }
    }

    template <typename T_View, typename T_Strat>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int32_t>) {
            const GrantPartPOD* pos_part  = p_context.get_grant_part(position_buffer_id);
            const GrantPartPOD* grid_part = p_context.get_grant_part(density_grid_buffer_id);
            if (!pos_part || !grid_part || !pos_part->raw_base_ptr || !grid_part->raw_base_ptr) return;

            const GrantPartPOD* wgt_part = (weight_buffer_id != INVALID_ID) ? p_context.get_grant_part(weight_buffer_id) : nullptr;

            const int32_t nx = std::max<int32_t>(2, grid_x);
            const int32_t ny = std::max<int32_t>(2, grid_y);
            const int32_t nz = std::max<int32_t>(2, grid_z);
            const size_t total_voxels = static_cast<size_t>(nx) * static_cast<size_t>(ny) * static_cast<size_t>(nz);
            if (grid_part->capacity_bytes / sizeof(float) < total_voxels) return;

            float* density = reinterpret_cast<float*>(grid_part->raw_base_ptr);
            for (size_t i = 0; i < total_voxels; ++i) {
                density[i] *= grid_decay;
            }

            const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part->element_stride);
            const int64_t n_particles = (pos_part->selection.element_count > 0)
                ? pos_part->selection.element_count
                : static_cast<int64_t>(pos_part->capacity_bytes / p_stride);

            const float* weights = (wgt_part && wgt_part->raw_base_ptr)
                ? reinterpret_cast<const float*>(wgt_part->raw_base_ptr)
                : nullptr;
            const int64_t max_w = weights ? static_cast<int64_t>(wgt_part->capacity_bytes / sizeof(float)) : 0;

            for (int64_t i = 0; i < n_particles; ++i) {
                const godot::Vector3& p = *reinterpret_cast<const godot::Vector3*>(pos_part->raw_base_ptr + static_cast<size_t>(i) * p_stride);
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
    }
};

static_assert(sizeof(GridDensitySplatTransformLogic<>) == 64, "GridDensitySplatTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<GridDensitySplatTransformLogic<>>, "GridDensitySplatTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core