#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/ring_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ParticleTrailRibbonGenerateTransformLogic<T, T_Strategy, T_View> (V7)
 * Builds camera-facing billboarded or velocity-aligned triangle-strip ribbon geometry
 * (ribbon vertices and optional UVs) from a ring/history buffer of 3D particle positions.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = RingView
>
struct alignas(64) ParticleTrailRibbonGenerateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities =
        ViewCapability::LINEAR_ACCESS | ViewCapability::QUEUE_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::RING | BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types =
        DataType::VECTOR3 | DataType::VECTOR2 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Particle Ring-History Trail Ribbon Generator";
    static constexpr std::string_view ui_category  = "Particles/Rendering";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::Vector3 camera_origin      = godot::Vector3(0.0f, 5.0f, 10.0f); // 12B
    uint32_t target_buffer_id         = INVALID_ID;                        // 4B -> 16B
    uint32_t trail_history_buffer_id  = INVALID_ID;                        // 4B: Ring/history Vector3 buffer
    uint32_t ribbon_vertex_buffer_id  = INVALID_ID;                        // 4B: Output triangle Vector3 vertices
    uint32_t ribbon_uv_buffer_id      = INVALID_ID;                        // 4B: Optional output Vector2 UVs
    int32_t num_trails                = 64;                                // 4B -> 32B
    int32_t trail_history_steps       = 8;                                 // 4B
    float head_half_width             = 0.15f;                             // 4B
    float tail_half_width             = 0.01f;                             // 4B
    bool camera_billboard_alignment   = true;                              // 1B
    uint8_t reserved_padding[19]      = {0};                               // 19B -> Total = 64B

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
        add_buf("trail_history_buffer_id");
        add_buf("ribbon_vertex_buffer_id");
        add_buf("ribbon_uv_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("num_trails", godot::Variant::INT, 64);
        add_prop("trail_history_steps", godot::Variant::INT, 8);
        add_prop("head_half_width", godot::Variant::FLOAT, 0.15f);
        add_prop("tail_half_width", godot::Variant::FLOAT, 0.01f);
        add_prop("camera_origin", godot::Variant::VECTOR3, godot::Vector3(0.0f, 5.0f, 10.0f));
        add_prop("camera_billboard_alignment", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (ribbon_vertex_buffer_id != INVALID_ID) ? ribbon_vertex_buffer_id : trail_history_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("trail_history_buffer_id")) trail_history_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["trail_history_buffer_id"]));
        if (p_props.has("ribbon_vertex_buffer_id")) ribbon_vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ribbon_vertex_buffer_id"]));
        if (p_props.has("ribbon_uv_buffer_id")) ribbon_uv_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ribbon_uv_buffer_id"]));
        if (p_props.has("num_trails")) num_trails = std::max<int32_t>(1, static_cast<int32_t>(p_props["num_trails"]));
        if (p_props.has("trail_history_steps")) trail_history_steps = std::max<int32_t>(2, static_cast<int32_t>(p_props["trail_history_steps"]));
        if (p_props.has("head_half_width")) head_half_width = std::max(0.0f, static_cast<float>(p_props["head_half_width"]));
        if (p_props.has("tail_half_width")) tail_half_width = std::max(0.0f, static_cast<float>(p_props["tail_half_width"]));
        if (p_props.has("camera_origin")) camera_origin = p_props["camera_origin"];
        if (p_props.has("camera_billboard_alignment")) camera_billboard_alignment = static_cast<bool>(p_props["camera_billboard_alignment"]);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* hist_part = (trail_history_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(trail_history_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);

            GrantPartPOD* vtx_part = (ribbon_vertex_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(ribbon_vertex_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);

            if (!hist_part || !hist_part->raw_base_ptr || !vtx_part || !vtx_part->raw_base_ptr) return;

            GrantPartPOD* uv_part = (ribbon_uv_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(ribbon_uv_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : nullptr);

            const int32_t steps = std::max<int32_t>(2, trail_history_steps);
            const int32_t avail_samples = static_cast<int32_t>(hist_part->capacity_bytes / sizeof(godot::Vector3));
            const int32_t trails = std::min<int32_t>(num_trails, avail_samples / steps);
            if (trails <= 0) return;

            const godot::Vector3* hist = reinterpret_cast<const godot::Vector3*>(hist_part->raw_base_ptr);
            godot::Vector3* out_v = reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr);
            const size_t max_out_v = vtx_part->capacity_bytes / sizeof(godot::Vector3);

            godot::Vector2* out_uv = (uv_part && uv_part->raw_base_ptr)
                ? reinterpret_cast<godot::Vector2*>(uv_part->raw_base_ptr)
                : nullptr;
            const size_t max_out_uv = out_uv ? (uv_part->capacity_bytes / sizeof(godot::Vector2)) : 0;

            size_t emitted_verts = 0;

            for (int32_t tr = 0; tr < trails; ++tr) {
                const godot::Vector3* chain = hist + static_cast<size_t>(tr) * steps;

                for (int32_t s = 0; s < steps - 1; ++s) {
                    if (emitted_verts + 6 > max_out_v) break;

                    const godot::Vector3 p0 = chain[s];
                    const godot::Vector3 p1 = chain[s + 1];
                    const float u0 = static_cast<float>(s) / static_cast<float>(steps - 1);
                    const float u1 = static_cast<float>(s + 1) / static_cast<float>(steps - 1);

                    const float w0 = head_half_width + (tail_half_width - head_half_width) * u0;
                    const float w1 = head_half_width + (tail_half_width - head_half_width) * u1;

                    godot::Vector3 tang = p1 - p0;
                    tang = (tang.length_squared() > 1e-10f) ? tang.normalized() : godot::Vector3(0.0f, 0.0f, 1.0f);

                    const godot::Vector3 view_dir = camera_billboard_alignment
                        ? (camera_origin - p0).normalized()
                        : ((std::abs(tang.y) < 0.99f) ? godot::Vector3(0.0f, 1.0f, 0.0f) : godot::Vector3(1.0f, 0.0f, 0.0f));
                    const godot::Vector3 side = tang.cross(view_dir).normalized();

                    const godot::Vector3 v0_l = p0 - side * w0;
                    const godot::Vector3 v0_r = p0 + side * w0;
                    const godot::Vector3 v1_l = p1 - side * w1;
                    const godot::Vector3 v1_r = p1 + side * w1;

                    out_v[emitted_verts + 0] = v0_l;
                    out_v[emitted_verts + 1] = v0_r;
                    out_v[emitted_verts + 2] = v1_r;
                    out_v[emitted_verts + 3] = v0_l;
                    out_v[emitted_verts + 4] = v1_r;
                    out_v[emitted_verts + 5] = v1_l;

                    if (out_uv && emitted_verts + 6 <= max_out_uv) {
                        out_uv[emitted_verts + 0] = godot::Vector2(0.0f, u0);
                        out_uv[emitted_verts + 1] = godot::Vector2(1.0f, u0);
                        out_uv[emitted_verts + 2] = godot::Vector2(1.0f, u1);
                        out_uv[emitted_verts + 3] = godot::Vector2(0.0f, u0);
                        out_uv[emitted_verts + 4] = godot::Vector2(1.0f, u1);
                        out_uv[emitted_verts + 5] = godot::Vector2(0.0f, u1);
                    }

                    emitted_verts += 6;
                }
            }

            vtx_part->selection.element_count = static_cast<int64_t>(emitted_verts);
            if (uv_part) uv_part->selection.element_count = static_cast<int64_t>(emitted_verts);
        }
    }
};

static_assert(sizeof(ParticleTrailRibbonGenerateTransformLogic<>) == 64, "ParticleTrailRibbonGenerateTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ParticleTrailRibbonGenerateTransformLogic<>>, "ParticleTrailRibbonGenerateTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core