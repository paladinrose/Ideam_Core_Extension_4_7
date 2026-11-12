#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class PhysicsForceAppMode : uint8_t {
    CENTRAL_IMPULSE    = 0,
    CONTINUOUS_FORCE   = 1,
    TORQUE_IMPULSE     = 2,
    POSITIONED_IMPULSE = 3,
    RADIAL_BLAST_WAVE  = 4
};

/**
 * PhysicsImpulseAndForceBatchTransformLogic<T> (P8)
 * Applies batched central impulses, positioned impulses, torque impulses, continuous forces,
 * or radial explosion shockwaves directly to Jolt 3D body RIDs via PhysicsServer3D.
 */
template <typename T = godot::Vector3>
struct alignas(64) PhysicsImpulseAndForceBatchTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::INT64;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Jolt3D Impulse, Force & Blast Wave Batch";
    static constexpr std::string_view ui_category  = "Physics/Bodies";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t body_rid_buffer_id = INVALID_ID; // 4B: Jolt 3D body RIDs buffer (uint64_t handles)
    uint32_t vector_buffer_id   = INVALID_ID; // 4B: Primary target impulse/force/torque Vector3 buffer
    uint32_t offset_buffer_id   = INVALID_ID; // 4B: Optional local offset Vector3 buffer for POSITIONED_IMPULSE

    godot::Vector3 blast_origin{0.0f, 0.0f, 0.0f}; // 12B: World-space explosion origin -> 24B

    float blast_radius                    = 10.0f; // 4B: Radial blast radius (meters)
    float blast_impulse_magnitude         = 25.0f; // 4B: Peak blast impulse (N*s)
    float blast_upward_bias               = 0.25f; // 4B: Vertical lift bias for radial blast -> 36B
    PhysicsForceAppMode application_mode  = PhysicsForceAppMode::CENTRAL_IMPULSE; // 1B
    uint8_t reserved_padding[27]          = {0};   // 27B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary rbuf_prop;
        rbuf_prop["name"] = "body_rid_buffer_id";
        rbuf_prop["type"] = godot::Variant::INT;
        rbuf_prop["hint_string"] = "buffer_option";
        props.push_back(rbuf_prop);

        godot::Dictionary vbuf_prop;
        vbuf_prop["name"] = "vector_buffer_id";
        vbuf_prop["type"] = godot::Variant::INT;
        vbuf_prop["hint_string"] = "buffer_option";
        props.push_back(vbuf_prop);

        godot::Dictionary obuf_prop;
        obuf_prop["name"] = "offset_buffer_id";
        obuf_prop["type"] = godot::Variant::INT;
        obuf_prop["hint_string"] = "buffer_option";
        props.push_back(obuf_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "application_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Central Impulse:0,Continuous Force:1,Torque Impulse:2,Positioned Impulse:3,Radial Blast Wave:4";
        props.push_back(mode_prop);

        godot::Dictionary orig_prop;
        orig_prop["name"] = "blast_origin";
        orig_prop["type"] = godot::Variant::VECTOR3;
        orig_prop["hint_string"] = "suffix:m";
        props.push_back(orig_prop);

        godot::Dictionary rad_prop;
        rad_prop["name"] = "blast_radius";
        rad_prop["type"] = godot::Variant::FLOAT;
        rad_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rad_prop["hint_string"] = "0.1,200.0,0.1,suffix:m";
        props.push_back(rad_prop);

        godot::Dictionary mag_prop;
        mag_prop["name"] = "blast_impulse_magnitude";
        mag_prop["type"] = godot::Variant::FLOAT;
        props.push_back(mag_prop);

        godot::Dictionary up_prop;
        up_prop["name"] = "blast_upward_bias";
        up_prop["type"] = godot::Variant::FLOAT;
        props.push_back(up_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (vector_buffer_id != INVALID_ID) ? vector_buffer_id : body_rid_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("body_rid_buffer_id")) {
            body_rid_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["body_rid_buffer_id"]));
        }
        if (p_props.has("vector_buffer_id")) {
            vector_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vector_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && vector_buffer_id == INVALID_ID) {
            vector_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("offset_buffer_id")) {
            offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["offset_buffer_id"]));
        }
        if (p_props.has("application_mode")) {
            application_mode = static_cast<PhysicsForceAppMode>(std::clamp<int64_t>(static_cast<int64_t>(p_props["application_mode"]), 0, 4));
        }
        if (p_props.has("blast_origin")) {
            blast_origin = static_cast<godot::Vector3>(p_props["blast_origin"]);
        }
        if (p_props.has("blast_radius")) {
            blast_radius = std::max(1e-4f, static_cast<float>(p_props["blast_radius"]));
        }
        if (p_props.has("blast_impulse_magnitude")) {
            blast_impulse_magnitude = static_cast<float>(p_props["blast_impulse_magnitude"]);
        }
        if (p_props.has("blast_upward_bias")) {
            blast_upward_bias = static_cast<float>(p_props["blast_upward_bias"]);
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
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, int64_t>) {
            const GrantPartPOD* rid_part = p_context.get_grant_part(body_rid_buffer_id);
            if (!rid_part || !rid_part->raw_base_ptr) return;

            const GrantPartPOD* vec_part    = (vector_buffer_id != INVALID_ID) ? p_context.get_grant_part(vector_buffer_id) : nullptr;
            const GrantPartPOD* offset_part = (offset_buffer_id != INVALID_ID) ? p_context.get_grant_part(offset_buffer_id) : nullptr;

            godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
            if (!ps) return;

            const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part->element_stride);
            const int64_t n_bodies = (rid_part->selection.element_count > 0)
                ? rid_part->selection.element_count
                : static_cast<int64_t>(rid_part->capacity_bytes / r_stride);

            const uint32_t v_stride = (vec_part && vec_part->raw_base_ptr) ? std::max<uint32_t>(sizeof(godot::Vector3), vec_part->element_stride) : sizeof(godot::Vector3);
            const int64_t n_vecs    = (vec_part && vec_part->raw_base_ptr) ? static_cast<int64_t>(vec_part->capacity_bytes / v_stride) : 0;

            const uint32_t o_stride = (offset_part && offset_part->raw_base_ptr) ? std::max<uint32_t>(sizeof(godot::Vector3), offset_part->element_stride) : sizeof(godot::Vector3);
            const int64_t n_offsets = (offset_part && offset_part->raw_base_ptr) ? static_cast<int64_t>(offset_part->capacity_bytes / o_stride) : 0;

            for (int64_t i = 0; i < n_bodies; ++i) {
                const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(rid_part->raw_base_ptr + static_cast<size_t>(i) * r_stride);
                if (raw_id == 0) continue;
                godot::RID body_rid;
                std::memcpy(&body_rid, &raw_id, sizeof(uint64_t));
                if (!body_rid.is_valid()) continue;

                if (application_mode == PhysicsForceAppMode::RADIAL_BLAST_WAVE) {
                    const godot::Transform3D xform = ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM);
                    const godot::Vector3 rel = xform.origin - blast_origin;
                    const float dist = rel.length();
                    if (dist >= blast_radius) continue;

                    const float u = 1.0f - dist / blast_radius;
                    const float falloff = u * u;
                    godot::Vector3 dir = (dist > 1e-5f) ? (rel / dist) : godot::Vector3(0.0f, 1.0f, 0.0f);
                    dir = (dir + godot::Vector3(0.0f, blast_upward_bias, 0.0f)).normalized();
                    ps->body_apply_central_impulse(body_rid, dir * (blast_impulse_magnitude * falloff));
                    continue;
                }

                if (!vec_part || !vec_part->raw_base_ptr || i >= n_vecs) continue;
                const godot::Vector3& vec = *reinterpret_cast<const godot::Vector3*>(vec_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);

                if (application_mode == PhysicsForceAppMode::CENTRAL_IMPULSE) {
                    ps->body_apply_central_impulse(body_rid, vec);
                } else if (application_mode == PhysicsForceAppMode::CONTINUOUS_FORCE) {
                    ps->body_add_central_force(body_rid, vec);
                } else if (application_mode == PhysicsForceAppMode::TORQUE_IMPULSE) {
                    ps->body_apply_torque_impulse(body_rid, vec);
                } else if (application_mode == PhysicsForceAppMode::POSITIONED_IMPULSE) {
                    const godot::Vector3 pos_offset = (offset_part && offset_part->raw_base_ptr && i < n_offsets)
                        ? *reinterpret_cast<const godot::Vector3*>(offset_part->raw_base_ptr + static_cast<size_t>(i) * o_stride)
                        : godot::Vector3(0.0f, 0.0f, 0.0f);
                    ps->body_apply_impulse(body_rid, vec, pos_offset);
                }
            }
        }
    }
};

static_assert(sizeof(PhysicsImpulseAndForceBatchTransformLogic<>) == 64, "PhysicsImpulseAndForceBatchTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<PhysicsImpulseAndForceBatchTransformLogic<>>, "PhysicsImpulseAndForceBatchTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core