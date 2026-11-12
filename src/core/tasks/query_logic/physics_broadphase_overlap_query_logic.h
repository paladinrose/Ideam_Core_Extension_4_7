#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

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
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * PhysicsBroadphaseOverlapQueryLogic<T, T_Strategy, T_View> (P6)
 * Queries Jolt 3D's broadphase/narrowphase (PhysicsDirectSpaceState3D::intersect_shape)
 * at a query transform or per-element probe position, culling or adding elements in r_selection
 * that overlap any collider matching collision_mask.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) PhysicsBroadphaseOverlapQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::UINT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Jolt3D Broadphase & Shape Overlap Query";
    static constexpr std::string_view ui_category  = "Physics/Queries";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::RID space_rid;                           // 8B
    godot::RID probe_shape_rid;                     // 8B -> 16B
    uint32_t target_buffer_id       = INVALID_ID;   // 4B: Probe position buffer
    float fallback_sphere_radius    = 1.0f;         // 4B
    uint32_t collision_mask         = 0xFFFFFFFFu;  // 4B
    int32_t max_results_per_probe   = 8;            // 4B -> 32B
    bool collide_with_bodies        = true;         // 1B
    bool collide_with_areas         = false;        // 1B
    uint8_t reserved_padding[30]    = {0};          // 30B -> Total = 64B

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
        add_prop("space_rid", godot::Variant::RID, godot::RID());
        add_prop("probe_shape_rid", godot::Variant::RID, godot::RID());
        add_prop("fallback_sphere_radius", godot::Variant::FLOAT, 1.0f);
        add_prop("collision_mask", godot::Variant::INT, static_cast<int64_t>(0xFFFFFFFFu));
        add_prop("max_results_per_probe", godot::Variant::INT, 8);
        add_prop("collide_with_bodies", godot::Variant::BOOL, true);
        add_prop("collide_with_areas", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        auto parse_rid = [](const godot::Variant& v) noexcept -> godot::RID {
            if (v.get_type() == godot::Variant::RID) return v;
            if (v.get_type() == godot::Variant::INT) {
                godot::RID r;
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&r, &raw, sizeof(uint64_t));
                return r;
            }
            return godot::RID();
        };
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("space_rid")) space_rid = parse_rid(p_props["space_rid"]);
        if (p_props.has("probe_shape_rid")) probe_shape_rid = parse_rid(p_props["probe_shape_rid"]);
        if (p_props.has("fallback_sphere_radius")) fallback_sphere_radius = std::max(1e-4f, static_cast<float>(p_props["fallback_sphere_radius"]));
        if (p_props.has("collision_mask")) collision_mask = static_cast<uint32_t>(static_cast<int64_t>(p_props["collision_mask"]));
        if (p_props.has("max_results_per_probe")) max_results_per_probe = std::clamp<int32_t>(static_cast<int32_t>(p_props["max_results_per_probe"]), 1, 64);
        if (p_props.has("collide_with_bodies")) collide_with_bodies = static_cast<bool>(p_props["collide_with_bodies"]);
        if (p_props.has("collide_with_areas")) collide_with_areas = static_cast<bool>(p_props["collide_with_areas"]);
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        const T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!space_rid.is_valid()) return;

            const GrantPartPOD* pos_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!pos_part || !pos_part->raw_base_ptr) return;

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

            const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part->element_stride);
            const int64_t n_probes = std::min<int64_t>(
                r_selection.capacity,
                static_cast<int64_t>(pos_part->capacity_bytes / stride)
            );

            godot::Transform3D xform;
            for (int64_t i = 0; i < n_probes; ++i) {
                xform.origin = *reinterpret_cast<const godot::Vector3*>(pos_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                params->set_transform(xform);

                const godot::TypedArray<godot::Dictionary> hits = space_state->intersect_shape(params, max_results_per_probe);
                const bool overlapped = !hits.is_empty();

                if constexpr (Op == QueryOp::CULL) {
                    if (!overlapped && r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                        const uint64_t word = static_cast<uint64_t>(i) >> 6;
                        const uint64_t bit = 1ULL << (i & 63);
                        if (r_selection.data.bitset[word] & bit) {
                            r_selection.data.bitset[word] &= ~bit;
                            r_selection.element_count = std::max<int64_t>(0, r_selection.element_count - 1);
                        }
                    }
                } else if constexpr (Op == QueryOp::ADD) {
                    if (overlapped) {
                        p_ctx.queue_selection_command(target_buffer_id, i);
                        if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                            const uint64_t word = static_cast<uint64_t>(i) >> 6;
                            const uint64_t bit = 1ULL << (i & 63);
                            if (!(r_selection.data.bitset[word] & bit)) {
                                r_selection.data.bitset[word] |= bit;
                                r_selection.element_count++;
                            }
                        }
                    }
                }
            }

            if (owns_shape && active_shape.is_valid()) {
                ps->free_rid(active_shape);
            }
        }
    }
};

static_assert(sizeof(PhysicsBroadphaseOverlapQueryLogic<>) == 64, "PhysicsBroadphaseOverlapQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<PhysicsBroadphaseOverlapQueryLogic<>>, "PhysicsBroadphaseOverlapQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core