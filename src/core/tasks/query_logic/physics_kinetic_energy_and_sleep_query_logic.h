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
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * P7: PhysicsKineticEnergyAndSleepQueryLogic
 * Evaluates translational and rotational kinetic energy E_k = 0.5*m*|v|^2 + 0.5*I*|w|^2
 * (or queries Jolt3D's BODY_STATE_SLEEPING state on PhysicsServer3D body RIDs) to wake
 * active/high-energy bodies or sleep settled debris shards.
 */
struct PhysicsKineticEnergyAndSleepQueryLogic {
    static constexpr std::string_view ui_name = "Jolt3D Kinetic Energy & Sleep State Query";
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

    int32_t evaluation_mode = 0; // 0 = Velocity Buffer Kinetic Energy >= threshold, 1 = Query Jolt3D BODY_STATE_SLEEPING
    float energy_threshold = 0.05f;
    float default_mass = 1.0f;
    float default_inertia = 0.4f;
    bool match_awake_bodies = true;

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
        add_prop("evaluation_mode", godot::Variant::INT, evaluation_mode, 2, "DOD Velocity Kinetic Energy,Jolt3D BODY_STATE_SLEEPING");
        add_prop("energy_threshold", godot::Variant::FLOAT, energy_threshold);
        add_prop("default_mass", godot::Variant::FLOAT, default_mass);
        add_prop("default_inertia", godot::Variant::FLOAT, default_inertia);
        add_prop("match_awake_bodies", godot::Variant::BOOL, match_awake_bodies);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("evaluation_mode")) evaluation_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["evaluation_mode"]), 0, 1);
        if (p_props.has("energy_threshold")) energy_threshold = std::max(0.0f, static_cast<float>(p_props["energy_threshold"]));
        if (p_props.has("default_mass")) default_mass = std::max(1e-4f, static_cast<float>(p_props["default_mass"]));
        if (p_props.has("default_inertia")) default_inertia = std::max(0.0f, static_cast<float>(p_props["default_inertia"]));
        if (p_props.has("match_awake_bodies")) match_awake_bodies = static_cast<bool>(p_props["match_awake_bodies"]);
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
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& primary = p_ctx.grant->parts[0];
        if (!primary.raw_base_ptr) return;

        MemoryBufferSelectionPOD& sel = primary.selection;

        if (evaluation_mode == 1) {
            godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
            if (!ps) return;

            const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), primary.element_stride);
            const int64_t n_bodies = static_cast<int64_t>(primary.capacity_bytes / r_stride);
            for (int64_t i = 0; i < n_bodies; ++i) {
                const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(primary.raw_base_ptr + static_cast<size_t>(i) * r_stride);
                if (raw_id == 0) continue;
                godot::RID body_rid;
                *(reinterpret_cast<uint64_t*>(&body_rid)) = raw_id;
                if (!body_rid.is_valid()) continue;

                const bool is_sleeping = static_cast<bool>(ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_SLEEPING));
                const bool is_awake = !is_sleeping;
                if (is_awake == match_awake_bodies) {
                    apply_selection_op(sel, i, p_op);
                }
            }
        } else {
            const GrantPartPOD* ang_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
                ? &p_ctx.grant->parts[1]
                : nullptr;
            const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), primary.element_stride);
            const int64_t n_elems = static_cast<int64_t>(primary.capacity_bytes / v_stride);

            const uint32_t w_stride = ang_part ? std::max<uint32_t>(sizeof(godot::Vector3), ang_part->element_stride) : sizeof(godot::Vector3);
            const int64_t max_w = ang_part ? static_cast<int64_t>(ang_part->capacity_bytes / w_stride) : 0;

            for (int64_t i = 0; i < n_elems; ++i) {
                const godot::Vector3& v = *reinterpret_cast<const godot::Vector3*>(primary.raw_base_ptr + static_cast<size_t>(i) * v_stride);
                float ke = 0.5f * default_mass * v.length_squared();
                if (ang_part && i < max_w) {
                    const godot::Vector3& w = *reinterpret_cast<const godot::Vector3*>(ang_part->raw_base_ptr + static_cast<size_t>(i) * w_stride);
                    ke += 0.5f * default_inertia * w.length_squared();
                }

                const bool above = (ke >= energy_threshold);
                if (above == match_awake_bodies) {
                    apply_selection_op(sel, i, p_op);
                }
            }
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