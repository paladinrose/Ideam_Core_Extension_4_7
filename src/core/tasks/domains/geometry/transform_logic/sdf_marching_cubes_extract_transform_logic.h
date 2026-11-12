#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/static_stencil_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

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
 * SDFMarchingCubesExtractTransformLogic<T, T_Strategy, T_View, KernelSize> (GEO8)
 * Extracts indexed or flat 3D triangle surfaces and smooth central-difference gradient normals
 * from a 3D scalar SDF / density volume buffer using manifold Marching Cubes / Tetrahedra extraction.
 */
template <
    typename T = float,
    typename T_Strategy = Spatial3DStrategy,
    template <typename, typename, size_t> class T_View = StaticStencilView,
    size_t KernelSize = 8
>
struct alignas(64) SDFMarchingCubesExtractTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy, KernelSize>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities =
        ViewCapability::SPATIAL_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types =
        DataType::FLOAT32 | DataType::VECTOR3;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = true;
    static constexpr size_t kernel_size          = KernelSize;

    static constexpr size_t transient_workspace_bytes = 4096;

    static constexpr std::string_view display_name = "SDF Marching Cubes Surface Extract";
    static constexpr std::string_view ui_category  = "Geometry/SDF";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::Vector3 voxel_size      = godot::Vector3(1.0f, 1.0f, 1.0f); // 12B
    uint32_t target_buffer_id      = INVALID_ID;                       // 4B -> 16B
    godot::Vector3 grid_origin     = godot::Vector3(0.0f, 0.0f, 0.0f); // 12B
    uint32_t sdf_volume_buffer_id  = INVALID_ID;                       // 4B -> 32B
    uint32_t vertex_out_buffer_id  = INVALID_ID;                       // 4B
    uint32_t normal_out_buffer_id  = INVALID_ID;                       // 4B
    uint32_t index_out_buffer_id   = INVALID_ID;                       // 4B
    int32_t grid_x                 = 32;                               // 4B -> 48B
    int32_t grid_y                 = 32;                               // 4B
    int32_t grid_z                 = 32;                               // 4B
    float iso_level                = 0.0f;                             // 4B
    bool compute_smooth_normals    = true;                             // 1B
    uint8_t reserved_padding[3]    = {0};                              // 3B -> Total = 64B

    static constexpr std::array<std::array<int32_t, 3>, 8> CORNER_OFFSETS = {{
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
        {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
    }};

    static constexpr std::array<std::array<uint8_t, 4>, 6> TETRAHEDRA = {{
        {0, 5, 1, 6},
        {0, 1, 2, 6},
        {0, 2, 3, 6},
        {0, 3, 7, 6},
        {0, 7, 4, 6},
        {0, 4, 5, 6}
    }};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("sdf_volume_buffer_id");
        add_buf("vertex_out_buffer_id");
        add_buf("normal_out_buffer_id");
        add_buf("index_out_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("grid_x", godot::Variant::INT, 32);
        add_prop("grid_y", godot::Variant::INT, 32);
        add_prop("grid_z", godot::Variant::INT, 32);
        add_prop("voxel_size", godot::Variant::VECTOR3, godot::Vector3(1.0f, 1.0f, 1.0f));
        add_prop("grid_origin", godot::Variant::VECTOR3, godot::Vector3(0.0f, 0.0f, 0.0f));
        add_prop("iso_level", godot::Variant::FLOAT, 0.0f);
        add_prop("compute_smooth_normals", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (vertex_out_buffer_id != INVALID_ID) ? vertex_out_buffer_id : sdf_volume_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("sdf_volume_buffer_id")) sdf_volume_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["sdf_volume_buffer_id"]));
        if (p_props.has("vertex_out_buffer_id")) vertex_out_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_out_buffer_id"]));
        if (p_props.has("normal_out_buffer_id")) normal_out_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_out_buffer_id"]));
        if (p_props.has("index_out_buffer_id")) index_out_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_out_buffer_id"]));
        if (p_props.has("grid_x")) grid_x = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_x"]));
        if (p_props.has("grid_y")) grid_y = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_y"]));
        if (p_props.has("grid_z")) grid_z = std::max<int32_t>(2, static_cast<int32_t>(p_props["grid_z"]));
        if (p_props.has("voxel_size")) voxel_size = p_props["voxel_size"];
        if (p_props.has("grid_origin")) grid_origin = p_props["grid_origin"];
        if (p_props.has("iso_level")) iso_level = static_cast<float>(p_props["iso_level"]);
        if (p_props.has("compute_smooth_normals")) compute_smooth_normals = static_cast<bool>(p_props["compute_smooth_normals"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    static inline godot::Vector3 interpolate_iso_vertex(
        const godot::Vector3& p1, const godot::Vector3& p2, float v1, float v2, float iso) noexcept
    {
        const float dv = v2 - v1;
        if (std::abs(dv) < 1e-7f) return (p1 + p2) * 0.5f;
        const float t = std::clamp((iso - v1) / dv, 0.0f, 1.0f);
        return p1 + (p2 - p1) * t;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const noexcept {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* sdf_part = (sdf_volume_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(sdf_volume_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            GrantPartPOD* vtx_part = (vertex_out_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(vertex_out_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!sdf_part || !sdf_part->raw_base_ptr || !vtx_part || !vtx_part->raw_base_ptr) return;

            GrantPartPOD* nrm_part = (normal_out_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(normal_out_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : nullptr);
            GrantPartPOD* idx_part = (index_out_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(index_out_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 3 && p_ctx.grant->parts[3].raw_base_ptr) ? &p_ctx.grant->parts[3] : nullptr);

            const int32_t nx = std::max<int32_t>(2, grid_x);
            const int32_t ny = std::max<int32_t>(2, grid_y);
            const int32_t nz = std::max<int32_t>(2, grid_z);
            const size_t required_voxels = static_cast<size_t>(nx) * ny * nz;
            const size_t avail_voxels = sdf_part->capacity_bytes / std::max<uint32_t>(1, sdf_part->element_stride);
            if (avail_voxels < required_voxels) return;

            const float* sdf = reinterpret_cast<const float*>(sdf_part->raw_base_ptr);
            const uint32_t sdf_step = std::max<uint32_t>(1, sdf_part->element_stride / sizeof(float));

            godot::Vector3* out_verts = reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr);
            const size_t max_verts = vtx_part->capacity_bytes / sizeof(godot::Vector3);

            godot::Vector3* out_norms = (nrm_part && nrm_part->raw_base_ptr)
                ? reinterpret_cast<godot::Vector3*>(nrm_part->raw_base_ptr)
                : nullptr;
            const size_t max_norms = out_norms ? (nrm_part->capacity_bytes / sizeof(godot::Vector3)) : 0;

            int32_t* out_indices = (idx_part && idx_part->raw_base_ptr)
                ? reinterpret_cast<int32_t*>(idx_part->raw_base_ptr)
                : nullptr;
            const size_t max_indices = out_indices ? (idx_part->capacity_bytes / sizeof(int32_t)) : 0;

            auto sample_sdf = [&](int32_t x, int32_t y, int32_t z) noexcept -> float {
                const int32_t cx = std::clamp(x, 0, nx - 1);
                const int32_t cy = std::clamp(y, 0, ny - 1);
                const int32_t cz = std::clamp(z, 0, nz - 1);
                const size_t idx = (static_cast<size_t>(cz) * ny + cy) * nx + cx;
                return sdf[idx * sdf_step];
            };

            auto sample_grad = [&](int32_t x, int32_t y, int32_t z) noexcept -> godot::Vector3 {
                const float gx = (sample_sdf(x + 1, y, z) - sample_sdf(x - 1, y, z)) / std::max(1e-6f, 2.0f * voxel_size.x);
                const float gy = (sample_sdf(x, y + 1, z) - sample_sdf(x, y - 1, z)) / std::max(1e-6f, 2.0f * voxel_size.y);
                const float gz = (sample_sdf(x, y, z + 1) - sample_sdf(x, y, z - 1)) / std::max(1e-6f, 2.0f * voxel_size.z);
                godot::Vector3 g(gx, gy, gz);
                const float len = g.length();
                return (len > 1e-6f) ? (g / len) : godot::Vector3(0.0f, 1.0f, 0.0f);
            };

            size_t emitted_verts = 0;
            size_t emitted_indices = 0;

            for (int32_t z = 0; z < nz - 1; ++z) {
                for (int32_t y = 0; y < ny - 1; ++y) {
                    for (int32_t x = 0; x < nx - 1; ++x) {
                        float c_val[8];
                        uint8_t mask = 0;
                        for (int c = 0; c < 8; ++c) {
                            const int32_t cx = x + CORNER_OFFSETS[c][0];
                            const int32_t cy = y + CORNER_OFFSETS[c][1];
                            const int32_t cz = z + CORNER_OFFSETS[c][2];
                            c_val[c] = sample_sdf(cx, cy, cz);
                            if (c_val[c] < iso_level) {
                                mask |= static_cast<uint8_t>(1u << c);
                            }
                        }

                        if (mask == 0 || mask == 0xFF) continue;

                        godot::Vector3 c_pos[8];
                        godot::Vector3 c_nrm[8];
                        for (int c = 0; c < 8; ++c) {
                            const int32_t cx = x + CORNER_OFFSETS[c][0];
                            const int32_t cy = y + CORNER_OFFSETS[c][1];
                            const int32_t cz = z + CORNER_OFFSETS[c][2];
                            c_pos[c] = godot::Vector3(
                                grid_origin.x + static_cast<float>(cx) * voxel_size.x,
                                grid_origin.y + static_cast<float>(cy) * voxel_size.y,
                                grid_origin.z + static_cast<float>(cz) * voxel_size.z
                            );
                            if (compute_smooth_normals && out_norms) {
                                c_nrm[c] = sample_grad(cx, cy, cz);
                            }
                        }

                        for (const auto& tet : TETRAHEDRA) {
                            uint8_t tmask = 0;
                            for (int ti = 0; ti < 4; ++ti) {
                                if (c_val[tet[ti]] < iso_level) {
                                    tmask |= static_cast<uint8_t>(1u << ti);
                                }
                            }
                            if (tmask == 0 || tmask == 0x0F) continue;

                            auto emit_triangle = [&](uint8_t a0, uint8_t a1, uint8_t b0, uint8_t b1, uint8_t c0, uint8_t c1) {
                                if (emitted_verts + 3 > max_verts) return;
                                const godot::Vector3 v0 = interpolate_iso_vertex(c_pos[a0], c_pos[a1], c_val[a0], c_val[a1], iso_level);
                                const godot::Vector3 v1 = interpolate_iso_vertex(c_pos[b0], c_pos[b1], c_val[b0], c_val[b1], iso_level);
                                const godot::Vector3 v2 = interpolate_iso_vertex(c_pos[c0], c_pos[c1], c_val[c0], c_val[c1], iso_level);

                                out_verts[emitted_verts + 0] = v0;
                                out_verts[emitted_verts + 1] = v1;
                                out_verts[emitted_verts + 2] = v2;

                                if (out_norms && emitted_verts + 3 <= max_norms) {
                                    if (compute_smooth_normals) {
                                        out_norms[emitted_verts + 0] = interpolate_iso_vertex(c_nrm[a0], c_nrm[a1], c_val[a0], c_val[a1], iso_level).normalized();
                                        out_norms[emitted_verts + 1] = interpolate_iso_vertex(c_nrm[b0], c_nrm[b1], c_val[b0], c_val[b1], iso_level).normalized();
                                        out_norms[emitted_verts + 2] = interpolate_iso_vertex(c_nrm[c0], c_nrm[c1], c_val[c0], c_val[c1], iso_level).normalized();
                                    } else {
                                        const godot::Vector3 fn = (v1 - v0).cross(v2 - v0).normalized();
                                        out_norms[emitted_verts + 0] = fn;
                                        out_norms[emitted_verts + 1] = fn;
                                        out_norms[emitted_verts + 2] = fn;
                                    }
                                }

                                if (out_indices && emitted_indices + 3 <= max_indices) {
                                    out_indices[emitted_indices + 0] = static_cast<int32_t>(emitted_verts + 0);
                                    out_indices[emitted_indices + 1] = static_cast<int32_t>(emitted_verts + 1);
                                    out_indices[emitted_indices + 2] = static_cast<int32_t>(emitted_verts + 2);
                                    emitted_indices += 3;
                                }

                                emitted_verts += 3;
                            };

                            const uint8_t i0 = tet[0], i1 = tet[1], i2 = tet[2], i3 = tet[3];
                            switch (tmask) {
                                case 0x01: emit_triangle(i0, i1, i0, i2, i0, i3); break;
                                case 0x0E: emit_triangle(i0, i1, i0, i3, i0, i2); break;
                                case 0x02: emit_triangle(i1, i0, i1, i3, i1, i2); break;
                                case 0x0D: emit_triangle(i1, i0, i1, i2, i1, i3); break;
                                case 0x04: emit_triangle(i2, i0, i2, i1, i2, i3); break;
                                case 0x0B: emit_triangle(i2, i0, i2, i3, i2, i1); break;
                                case 0x08: emit_triangle(i3, i0, i3, i2, i3, i1); break;
                                case 0x07: emit_triangle(i3, i0, i3, i1, i3, i2); break;
                                case 0x03:
                                    emit_triangle(i0, i3, i0, i2, i1, i3);
                                    emit_triangle(i1, i3, i0, i2, i1, i2);
                                    break;
                                case 0x0C:
                                    emit_triangle(i0, i3, i1, i3, i0, i2);
                                    emit_triangle(i1, i3, i1, i2, i0, i2);
                                    break;
                                case 0x05:
                                    emit_triangle(i0, i1, i2, i3, i0, i3);
                                    emit_triangle(i0, i1, i2, i1, i2, i3);
                                    break;
                                case 0x0A:
                                    emit_triangle(i0, i1, i0, i3, i2, i3);
                                    emit_triangle(i0, i1, i2, i3, i2, i1);
                                    break;
                                case 0x06:
                                    emit_triangle(i1, i0, i1, i3, i2, i0);
                                    emit_triangle(i2, i0, i1, i3, i2, i3);
                                    break;
                                case 0x09:
                                    emit_triangle(i1, i0, i2, i0, i1, i3);
                                    emit_triangle(i2, i0, i2, i3, i1, i3);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
            }

            vtx_part->selection.element_count = static_cast<int64_t>(emitted_verts);
            if (nrm_part) nrm_part->selection.element_count = static_cast<int64_t>(emitted_verts);
            if (idx_part) idx_part->selection.element_count = static_cast<int64_t>(emitted_indices);
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(SDFMarchingCubesExtractTransformLogic<>) == 64, "SDFMarchingCubesExtractTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SDFMarchingCubesExtractTransformLogic<>>, "SDFMarchingCubesExtractTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core