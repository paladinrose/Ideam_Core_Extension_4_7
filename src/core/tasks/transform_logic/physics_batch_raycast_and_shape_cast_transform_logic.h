#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * PhysicsBatchRaycastAndShapeCastTransformLogic<T> (P3/P4)
 * Executes batched 3D raycasts over Jolt3D's lock-free BVH via PhysicsDirectSpaceState3D::intersect_ray,
 * reading ray start (ray_from_buffer_id) and ray end (ray_to_buffer_id) buffers and writing
 * hit positions (hit_position_buffer_id), hit normals (hit_normal_buffer_id), and hit collider IDs.
 */
template <typename T = godot::Vector3>
struct alignas(64) PhysicsBatchRaycastAndShapeCastTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = SoAStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::INT32 | DataType::INT64;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Jolt3D Batch Raycast & Surface Hit Extractor";
    static constexpr std::string_view ui_category  = "Physics/Queries";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t ray_from_buffer_id        = INVALID_ID;  // 4B: Ray start positions (Vector3)
    uint32_t ray_to_buffer_id          = INVALID_ID;  // 4B: Ray end positions (Vector3)
    uint32_t hit_position_buffer_id    = INVALID_ID;  // 4B: Primary target hit world positions (Vector3)
    uint32_t hit_normal_buffer_id      = INVALID_ID;  // 4B: Optional hit surface normals (Vector3) -> 16B
    uint32_t hit_collider_id_buffer_id = INVALID_ID;  // 4B: Optional hit collider ObjectIDs (int64_t)
    uint32_t collision_mask            = 0xFFFFFFFFu; // 4B: Physics collision layer mask -> 24B

    godot::RID space_rid;                             // 8B: Target PhysicsServer3D Space RID -> 32B
    bool collide_with_bodies           = true;        // 1B
    bool collide_with_areas            = false;       // 1B
    bool hit_from_inside               = false;       // 1B
    uint8_t reserved_padding[29]       = {0};         // 29B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary fbuf_prop;
        fbuf_prop["name"] = "ray_from_buffer_id";
        fbuf_prop["type"] = godot::Variant::INT;
        fbuf_prop["hint_string"] = "buffer_option";
        props.push_back(fbuf_prop);

        godot::Dictionary tbuf_prop;
        tbuf_prop["name"] = "ray_to_buffer_id";
        tbuf_prop["type"] = godot::Variant::INT;
        tbuf_prop["hint_string"] = "buffer_option";
        props.push_back(tbuf_prop);

        godot::Dictionary hpbuf_prop;
        hpbuf_prop["name"] = "hit_position_buffer_id";
        hpbuf_prop["type"] = godot::Variant::INT;
        hpbuf_prop["hint_string"] = "buffer_option";
        props.push_back(hpbuf_prop);

        godot::Dictionary hnbuf_prop;
        hnbuf_prop["name"] = "hit_normal_buffer_id";
        hnbuf_prop["type"] = godot::Variant::INT;
        hnbuf_prop["hint_string"] = "buffer_option";
        props.push_back(hnbuf_prop);

        godot::Dictionary hcbuf_prop;
        hcbuf_prop["name"] = "hit_collider_id_buffer_id";
        hcbuf_prop["type"] = godot::Variant::INT;
        hcbuf_prop["hint_string"] = "buffer_option";
        props.push_back(hcbuf_prop);

        godot::Dictionary srid_prop;
        srid_prop["name"] = "space_rid";
        srid_prop["type"] = godot::Variant::RID;
        props.push_back(srid_prop);

        godot::Dictionary msk_prop;
        msk_prop["name"] = "collision_mask";
        msk_prop["type"] = godot::Variant::INT;
        msk_prop["hint"] = godot::PROPERTY_HINT_LAYERS_3D_PHYSICS;
        props.push_back(msk_prop);

        godot::Dictionary cwb_prop;
        cwb_prop["name"] = "collide_with_bodies";
        cwb_prop["type"] = godot::Variant::BOOL;
        props.push_back(cwb_prop);

        godot::Dictionary cwa_prop;
        cwa_prop["name"] = "collide_with_areas";
        cwa_prop["type"] = godot::Variant::BOOL;
        props.push_back(cwa_prop);

        godot::Dictionary hfi_prop;
        hfi_prop["name"] = "hit_from_inside";
        hfi_prop["type"] = godot::Variant::BOOL;
        props.push_back(hfi_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (hit_position_buffer_id != INVALID_ID) return hit_position_buffer_id;
        return ray_from_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("ray_from_buffer_id")) {
            ray_from_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ray_from_buffer_id"]));
        }
        if (p_props.has("ray_to_buffer_id")) {
            ray_to_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ray_to_buffer_id"]));
        }
        if (p_props.has("hit_position_buffer_id")) {
            hit_position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["hit_position_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && hit_position_buffer_id == INVALID_ID) {
            hit_position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("hit_normal_buffer_id")) {
            hit_normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["hit_normal_buffer_id"]));
        }
        if (p_props.has("hit_collider_id_buffer_id")) {
            hit_collider_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["hit_collider_id_buffer_id"]));
        }
        if (p_props.has("space_rid")) {
            const godot::Variant v = p_props["space_rid"];
            if (v.get_type() == godot::Variant::RID) {
                space_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&space_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("collision_mask")) {
            collision_mask = static_cast<uint32_t>(static_cast<int64_t>(p_props["collision_mask"]));
        }
        if (p_props.has("collide_with_bodies")) {
            collide_with_bodies = static_cast<bool>(p_props["collide_with_bodies"]);
        }
        if (p_props.has("collide_with_areas")) {
            collide_with_areas = static_cast<bool>(p_props["collide_with_areas"]);
        }
        if (p_props.has("hit_from_inside")) {
            hit_from_inside = static_cast<bool>(p_props["hit_from_inside"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t>) {
            if (!space_rid.is_valid()) return;

            const GrantPartPOD* from_part    = p_context.get_grant_part(ray_from_buffer_id);
            const GrantPartPOD* to_part      = p_context.get_grant_part(ray_to_buffer_id);
            const GrantPartPOD* hit_pos_part = p_context.get_grant_part(hit_position_buffer_id);
            if (!from_part || !to_part || !hit_pos_part || !from_part->raw_base_ptr || !to_part->raw_base_ptr || !hit_pos_part->raw_base_ptr) return;

            const GrantPartPOD* hit_nrm_part = (hit_normal_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(hit_normal_buffer_id)
                : nullptr;
            const GrantPartPOD* hit_id_part  = (hit_collider_id_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(hit_collider_id_buffer_id)
                : nullptr;

            godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
            if (!ps) return;
            godot::PhysicsDirectSpaceState3D* space_state = ps->space_get_direct_state(space_rid);
            if (!space_state) return;

            const uint32_t f_stride  = std::max<uint32_t>(sizeof(godot::Vector3), from_part->element_stride);
            const uint32_t t_stride  = std::max<uint32_t>(sizeof(godot::Vector3), to_part->element_stride);
            const uint32_t hp_stride = std::max<uint32_t>(sizeof(godot::Vector3), hit_pos_part->element_stride);
            const int64_t n_rays = std::min<int64_t>({
                static_cast<int64_t>(from_part->capacity_bytes / f_stride),
                static_cast<int64_t>(to_part->capacity_bytes / t_stride),
                static_cast<int64_t>(hit_pos_part->capacity_bytes / hp_stride)
            });
            if (n_rays <= 0) return;

            godot::Ref<godot::PhysicsRayQueryParameters3D> query;
            query.instantiate();
            query->set_collision_mask(collision_mask);
            query->set_collide_with_bodies(collide_with_bodies);
            query->set_collide_with_areas(collide_with_areas);
            query->set_hit_from_inside(hit_from_inside);

            for (int64_t i = 0; i < n_rays; ++i) {
                const godot::Vector3& from_pt = *reinterpret_cast<const godot::Vector3*>(from_part->raw_base_ptr + static_cast<size_t>(i) * f_stride);
                const godot::Vector3& to_pt   = *reinterpret_cast<const godot::Vector3*>(to_part->raw_base_ptr + static_cast<size_t>(i) * t_stride);

                query->set_from(from_pt);
                query->set_to(to_pt);

                const godot::Dictionary res = space_state->intersect_ray(query);
                godot::Vector3* out_p = reinterpret_cast<godot::Vector3*>(hit_pos_part->raw_base_ptr + static_cast<size_t>(i) * hp_stride);

                if (!res.is_empty()) {
                    *out_p = res["position"];
                    if (hit_nrm_part && hit_nrm_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(godot::Vector3) <= hit_nrm_part->capacity_bytes) {
                        reinterpret_cast<godot::Vector3*>(hit_nrm_part->raw_base_ptr)[i] = res["normal"];
                    }
                    if (hit_id_part && hit_id_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(int64_t) <= hit_id_part->capacity_bytes) {
                        reinterpret_cast<int64_t*>(hit_id_part->raw_base_ptr)[i] = static_cast<int64_t>(res["collider_id"]);
                    }
                } else {
                    *out_p = to_pt;
                    if (hit_nrm_part && hit_nrm_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(godot::Vector3) <= hit_nrm_part->capacity_bytes) {
                        reinterpret_cast<godot::Vector3*>(hit_nrm_part->raw_base_ptr)[i] = godot::Vector3(0.0f, 1.0f, 0.0f);
                    }
                    if (hit_id_part && hit_id_part->raw_base_ptr && static_cast<size_t>(i + 1) * sizeof(int64_t) <= hit_id_part->capacity_bytes) {
                        reinterpret_cast<int64_t*>(hit_id_part->raw_base_ptr)[i] = 0;
                    }
                }
            }
        }
    }
};

static_assert(sizeof(PhysicsBatchRaycastAndShapeCastTransformLogic<>) == 64, "PhysicsBatchRaycastAndShapeCastTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<PhysicsBatchRaycastAndShapeCastTransformLogic<>>, "PhysicsBatchRaycastAndShapeCastTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core