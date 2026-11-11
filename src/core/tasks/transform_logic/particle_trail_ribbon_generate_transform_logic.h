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
 * V7: ParticleTrailRibbonGenerateTransformLogic
 * Builds camera-facing billboarded or velocity-aligned triangle-strip ribbon geometry (Part 1 vertices
 * and optional Part 2 UVs) from a ring/history buffer of 3D particle positions (Part 0).
 */
struct ParticleTrailRibbonGenerateTransformLogic {
    static constexpr std::string_view ui_name = "Particle Ring-History Trail Ribbon Generator";
    static constexpr std::string_view ui_category = "Particles/Rendering";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::RING,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::VECTOR2
    };

    int32_t num_trails = 64;
    int32_t trail_history_steps = 8;
    float head_half_width = 0.15f;
    float tail_half_width = 0.01f;
    godot::Vector3 camera_origin = godot::Vector3(0.0f, 5.0f, 10.0f);
    bool camera_billboard_alignment = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("num_trails", godot::Variant::INT, num_trails);
        add_prop("trail_history_steps", godot::Variant::INT, trail_history_steps);
        add_prop("head_half_width", godot::Variant::FLOAT, head_half_width);
        add_prop("tail_half_width", godot::Variant::FLOAT, tail_half_width);
        add_prop("camera_origin", godot::Variant::VECTOR3, camera_origin);
        add_prop("camera_billboard_alignment", godot::Variant::BOOL, camera_billboard_alignment);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("num_trails")) num_trails = std::max<int32_t>(1, static_cast<int32_t>(p_props["num_trails"]));
        if (p_props.has("trail_history_steps")) trail_history_steps = std::max<int32_t>(2, static_cast<int32_t>(p_props["trail_history_steps"]));
        if (p_props.has("head_half_width")) head_half_width = std::max(0.0f, static_cast<float>(p_props["head_half_width"]));
        if (p_props.has("tail_half_width")) tail_half_width = std::max(0.0f, static_cast<float>(p_props["tail_half_width"]));
        if (p_props.has("camera_origin")) camera_origin = p_props["camera_origin"];
        if (p_props.has("camera_billboard_alignment")) camera_billboard_alignment = static_cast<bool>(p_props["camera_billboard_alignment"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& hist_part = p_ctx.grant->parts[0];
        GrantPartPOD& vtx_part = p_ctx.grant->parts[1];
        if (!hist_part.raw_base_ptr || !vtx_part.raw_base_ptr) return;

        GrantPartPOD* uv_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        const int32_t steps = std::max<int32_t>(2, trail_history_steps);
        const int32_t avail_samples = static_cast<int32_t>(hist_part.capacity_bytes / sizeof(godot::Vector3));
        const int32_t trails = std::min<int32_t>(num_trails, avail_samples / steps);
        if (trails <= 0) return;

        const godot::Vector3* hist = reinterpret_cast<const godot::Vector3*>(hist_part.raw_base_ptr);
        godot::Vector3* out_v = reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr);
        const size_t max_out_v = vtx_part.capacity_bytes / sizeof(godot::Vector3);

        godot::Vector2* out_uv = uv_part ? reinterpret_cast<godot::Vector2*>(uv_part->raw_base_ptr) : nullptr;
        const size_t max_out_uv = uv_part ? (uv_part->capacity_bytes / sizeof(godot::Vector2)) : 0;

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

        vtx_part.selection.element_count = static_cast<int64_t>(emitted_verts);
        if (uv_part) uv_part->selection.element_count = static_cast<int64_t>(emitted_verts);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core