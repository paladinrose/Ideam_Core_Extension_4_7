#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/physics_shape_query_parameters3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * P6: PhysicsBroadphaseOverlapQueryLogic
 * Queries Jolt 3D's broadphase/narrowphase (PhysicsDirectSpaceState3D::intersect_shape)
 * at a query transform or per-element probe position, waking or sleeping elements in r_selection
 * that overlap any collider matching collision_mask.
 */
struct PhysicsBroadphaseOverlapQueryLogic {
    static constexpr std::string_view ui_name = "Jolt3D Broadphase & Shape Overlap Query";
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
        DataType::UINT32
    };
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::WAKE,
        QueryOp::SLEEP,
        QueryOp::INVERT
    };

    godot::RID space_rid;
    godot::RID probe_shape_rid;
    float fallback_sphere_radius = 1.0f;
    uint32_t collision_mask = 0xFFFFFFFFu;
    int32_t max_results_per_probe = 8;
    bool collide_with_bodies = true;
    bool collide_with_areas = false;

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
        add_prop("probe_shape_rid", godot::Variant::RID, probe_shape_rid);
        add_prop("fallback_sphere_radius", godot::Variant::FLOAT, fallback_sphere_radius);
        add_prop("collision_mask", godot::Variant::INT, static_cast<int64_t>(collision_mask));
        add_prop("max_results_per_probe", godot::Variant::INT, max_results_per_probe);
        add_prop("collide_with_bodies", godot::Variant::BOOL, collide_with_bodies);
        add_prop("collide_with_areas", godot::Variant::BOOL, collide_with_areas);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        auto parse_rid = [](const godot::Variant& v) noexcept -> godot::RID {
            if (v.get_type() == godot::Variant::RID) return v;
            if (v.get_type() == godot::Variant::INT) {
                godot::RID r;
                *(reinterpret_cast<uint64_t*>(&r)) = static_cast<uint64_t>(static_cast<int64_t>(v));
                return r;
            }
            return godot::RID();
        };
        if (p_props.has("space_rid")) space_rid = parse_rid(p_props["space_rid"]);
        if (p_props.has("probe_shape_rid")) probe_shape_rid = parse_rid(p_props["probe_shape_rid"]);
        if (p_props.has("fallback_sphere_radius")) fallback_sphere_radius = std::max(1e-4f, static_cast<float>(p_props["fallback_sphere_radius"]));
        if (p_props.has("collision_mask")) collision_mask = static_cast<uint32_t>(static_cast<int64_t>(p_props["collision_mask"]));
        if (p_props.has("max_results_per_probe")) max_results_per_probe = std::clamp<int32_t>(static_cast<int32_t>(p_props["max_results_per_probe"]), 1, 64);
        if (p_props.has("collide_with_bodies")) collide_with_bodies = static_cast<bool>(p_props["collide_with_bodies"]);
        if (p_props.has("collide_with_areas")) collide_with_areas = static_cast<bool>(p_props["collide_with_areas"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline void apply_selection_op(MemoryBufferSelectionPOD& r_sel, int64_t idx, QueryOp p_op) noexcept {
        if (idx < 0 || idx >= r_sel.capacity) return;
        if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
            const uint64_t word = static_cast<uint64_t>(idx) >> 6;
            const uint64_t bit = 1ULL << (idx & 63);
            const bool was_set = (r_sel.data.bitset[word] & bit) != 0;
            if (p_op == QueryOp::WAKE && !was_set) {
                r_sel.data.bitset[word] |= bit;
                r_sel.element_count++;
            } else if (p_op == QueryOp::SLEEP && was_set) {
                r_sel.data.bitset[word] &= ~bit;
                r_sel.element_count = std::max<int64_t>(0, r_sel.element_count - 1);
            } else if (p_op == QueryOp::INVERT) {
                r_sel.data.bitset[word] ^= bit;
                r_sel.element_count += was_set ? -1 : 1;
            }
        } else if (r_sel.mode == SelectionMode::SPARSE && r_sel.data.indices) {
            if (p_op == QueryOp::WAKE && r_sel.element_count < r_sel.capacity) {
                r_sel.data.indices[r_sel.element_count++] = idx;
            }
        }
    }

    void execute_with_op(const TaskContextPOD& p_ctx, QueryOp p_op) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !space_rid.is_valid()) return;

        GrantPartPOD& pos_part = p_ctx.grant->parts[0];
        if (!pos_part.raw_base_ptr) return;

        godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
        if (!ps) return;
        godot::PhysicsDirectSpaceState3D* space_state = ps->space_get_direct_state(space_rid);
        if (!space_state) return;

        godot::RID active_shape = probe_shape_rid;
        bool owns_shape = false;
        if (!active_shape.is_valid()) {
            active_shape = ps->sphere_shape_create();
            ps->shape_set_data(active_shape, fallback_sphere_radius);
            owns_shape = true;
        }

        godot::Ref<godot::PhysicsShapeQueryParameters3D> params;
        params.instantiate();
        params->set_shape_rid(active_shape);
        params->set_collision_mask(collision_mask);
        params->set_collide_with_bodies(collide_with_bodies);
        params->set_collide_with_areas(collide_with_areas);

        MemoryBufferSelectionPOD& sel = pos_part.selection;
        const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part.element_stride);
        const int64_t n_probes = (sel.element_count > 0)
            ? sel.element_count
            : static_cast<int64_t>(pos_part.capacity_bytes / stride);

        godot::Transform3D xform;
        for (int64_t i = 0; i < n_probes; ++i) {
            xform.origin = *reinterpret_cast<const godot::Vector3*>(pos_part.raw_base_ptr + static_cast<size_t>(i) * stride);
            params->set_transform(xform);

            const godot::TypedArray<godot::Dictionary> hits = space_state->intersect_shape(params, max_results_per_probe);
            if (!hits.is_empty()) {
                apply_selection_op(sel, i, p_op);
            }
        }

        if (owns_shape && active_shape.is_valid()) {
            ps->free_rid(active_shape);
        }
    }

    void execute(const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, QueryOp p_op, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, p_op);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }
};

} // namespace ideam::core