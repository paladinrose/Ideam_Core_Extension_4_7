#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * PhysicsKineticEnergyAndSleepQueryLogic<T, T_Strategy, T_View> (P7)
 * Evaluates translational and rotational kinetic energy E_k = 0.5*m*|v|^2 + 0.5*I*|w|^2
 * (or queries Jolt3D's BODY_STATE_SLEEPING state on PhysicsServer3D body RIDs) to cull
 * settled bodies or add active/high-energy bodies.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) PhysicsKineticEnergyAndSleepQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::UINT32 | DataType::UINT64;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Jolt3D Kinetic Energy & Sleep State Query";
    static constexpr std::string_view ui_category  = "Physics/Queries";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id           = INVALID_ID; // 4B: Velocity Vector3 or Body RID uint64 buffer
    uint32_t angular_velocity_buffer_id = INVALID_ID; // 4B: Optional Angular Velocity Vector3 buffer
    int32_t evaluation_mode             = 0;          // 4B: 0 = Velocity Kinetic Energy, 1 = BODY_STATE_SLEEPING
    float energy_threshold              = 0.05f;      // 4B -> 16B
    float default_mass                  = 1.0f;       // 4B
    float default_inertia               = 0.4f;       // 4B
    bool match_awake_bodies             = true;       // 1B
    uint8_t reserved_padding[39]        = {0};        // 39B -> Total = 64B

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
        add_buf("angular_velocity_buffer_id");

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
        add_prop("evaluation_mode", godot::Variant::INT, 0, 2, "DOD Velocity Kinetic Energy,Jolt3D BODY_STATE_SLEEPING");
        add_prop("energy_threshold", godot::Variant::FLOAT, 0.05f);
        add_prop("default_mass", godot::Variant::FLOAT, 1.0f);
        add_prop("default_inertia", godot::Variant::FLOAT, 0.4f);
        add_prop("match_awake_bodies", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("angular_velocity_buffer_id")) angular_velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["angular_velocity_buffer_id"]));
        if (p_props.has("evaluation_mode")) evaluation_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["evaluation_mode"]), 0, 1);
        if (p_props.has("energy_threshold")) energy_threshold = std::max(0.0f, static_cast<float>(p_props["energy_threshold"]));
        if (p_props.has("default_mass")) default_mass = std::max(1e-4f, static_cast<float>(p_props["default_mass"]));
        if (p_props.has("default_inertia")) default_inertia = std::max(0.0f, static_cast<float>(p_props["default_inertia"]));
        if (p_props.has("match_awake_bodies")) match_awake_bodies = static_cast<bool>(p_props["match_awake_bodies"]);
    }

    template <QueryOp Op>
    static inline void _apply_match(MemoryBufferSelectionPOD& r_sel, const TaskContextPOD& p_ctx, uint32_t p_buf_id, int64_t idx, bool matched) noexcept {
        if (idx < 0 || idx >= r_sel.capacity) return;
        if constexpr (Op == QueryOp::CULL) {
            if (!matched && r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
                const uint64_t word = static_cast<uint64_t>(idx) >> 6;
                const uint64_t bit = 1ULL << (idx & 63);
                if (r_sel.data.bitset[word] & bit) {
                    r_sel.data.bitset[word] &= ~bit;
                    r_sel.element_count = std::max<int64_t>(0, r_sel.element_count - 1);
                }
            }
        } else if constexpr (Op == QueryOp::ADD) {
            if (matched) {
                p_ctx.queue_selection_command(p_buf_id, idx);
                if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
                    const uint64_t word = static_cast<uint64_t>(idx) >> 6;
                    const uint64_t bit = 1ULL << (idx & 63);
                    if (!(r_sel.data.bitset[word] & bit)) {
                        r_sel.data.bitset[word] |= bit;
                        r_sel.element_count++;
                    }
                }
            }
        }
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        const T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* primary = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!primary || !primary->raw_base_ptr) return;

            if (evaluation_mode == 1) {
                godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
                if (!ps) return;

                const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), primary->element_stride);
                const int64_t n_bodies = std::min<int64_t>(r_selection.capacity, static_cast<int64_t>(primary->capacity_bytes / r_stride));
                for (int64_t i = 0; i < n_bodies; ++i) {
                    const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(primary->raw_base_ptr + static_cast<size_t>(i) * r_stride);
                    if (raw_id == 0) continue;
                    godot::RID body_rid;
                    std::memcpy(&body_rid, &raw_id, sizeof(uint64_t));
                    if (!body_rid.is_valid()) continue;

                    const bool is_sleeping = static_cast<bool>(ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_SLEEPING));
                    const bool is_awake = !is_sleeping;
                    _apply_match<Op>(r_selection, p_ctx, target_buffer_id, i, is_awake == match_awake_bodies);
                }
            } else {
                const GrantPartPOD* ang_part = (angular_velocity_buffer_id != INVALID_ID)
                    ? p_ctx.get_grant_part(angular_velocity_buffer_id)
                    : ((p_ctx.grant && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr) ? &p_ctx.grant->parts[1] : nullptr);
                const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), primary->element_stride);
                const int64_t n_elems = std::min<int64_t>(r_selection.capacity, static_cast<int64_t>(primary->capacity_bytes / v_stride));

                const uint32_t w_stride = (ang_part && ang_part->raw_base_ptr)
                    ? std::max<uint32_t>(sizeof(godot::Vector3), ang_part->element_stride)
                    : sizeof(godot::Vector3);
                const int64_t max_w = (ang_part && ang_part->raw_base_ptr)
                    ? static_cast<int64_t>(ang_part->capacity_bytes / w_stride)
                    : 0;

                for (int64_t i = 0; i < n_elems; ++i) {
                    const godot::Vector3& v = *reinterpret_cast<const godot::Vector3*>(primary->raw_base_ptr + static_cast<size_t>(i) * v_stride);
                    float ke = 0.5f * default_mass * v.length_squared();
                    if (ang_part && ang_part->raw_base_ptr && i < max_w) {
                        const godot::Vector3& w = *reinterpret_cast<const godot::Vector3*>(ang_part->raw_base_ptr + static_cast<size_t>(i) * w_stride);
                        ke += 0.5f * default_inertia * w.length_squared();
                    }

                    const bool above = (ke >= energy_threshold);
                    _apply_match<Op>(r_selection, p_ctx, target_buffer_id, i, above == match_awake_bodies);
                }
            }
        }
    }
};

static_assert(sizeof(PhysicsKineticEnergyAndSleepQueryLogic<>) == 64, "PhysicsKineticEnergyAndSleepQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<PhysicsKineticEnergyAndSleepQueryLogic<>>, "PhysicsKineticEnergyAndSleepQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core