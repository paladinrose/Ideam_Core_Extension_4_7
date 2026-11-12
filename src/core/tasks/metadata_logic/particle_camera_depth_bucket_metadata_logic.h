#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "metadata_logic_traits.h"

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
 * ParticleCameraDepthBucketMetadataLogic<T, T_Strategy, T_View> (V10)
 * Quantizes camera-space depth along camera_forward from camera_origin into
 * r_selection.partition_ids (for back-to-front translucent radix/bucket sorting)
 * and distance LOD tiers into r_selection.lod_levels.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) ParticleCameraDepthBucketMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Particle Camera Depth Bucket & LOD Metadata";
    static constexpr std::string_view ui_category  = "Particles/Metadata";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::Vector3 camera_origin  = godot::Vector3(0.0f, 0.0f, 0.0f);  // 12B
    uint32_t target_buffer_id     = INVALID_ID;                        // 4B -> 16B: Particle 3D position buffer
    godot::Vector3 camera_forward = godot::Vector3(0.0f, 0.0f, -1.0f); // 12B
    float near_plane              = 0.1f;                              // 4B -> 32B
    float far_plane               = 200.0f;                            // 4B
    int32_t num_depth_buckets     = 1024;                              // 4B
    int32_t num_lod_tiers         = 4;                                 // 4B
    bool back_to_front_order      = true;                              // 1B
    uint8_t reserved_padding[19]  = {0};                               // 19B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "target_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("camera_origin", godot::Variant::VECTOR3, godot::Vector3(0.0f, 0.0f, 0.0f));
        add_prop("camera_forward", godot::Variant::VECTOR3, godot::Vector3(0.0f, 0.0f, -1.0f));
        add_prop("near_plane", godot::Variant::FLOAT, 0.1f);
        add_prop("far_plane", godot::Variant::FLOAT, 200.0f);
        add_prop("num_depth_buckets", godot::Variant::INT, 1024);
        add_prop("num_lod_tiers", godot::Variant::INT, 4);
        add_prop("back_to_front_order", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("camera_origin")) camera_origin = p_props["camera_origin"];
        if (p_props.has("camera_forward")) {
            const godot::Vector3 f = p_props["camera_forward"];
            camera_forward = (f.length_squared() > 1e-8f) ? f.normalized() : godot::Vector3(0.0f, 0.0f, -1.0f);
        }
        if (p_props.has("near_plane")) near_plane = std::max(1e-3f, static_cast<float>(p_props["near_plane"]));
        if (p_props.has("far_plane")) far_plane = std::max(near_plane + 1e-2f, static_cast<float>(p_props["far_plane"]));
        if (p_props.has("num_depth_buckets")) num_depth_buckets = std::max<int32_t>(2, static_cast<int32_t>(p_props["num_depth_buckets"]));
        if (p_props.has("num_lod_tiers")) num_lod_tiers = std::clamp<int32_t>(static_cast<int32_t>(p_props["num_lod_tiers"]), 1, 255);
        if (p_props.has("back_to_front_order")) back_to_front_order = static_cast<bool>(p_props["back_to_front_order"]);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!r_selection.partition_ids || r_selection.capacity <= 0) return;

            const GrantPartPOD* pos_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!pos_part || !pos_part->raw_base_ptr) return;

            const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part->element_stride);
            const int64_t n_particles = std::min<int64_t>(
                (r_selection.element_count > 0) ? r_selection.element_count : r_selection.capacity,
                static_cast<int64_t>(pos_part->capacity_bytes / p_stride)
            );

            const godot::Vector3 fwd = (camera_forward.length_squared() > 1e-8f)
                ? camera_forward.normalized()
                : godot::Vector3(0.0f, 0.0f, -1.0f);
            const float span = std::max(1e-4f, far_plane - near_plane);

            for (int64_t i = 0; i < n_particles; ++i) {
                const godot::Vector3& p = *reinterpret_cast<const godot::Vector3*>(pos_part->raw_base_ptr + static_cast<size_t>(i) * p_stride);
                const float z_depth = (p - camera_origin).dot(fwd);
                const float u = std::clamp((z_depth - near_plane) / span, 0.0f, 1.0f);

                const float bucket_u = back_to_front_order ? (1.0f - u) : u;
                const int64_t bucket = std::clamp<int64_t>(
                    static_cast<int64_t>(bucket_u * static_cast<float>(num_depth_buckets - 1) + 0.5f),
                    0,
                    num_depth_buckets - 1
                );
                r_selection.partition_ids[i] = bucket;

                if (r_selection.lod_levels) {
                    const int32_t lod = std::clamp<int32_t>(
                        static_cast<int32_t>(u * static_cast<float>(num_lod_tiers)),
                        0,
                        num_lod_tiers - 1
                    );
                    r_selection.lod_levels[i] = static_cast<uint8_t>(lod);
                }
            }
        }
    }
};

static_assert(sizeof(ParticleCameraDepthBucketMetadataLogic<>) == 64, "ParticleCameraDepthBucketMetadataLogic must be exactly 64 bytes!");
static_assert(IsMetadataLogic<ParticleCameraDepthBucketMetadataLogic<>>, "ParticleCameraDepthBucketMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core