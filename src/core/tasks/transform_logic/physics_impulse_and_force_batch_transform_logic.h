#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * P2: PhysicsImpulseAndForceBatchTransformLogic
 * Applies batched central impulses, positioned impulses, torque impulses, continuous forces,
 * or radial explosion shockwaves directly to Jolt 3D body RIDs via PhysicsServer3D.
 */
struct PhysicsImpulseAndForceBatchTransformLogic {
    static constexpr std::string_view ui_name = "Jolt3D Impulse, Force & Blast Wave Batch";
    static constexpr std::string_view ui_category = "Physics/Bodies";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::UINT32
    };

    int32_t application_mode = 0; // 0 = Central Impulse, 1 = Continuous Central Force, 2 = Torque Impulse, 3 = Positioned Impulse, 4 = Radial Blast Wave
    godot::Vector3 blast_origin = godot::Vector3(0.0f, 0.0f, 0.0f);
    float blast_radius = 10.0f;
    float blast_impulse_magnitude = 25.0f;
    float blast_upward_bias = 0.25f;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            list.push_back(d);
        };
        add_prop("application_mode", godot::Variant::INT, application_mode, 2, "Central Impulse,Continuous Force,Torque Impulse,Positioned Impulse,Radial Blast Wave");
        add_prop("blast_origin", godot::Variant::VECTOR3, blast_origin);
        add_prop("blast_radius", godot::Variant::FLOAT, blast_radius);
        add_prop("blast_impulse_magnitude", godot::Variant::FLOAT, blast_impulse_magnitude);
        add_prop("blast_upward_bias", godot::Variant::FLOAT, blast_upward_bias);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("application_mode")) application_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["application_mode"]), 0, 4);
        if (p_props.has("blast_origin")) blast_origin = p_props["blast_origin"];
        if (p_props.has("blast_radius")) blast_radius = std::max(1e-4f, static_cast<float>(p_props["blast_radius"]));
        if (p_props.has("blast_impulse_magnitude")) blast_impulse_magnitude = static_cast<float>(p_props["blast_impulse_magnitude"]);
        if (p_props.has("blast_upward_bias")) blast_upward_bias = static_cast<float>(p_props["blast_upward_bias"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        const GrantPartPOD& rid_part = p_ctx.grant->parts[0];
        if (!rid_part.raw_base_ptr) return;

        const GrantPartPOD* vec_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;
        const GrantPartPOD* offset_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
        if (!ps) return;

        const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part.element_stride);
        const int64_t n_bodies = (rid_part.selection.element_count > 0)
            ? rid_part.selection.element_count
            : static_cast<int64_t>(rid_part.capacity_bytes / r_stride);

        const uint32_t v_stride = vec_part ? std::max<uint32_t>(sizeof(godot::Vector3), vec_part->element_stride) : sizeof(godot::Vector3);
        const int64_t n_vecs = vec_part ? static_cast<int64_t>(vec_part->capacity_bytes / v_stride) : 0;

        const uint32_t o_stride = offset_part ? std::max<uint32_t>(sizeof(godot::Vector3), offset_part->element_stride) : sizeof(godot::Vector3);
        const int64_t n_offsets = offset_part ? static_cast<int64_t>(offset_part->capacity_bytes / o_stride) : 0;

        for (int64_t i = 0; i < n_bodies; ++i) {
            const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(rid_part.raw_base_ptr + static_cast<size_t>(i) * r_stride);
            if (raw_id == 0) continue;
            godot::RID body_rid;
            *(reinterpret_cast<uint64_t*>(&body_rid)) = raw_id;
            if (!body_rid.is_valid()) continue;

            if (application_mode == 4) {
                const godot::Transform3D xform = ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM);
                const godot::Vector3 rel = xform.origin - blast_origin;
                const float dist = rel.length();
                if (dist >= blast_radius) continue;

                const float falloff = (1.0f - dist / blast_radius) * (1.0f - dist / blast_radius);
                godot::Vector3 dir = (dist > 1e-5f) ? (rel / dist) : godot::Vector3(0.0f, 1.0f, 0.0f);
                dir = (dir + godot::Vector3(0.0f, blast_upward_bias, 0.0f)).normalized();
                ps->body_apply_central_impulse(body_rid, dir * (blast_impulse_magnitude * falloff));
                continue;
            }

            if (!vec_part || i >= n_vecs) continue;
            const godot::Vector3& vec = *reinterpret_cast<const godot::Vector3*>(vec_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);

            if (application_mode == 0) {
                ps->body_apply_central_impulse(body_rid, vec);
            } else if (application_mode == 1) {
                ps->body_add_central_force(body_rid, vec);
            } else if (application_mode == 2) {
                ps->body_apply_torque_impulse(body_rid, vec);
            } else if (application_mode == 3) {
                const godot::Vector3 pos_offset = (offset_part && i < n_offsets)
                    ? *reinterpret_cast<const godot::Vector3*>(offset_part->raw_base_ptr + static_cast<size_t>(i) * o_stride)
                    : godot::Vector3(0.0f, 0.0f, 0.0f);
                ps->body_apply_impulse(body_rid, vec, pos_offset);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core