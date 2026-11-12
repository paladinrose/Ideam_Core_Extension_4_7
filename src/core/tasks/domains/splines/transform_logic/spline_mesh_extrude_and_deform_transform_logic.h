#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * SplineMeshExtrudeAndDeformTransformLogic<T, T_Strategy, T_View> (SPL7)
 * Sweeps and deforms 3D mesh vertices (and optional normals) along a 3D/4D spline using
 * precomputed Bishop frames (position, tangent, normal, binormal) from SplineDOD, supporting
 * twist roll, start/end radial taper, and longitudinal repeat/stretch.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SplineMeshExtrudeAndDeformTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    struct BishopSamplePod {
        godot::Vector3 position;
        godot::Vector3 tangent;
        godot::Vector3 normal;
        godot::Vector3 binormal;
    };

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR4;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline Mesh Extrude & Bishop Deform";
    static constexpr std::string_view ui_category  = "Splines/3D";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id       = INVALID_ID; // 4B
    uint32_t vertex_buffer_id       = INVALID_ID; // 4B
    uint32_t bishop_frame_buffer_id = INVALID_ID; // 4B
    uint32_t normal_buffer_id       = INVALID_ID; // 4B -> 16B
    int32_t forward_axis            = 0;          // 4B: 0 = Z, 1 = X, 2 = Y
    float mesh_bounds_min           = 0.0f;       // 4B
    float mesh_bounds_max           = 10.0f;      // 4B
    float twist_radians             = 0.0f;       // 4B -> 32B
    float taper_start               = 1.0f;       // 4B
    float taper_end                 = 1.0f;       // 4B
    bool deform_normals             = true;       // 1B
    uint8_t reserved_padding[23]    = {0};        // 23B -> Total = 64B

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
        add_buf("vertex_buffer_id");
        add_buf("bishop_frame_buffer_id");
        add_buf("normal_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            props.push_back(d);
        };
        add_prop("forward_axis", godot::Variant::INT, 0, 2, "+Z Axis,+X Axis,+Y Axis");
        add_prop("mesh_bounds_min", godot::Variant::FLOAT, 0.0f);
        add_prop("mesh_bounds_max", godot::Variant::FLOAT, 10.0f);
        add_prop("twist_radians", godot::Variant::FLOAT, 0.0f);
        add_prop("taper_start", godot::Variant::FLOAT, 1.0f);
        add_prop("taper_end", godot::Variant::FLOAT, 1.0f);
        add_prop("deform_normals", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("vertex_buffer_id")) vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        if (p_props.has("bishop_frame_buffer_id")) bishop_frame_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["bishop_frame_buffer_id"]));
        if (p_props.has("normal_buffer_id")) normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        if (p_props.has("forward_axis")) forward_axis = std::clamp<int32_t>(static_cast<int32_t>(p_props["forward_axis"]), 0, 2);
        if (p_props.has("mesh_bounds_min")) mesh_bounds_min = static_cast<float>(p_props["mesh_bounds_min"]);
        if (p_props.has("mesh_bounds_max")) mesh_bounds_max = static_cast<float>(p_props["mesh_bounds_max"]);
        if (p_props.has("twist_radians")) twist_radians = static_cast<float>(p_props["twist_radians"]);
        if (p_props.has("taper_start")) taper_start = static_cast<float>(p_props["taper_start"]);
        if (p_props.has("taper_end")) taper_end = static_cast<float>(p_props["taper_end"]);
        if (p_props.has("deform_normals")) deform_normals = static_cast<bool>(p_props["deform_normals"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    static BishopSamplePod sample_bishop_frame(const GrantPartPOD& frame_part, float u) noexcept {
        BishopSamplePod out{
            godot::Vector3(0.0f, 0.0f, u * 10.0f),
            godot::Vector3(0.0f, 0.0f, 1.0f),
            godot::Vector3(1.0f, 0.0f, 0.0f),
            godot::Vector3(0.0f, 1.0f, 0.0f)
        };
        if (!frame_part.raw_base_ptr) return out;

        if (frame_part.element_stride >= sizeof(BishopSamplePod)) {
            const int32_t count = (frame_part.selection.element_count > 0)
                ? static_cast<int32_t>(frame_part.selection.element_count)
                : static_cast<int32_t>(frame_part.capacity_bytes / sizeof(BishopSamplePod));
            if (count <= 0) return out;
            if (count == 1) {
                return *reinterpret_cast<const BishopSamplePod*>(frame_part.raw_base_ptr);
            }
            const float f_idx = std::clamp(u, 0.0f, 1.0f) * static_cast<float>(count - 1);
            const int32_t i0 = std::clamp<int32_t>(static_cast<int32_t>(std::floor(f_idx)), 0, count - 1);
            const int32_t i1 = std::min<int32_t>(count - 1, i0 + 1);
            const float alpha = f_idx - static_cast<float>(i0);

            const auto* frames = reinterpret_cast<const BishopSamplePod*>(frame_part.raw_base_ptr);
            out.position = frames[i0].position.lerp(frames[i1].position, alpha);
            out.tangent = frames[i0].tangent.lerp(frames[i1].tangent, alpha).normalized();
            out.normal = frames[i0].normal.lerp(frames[i1].normal, alpha).normalized();
            out.binormal = out.tangent.cross(out.normal).normalized();
            return out;
        }

        const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), frame_part.element_stride);
        const int32_t count = (frame_part.selection.element_count > 0)
            ? static_cast<int32_t>(frame_part.selection.element_count)
            : static_cast<int32_t>(frame_part.capacity_bytes / stride);
        if (count < 2) return out;

        auto pt = [&](int32_t idx) -> godot::Vector3 {
            return *reinterpret_cast<const godot::Vector3*>(frame_part.raw_base_ptr + static_cast<size_t>(idx) * stride);
        };

        const float f_idx = std::clamp(u, 0.0f, 1.0f) * static_cast<float>(count - 1);
        const int32_t i0 = std::clamp<int32_t>(static_cast<int32_t>(std::floor(f_idx)), 0, count - 2);
        const int32_t i1 = i0 + 1;
        const float alpha = f_idx - static_cast<float>(i0);

        const godot::Vector3 p0 = pt(i0);
        const godot::Vector3 p1 = pt(i1);
        out.position = p0.lerp(p1, alpha);
        godot::Vector3 tang = p1 - p0;
        out.tangent = (tang.length_squared() > 1e-10f) ? tang.normalized() : godot::Vector3(0.0f, 0.0f, 1.0f);
        const godot::Vector3 up = (std::abs(out.tangent.y) < 0.99f) ? godot::Vector3(0.0f, 1.0f, 0.0f) : godot::Vector3(1.0f, 0.0f, 0.0f);
        out.normal = out.tangent.cross(up).normalized();
        out.binormal = out.tangent.cross(out.normal).normalized();
        return out;
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
            GrantPartPOD* vtx_part = (vertex_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(vertex_buffer_id))
                : ((target_buffer_id != INVALID_ID)
                    ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(target_buffer_id))
                    : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr));
            const GrantPartPOD* frame_part = (bishop_frame_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(bishop_frame_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!vtx_part || !vtx_part->raw_base_ptr || !frame_part || !frame_part->raw_base_ptr) return;

            GrantPartPOD* nrm_part = (normal_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(normal_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : nullptr);

            const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part->element_stride);
            const int64_t n_verts = (vtx_part->selection.element_count > 0)
                ? vtx_part->selection.element_count
                : static_cast<int64_t>(vtx_part->capacity_bytes / v_stride);
            const float span = std::max(1e-5f, mesh_bounds_max - mesh_bounds_min);

            for (int64_t i = 0; i < n_verts; ++i) {
                godot::Vector3& v = *reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);

                float along = v.z;
                float ox = v.x;
                float oy = v.y;
                if (forward_axis == 1) {
                    along = v.x; ox = v.y; oy = v.z;
                } else if (forward_axis == 2) {
                    along = v.y; ox = v.x; oy = v.z;
                }

                const float u = std::clamp((along - mesh_bounds_min) / span, 0.0f, 1.0f);
                const BishopSamplePod frame = sample_bishop_frame(*frame_part, u);

                const float angle = twist_radians * u;
                const float ca = std::cos(angle);
                const float sa = std::sin(angle);
                const godot::Vector3 n_rot = frame.normal * ca + frame.binormal * sa;
                const godot::Vector3 b_rot = frame.binormal * ca - frame.normal * sa;
                const float scale = taper_start + (taper_end - taper_start) * u;

                v = frame.position + (n_rot * ox + b_rot * oy) * scale;

                if (deform_normals && nrm_part && nrm_part->raw_base_ptr) {
                    const uint32_t n_stride = std::max<uint32_t>(sizeof(godot::Vector3), nrm_part->element_stride);
                    if (static_cast<size_t>(i + 1) * n_stride <= nrm_part->capacity_bytes) {
                        godot::Vector3& nrm = *reinterpret_cast<godot::Vector3*>(nrm_part->raw_base_ptr + static_cast<size_t>(i) * n_stride);
                        float nl = nrm.z, nx = nrm.x, ny = nrm.y;
                        if (forward_axis == 1) { nl = nrm.x; nx = nrm.y; ny = nrm.z; }
                        else if (forward_axis == 2) { nl = nrm.y; nx = nrm.x; ny = nrm.z; }
                        nrm = (frame.tangent * nl + n_rot * nx + b_rot * ny).normalized();
                    }
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(SplineMeshExtrudeAndDeformTransformLogic<>) == 64, "SplineMeshExtrudeAndDeformTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SplineMeshExtrudeAndDeformTransformLogic<>>, "SplineMeshExtrudeAndDeformTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core