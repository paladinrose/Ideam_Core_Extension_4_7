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
 * V10: ParticleCameraDepthBucketMetadataLogic
 * Quantizes camera-space depth along camera_forward from camera_origin into
 * r_selection.partition_ids (for back-to-front translucent radix/bucket sorting)
 * and distance LOD tiers into r_selection.lod_levels.
 */
struct ParticleCameraDepthBucketMetadataLogic {
    static constexpr std::string_view ui_name = "Particle Camera Depth Bucket & LOD Metadata";
    static constexpr std::string_view ui_category = "Particles/Metadata";

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

    godot::Vector3 camera_origin = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::Vector3 camera_forward = godot::Vector3(0.0f, 0.0f, -1.0f);
    float near_plane = 0.1f;
    float far_plane = 200.0f;
    int32_t num_depth_buckets = 1024;
    int32_t num_lod_tiers = 4;
    bool back_to_front_order = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("camera_origin", godot::Variant::VECTOR3, camera_origin);
        add_prop("camera_forward", godot::Variant::VECTOR3, camera_forward);
        add_prop("near_plane", godot::Variant::FLOAT, near_plane);
        add_prop("far_plane", godot::Variant::FLOAT, far_plane);
        add_prop("num_depth_buckets", godot::Variant::INT, num_depth_buckets);
        add_prop("num_lod_tiers", godot::Variant::INT, num_lod_tiers);
        add_prop("back_to_front_order", godot::Variant::BOOL, back_to_front_order);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("camera_origin")) camera_origin = p_props["camera_origin"];
        if (p_props.has("camera_forward")) {
            godot::Vector3 f = p_props["camera_forward"];
            camera_forward = (f.length_squared() > 1e-8f) ? f.normalized() : godot::Vector3(0.0f, 0.0f, -1.0f);
        }
        if (p_props.has("near_plane")) near_plane = std::max(1e-3f, static_cast<float>(p_props["near_plane"]));
        if (p_props.has("far_plane")) far_plane = std::max(near_plane + 1e-2f, static_cast<float>(p_props["far_plane"]));
        if (p_props.has("num_depth_buckets")) num_depth_buckets = std::max<int32_t>(2, static_cast<int32_t>(p_props["num_depth_buckets"]));
        if (p_props.has("num_lod_tiers")) num_lod_tiers = std::clamp<int32_t>(static_cast<int32_t>(p_props["num_lod_tiers"]), 1, 255);
        if (p_props.has("back_to_front_order")) back_to_front_order = static_cast<bool>(p_props["back_to_front_order"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& pos_part = p_ctx.grant->parts[0];
        MemoryBufferSelectionPOD& sel = pos_part.selection;
        if (!pos_part.raw_base_ptr || !sel.partition_ids || sel.capacity <= 0) return;

        const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part.element_stride);
        const int64_t n_particles = std::min<int64_t>(
            (sel.element_count > 0) ? sel.element_count : sel.capacity,
            static_cast<int64_t>(pos_part.capacity_bytes / p_stride)
        );

        const godot::Vector3 fwd = (camera_forward.length_squared() > 1e-8f)
            ? camera_forward.normalized()
            : godot::Vector3(0.0f, 0.0f, -1.0f);
        const float span = std::max(1e-4f, far_plane - near_plane);

        for (int64_t i = 0; i < n_particles; ++i) {
            const godot::Vector3& p = *reinterpret_cast<const godot::Vector3*>(pos_part.raw_base_ptr + static_cast<size_t>(i) * p_stride);
            const float z_depth = (p - camera_origin).dot(fwd);
            const float u = std::clamp((z_depth - near_plane) / span, 0.0f, 1.0f);

            const float bucket_u = back_to_front_order ? (1.0f - u) : u;
            const int64_t bucket = std::clamp<int64_t>(
                static_cast<int64_t>(bucket_u * static_cast<float>(num_depth_buckets - 1) + 0.5f),
                0,
                num_depth_buckets - 1
            );
            sel.partition_ids[i] = bucket;

            if (sel.lod_levels) {
                const int32_t lod = std::clamp<int32_t>(
                    static_cast<int32_t>(u * static_cast<float>(num_lod_tiers)),
                    0,
                    num_lod_tiers - 1
                );
                sel.lod_levels[i] = static_cast<uint8_t>(lod);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core