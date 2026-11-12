#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * VectorFieldCurlNoiseTransformLogic<T, T_Strategy, KernelSize> (V3)
 * Computes a divergence-free 3D curl velocity field (v = curl(psi)) across a 3D spatial grid
 * either via central differences of an input 3D vector potential buffer or by analytically
 * differentiating a 3D trigonometric potential field in-place.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = Spatial3DStrategy,
    size_t KernelSize = 6
>
struct alignas(64) VectorFieldCurlNoiseTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StaticStencilView<T, DefaultStrategy, KernelSize>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = true;
    static constexpr size_t kernel_size          = KernelSize;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "3D Divergence-Free Curl Noise Velocity Field";
    static constexpr std::string_view ui_category  = "Particles/Fields";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t potential_buffer_id       = INVALID_ID; // 4B: Optional source 3D Vector3 potential grid
    uint32_t velocity_field_buffer_id  = INVALID_ID; // 4B: Primary target 3D Vector3 curl velocity grid
    int32_t grid_x                     = 32;         // 4B: Voxel count along X
    int32_t grid_y                     = 32;         // 4B: Voxel count along Y
    int32_t grid_z                     = 32;         // 4B: Voxel count along Z
    float cell_size                    = 1.0f;       // 4B: Voxel spacing
    float noise_frequency              = 0.15f;      // 4B: Procedural spatial frequency
    float curl_amplitude               = 4.0f;       // 4B: Curl velocity scale
    float time_offset                  = 0.0f;       // 4B: Temporal phase offset -> 36B
    bool use_input_potential_buffer    = true;       // 1B
    uint8_t reserved_padding[27]       = {0};        // 27B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary pbuf_prop;
        pbuf_prop["name"] = "potential_buffer_id";
        pbuf_prop["type"] = godot::Variant::INT;
        pbuf_prop["hint_string"] = "buffer_option";
        props.push_back(pbuf_prop);

        godot::Dictionary vbuf_prop;
        vbuf_prop["name"] = "velocity_field_buffer_id";
        vbuf_prop["type"] = godot::Variant::INT;
        vbuf_prop["hint_string"] = "buffer_option";
        props.push_back(vbuf_prop);

        godot::Dictionary gx_prop;
        gx_prop["name"] = "grid_x";
        gx_prop["type"] = godot::Variant::INT;
        gx_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        gx_prop["hint_string"] = "2,512,1";
        props.push_back(gx_prop);

        godot::Dictionary gy_prop;
        gy_prop["name"] = "grid_y";
        gy_prop["type"] = godot::Variant::INT;
        gy_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        gy_prop["hint_string"] = "2,512,1";
        props.push_back(gy_prop);

        godot::Dictionary gz_prop;
        gz_prop["name"] = "grid_z";
        gz_prop["type"] = godot::Variant::INT;
        gz_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        gz_prop["hint_string"] = "2,512,1";
        props.push_back(gz_prop);

        godot::Dictionary cs_prop;
        cs_prop["name"] = "cell_size";
        cs_prop["type"] = godot::Variant::FLOAT;
        cs_prop["hint_string"] = "suffix:m";
        props.push_back(cs_prop);

        godot::Dictionary nfr_prop;
        nfr_prop["name"] = "noise_frequency";
        nfr_prop["type"] = godot::Variant::FLOAT;
        props.push_back(nfr_prop);

        godot::Dictionary amp_prop;
        amp_prop["name"] = "curl_amplitude";
        amp_prop["type"] = godot::Variant::FLOAT;
        props.push_back(amp_prop);

        godot::Dictionary tmo_prop;
        tmo_prop["name"] = "time_offset";
        tmo_prop["type"] = godot::Variant::FLOAT;
        props.push_back(tmo_prop);

        godot::Dictionary uip_prop;
        uip_prop["name"] = "use_input_potential_buffer";
        uip_prop["type"] = godot::Variant::BOOL;
        props.push_back(uip_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (velocity_field_buffer_id != INVALID_ID) ? velocity_field_buffer_id : potential_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("potential_buffer_id")) {
            potential_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["potential_buffer_id"]));
        }
        if (p_props.has("velocity_field_buffer_id")) {
            velocity_field_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["velocity_field_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && velocity_field_buffer_id == INVALID_ID) {
            velocity_field_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("grid_buffer_id") && velocity_field_buffer_id == INVALID_ID) {
            velocity_field_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_buffer_id"]));
        }

        if (p_props.has("grid_x")) grid_x = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_x"]));
        if (p_props.has("grid_y")) grid_y = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_y"]));
        if (p_props.has("grid_z")) grid_z = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_z"]));
        if (p_props.has("cell_size")) cell_size = std::max(1e-4f, static_cast<float>(p_props["cell_size"]));
        if (p_props.has("noise_frequency")) noise_frequency = std::max(1e-5f, static_cast<float>(p_props["noise_frequency"]));
        if (p_props.has("curl_amplitude")) curl_amplitude = static_cast<float>(p_props["curl_amplitude"]);
        if (p_props.has("time_offset")) time_offset = static_cast<float>(p_props["time_offset"]);
        if (p_props.has("use_input_potential_buffer")) {
            use_input_potential_buffer = static_cast<bool>(p_props["use_input_potential_buffer"]);
        }
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
        if constexpr (requires { r_view.baked_offsets; } && KernelSize >= 6) {
            const intptr_t sx = static_cast<intptr_t>(stride);
            const intptr_t sy = static_cast<intptr_t>(nx) * sx;
            const intptr_t sz = static_cast<intptr_t>(ny) * sy;
            r_view.baked_offsets[0] = -sx;
            r_view.baked_offsets[1] =  sx;
            r_view.baked_offsets[2] = -sy;
            r_view.baked_offsets[3] =  sy;
            r_view.baked_offsets[4] = -sz;
            r_view.baked_offsets[5] =  sz;
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
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const uint32_t dst_id = (velocity_field_buffer_id != INVALID_ID) ? velocity_field_buffer_id : potential_buffer_id;
            const GrantPartPOD* dst_part = p_context.get_grant_part(dst_id);
            if (!dst_part || !dst_part->raw_base_ptr) return;

            const GrantPartPOD* src_part = (use_input_potential_buffer && potential_buffer_id != INVALID_ID && potential_buffer_id != dst_id)
                ? p_context.get_grant_part(potential_buffer_id)
                : nullptr;

            const int32_t nx = std::max<int32_t>(2, grid_x);
            const int32_t ny = std::max<int32_t>(2, grid_y);
            const int32_t nz = std::max<int32_t>(2, grid_z);
            const size_t total = static_cast<size_t>(nx) * static_cast<size_t>(ny) * static_cast<size_t>(nz);

            if (dst_part->capacity_bytes / sizeof(godot::Vector3) < total) return;
            godot::Vector3* out_vel = reinterpret_cast<godot::Vector3*>(dst_part->raw_base_ptr);

            const godot::Vector3* psi_grid = (src_part && src_part->raw_base_ptr && src_part->capacity_bytes / sizeof(godot::Vector3) >= total)
                ? reinterpret_cast<const godot::Vector3*>(src_part->raw_base_ptr)
                : nullptr;

            auto sample_psi = [&](int32_t x, int32_t y, int32_t z) noexcept -> godot::Vector3 {
                const int32_t cx = std::clamp(x, 0, nx - 1);
                const int32_t cy = std::clamp(y, 0, ny - 1);
                const int32_t cz = std::clamp(z, 0, nz - 1);
                if (psi_grid) {
                    return psi_grid[(static_cast<size_t>(cz) * ny + cy) * nx + cx];
                }
                return _eval_procedural_potential(
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
    }

private:
    inline godot::Vector3 _eval_procedural_potential(float p_x, float p_y, float p_z) const noexcept {
        const float fx = p_x * noise_frequency + time_offset;
        const float fy = p_y * noise_frequency + time_offset * 0.73f;
        const float fz = p_z * noise_frequency - time_offset * 0.51f;
        return godot::Vector3(
            std::sin(fy) * std::cos(fz),
            std::sin(fz) * std::cos(fx),
            std::sin(fx) * std::cos(fy)
        );
    }
};

static_assert(sizeof(VectorFieldCurlNoiseTransformLogic<>) == 64, "VectorFieldCurlNoiseTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<VectorFieldCurlNoiseTransformLogic<>>, "VectorFieldCurlNoiseTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core