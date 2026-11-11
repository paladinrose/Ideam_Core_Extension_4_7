#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * P5: PhysicsBatchRaycastAndShapeCastTransformLogic
 * Executes batched 3D raycasts over Jolt3D's lock-free BVH via PhysicsDirectSpaceState3D::intersect_ray,
 * reading ray start (Part 0) and ray end (Part 1) buffers and writing hit positions (Part 2),
 * hit normals (Part 3), and hit object/collider IDs (Part 4).
 */
struct PhysicsBatchRaycastAndShapeCastTransformLogic {
    static constexpr std::string_view ui_name = "Jolt3D Batch Raycast & Surface Hit Extractor";
    static constexpr std::string_view ui_category = "Physics/Queries";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::INT32
    };

    godot::RID space_rid;
    uint32_t collision_mask = 0xFFFFFFFFu;
    bool collide_with_bodies = true;
    bool collide_with_areas = false;
    bool hit_from_inside = false;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("space_rid", godot::Variant::RID, space_rid);
        add_prop("collision_mask", godot::Variant::INT, static_cast<int64_t>(collision_mask));
        add_prop("collide_with_bodies", godot::Variant::BOOL, collide_with_bodies);
        add_prop("collide_with_areas", godot::Variant::BOOL, collide_with_areas);
        add_prop("hit_from_inside", godot::Variant::BOOL, hit_from_inside);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("space_rid")) {
            godot::Variant v = p_props["space_rid"];
            if (v.get_type() == godot::Variant::RID) {
                space_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&space_rid)) = raw;
            }
        }
        if (p_props.has("collision_mask")) collision_mask = static_cast<uint32_t>(static_cast<int64_t>(p_props["collision_mask"]));
        if (p_props.has("collide_with_bodies")) collide_with_bodies = static_cast<bool>(p_props["collide_with_bodies"]);
        if (p_props.has("collide_with_areas")) collide_with_areas = static_cast<bool>(p_props["collide_with_areas"]);
        if (p_props.has("hit_from_inside")) hit_from_inside = static_cast<bool>(p_props["hit_from_inside"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 3 || !space_rid.is_valid()) return;

        const GrantPartPOD& from_part = p_ctx.grant->parts[0];
        const GrantPartPOD& to_part = p_ctx.grant->parts[1];
        GrantPartPOD& hit_pos_part = p_ctx.grant->parts[2];
        if (!from_part.raw_base_ptr || !to_part.raw_base_ptr || !hit_pos_part.raw_base_ptr) return;

        GrantPartPOD* hit_nrm_part = (p_ctx.grant->part_count > 3 && p_ctx.grant->parts[3].raw_base_ptr)
            ? &p_ctx.grant->parts[3]
            : nullptr;
        GrantPartPOD* hit_id_part = (p_ctx.grant->part_count > 4 && p_ctx.grant->parts[4].raw_base_ptr)
            ? &p_ctx.grant->parts[4]
            : nullptr;

        godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
        if (!ps) return;
        godot::PhysicsDirectSpaceState3D* space_state = ps->space_get_direct_state(space_rid);
        if (!space_state) return;

        const uint32_t f_stride = std::max<uint32_t>(sizeof(godot::Vector3), from_part.element_stride);
        const uint32_t t_stride = std::max<uint32_t>(sizeof(godot::Vector3), to_part.element_stride);
        const uint32_t hp_stride = std::max<uint32_t>(sizeof(godot::Vector3), hit_pos_part.element_stride);
        const int64_t n_rays = std::min<int64_t>({
            static_cast<int64_t>(from_part.capacity_bytes / f_stride),
            static_cast<int64_t>(to_part.capacity_bytes / t_stride),
            static_cast<int64_t>(hit_pos_part.capacity_bytes / hp_stride)
        });
        if (n_rays <= 0) return;

        godot::Ref<godot::PhysicsRayQueryParameters3D> query;
        query.instantiate();
        query->set_collision_mask(collision_mask);
        query->set_collide_with_bodies(collide_with_bodies);
        query->set_collide_with_areas(collide_with_areas);
        query->set_hit_from_inside(hit_from_inside);

        for (int64_t i = 0; i < n_rays; ++i) {
            const godot::Vector3& from_pt = *reinterpret_cast<const godot::Vector3*>(from_part.raw_base_ptr + static_cast<size_t>(i) * f_stride);
            const godot::Vector3& to_pt = *reinterpret_cast<const godot::Vector3*>(to_part.raw_base_ptr + static_cast<size_t>(i) * t_stride);

            query->set_from(from_pt);
            query->set_to(to_pt);

            const godot::Dictionary res = space_state->intersect_ray(query);
            godot::Vector3* out_p = reinterpret_cast<godot::Vector3*>(hit_pos_part.raw_base_ptr + static_cast<size_t>(i) * hp_stride);

            if (!res.is_empty()) {
                *out_p = res["position"];
                if (hit_nrm_part && static_cast<size_t>(i + 1) * sizeof(godot::Vector3) <= hit_nrm_part->capacity_bytes) {
                    reinterpret_cast<godot::Vector3*>(hit_nrm_part->raw_base_ptr)[i] = res["normal"];
                }
                if (hit_id_part && static_cast<size_t>(i + 1) * sizeof(int64_t) <= hit_id_part->capacity_bytes) {
                    reinterpret_cast<int64_t*>(hit_id_part->raw_base_ptr)[i] = static_cast<int64_t>(res["collider_id"]);
                }
            } else {
                *out_p = to_pt;
                if (hit_nrm_part && static_cast<size_t>(i + 1) * sizeof(godot::Vector3) <= hit_nrm_part->capacity_bytes) {
                    reinterpret_cast<godot::Vector3*>(hit_nrm_part->raw_base_ptr)[i] = godot::Vector3(0.0f, 1.0f, 0.0f);
                }
                if (hit_id_part && static_cast<size_t>(i + 1) * sizeof(int64_t) <= hit_id_part->capacity_bytes) {
                    reinterpret_cast<int64_t*>(hit_id_part->raw_base_ptr)[i] = 0;
                }
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core