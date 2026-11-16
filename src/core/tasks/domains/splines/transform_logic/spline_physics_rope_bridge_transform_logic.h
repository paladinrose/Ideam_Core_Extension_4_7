#pragma once

/**
 * spline_physics_rope_bridge_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`SPL12`) for `IdeamSplines` / `@Physics`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.2 — `SPL12` `PhysicsRope.cs` Port)
 * ============================================================================
 * Synchronizes a 3D Catmull-Rom spline (`SplineHeaderPOD` + `SplineSegmentCoeffsPOD<4, float>`)
 * with a chain of Jolt/Godot 3D rigid-body hinge solvers (`number_of_physics_solvers *
 * number_of_segments_per_solver`):
 *   - `ROPE_SYNC_PHYSICS_TO_SPLINE (0)`: Reads rigid-body solver transforms (from Jolt3D
 *     `body_rid_buffer_id` or DOD `solver_position_buffer_id`), subdivides each solver span
 *     into `segments_per_solver` Catmull-Rom cubic Horner segments (`c0 + u*(c1 + u*(c2 + u*c3))`),
 *     and updates the spline segment coefficient buffer and arc-length bounds in place.
 *   - `ROPE_SYNC_SPLINE_TO_PHYSICS (1)`: Evaluates the Catmull-Rom spline at uniform parameter
 *     steps across `number_of_physics_solvers * segments_per_solver` and writes rest/target
 *     positions back to `solver_position_buffer_id` (and `PhysicsServer3D` when `body_rid_buffer_id`
 *     is bound).
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../../splines/spline_math.h"
#include "../../../../characters/character_common.h"

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

namespace ideam::core {

enum class SplineRopeSyncMode : uint8_t {
    ROPE_SYNC_PHYSICS_TO_SPLINE = 0,
    ROPE_SYNC_SPLINE_TO_PHYSICS = 1
};

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SplinePhysicsRopeBridgeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32 | DataType::VECTOR3 | DataType::VECTOR4;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline 3D Catmull-Rom Physics Rope Bridge";
    static constexpr std::string_view ui_category  = "Splines/Physics";

    uint32_t target_buffer_id              = INVALID_ID; ///< `SPLINE_SEGMENT_COEFFS_4D_POD` or `solver_position_buffer_id`
    uint32_t spline_header_buffer_id       = INVALID_ID; ///< Optional `SPLINE_HEADER_POD` (`SplineHeaderPOD`)
    uint32_t solver_position_buffer_id     = INVALID_ID; ///< `glm::vec3` / `godot::Vector3` chain node positions
    uint32_t body_rid_buffer_id            = INVALID_ID; ///< Optional `uint64_t` Jolt3D `RID` handles per solver body
    uint32_t spline_slot_index             = 0u;         ///< Target spline header slot
    uint16_t number_of_physics_solvers     = 8u;         ///< `numberOfPhysicsSolvers` from `PhysicsRope.cs`
    uint16_t number_of_segments_per_solver = 2u;         ///< `numberOfSegmentsPerSolver` from `PhysicsRope.cs`
    float    catmull_rom_tension           = 0.5f;       ///< Centripetal/uniform Catmull-Rom tangent scale (`0.5` = standard)
    float    rope_segment_length           = 0.25f;      ///< Rest distance between adjacent solver bodies (m)
    SplineRopeSyncMode sync_mode           = SplineRopeSyncMode::ROPE_SYNC_PHYSICS_TO_SPLINE;
    uint8_t  reserved_padding[31]          = {0};        ///< 31B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",              godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("spline_header_buffer_id",       godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("solver_position_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("body_rid_buffer_id",            godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("spline_slot_index",             godot::Variant::INT,   static_cast<int64_t>(0));
        add_prop("number_of_physics_solvers",     godot::Variant::INT,   static_cast<int64_t>(8));
        add_prop("number_of_segments_per_solver", godot::Variant::INT,   static_cast<int64_t>(2));
        add_prop("catmull_rom_tension",           godot::Variant::FLOAT, 0.5);
        add_prop("rope_segment_length",           godot::Variant::FLOAT, 0.25);
        add_prop("sync_mode",                     godot::Variant::INT,   static_cast<int64_t>(0));
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : solver_position_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))              target_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("spline_header_buffer_id"))       spline_header_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["spline_header_buffer_id"]));
        if (p_props.has("solver_position_buffer_id"))     solver_position_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["solver_position_buffer_id"]));
        if (p_props.has("body_rid_buffer_id"))            body_rid_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["body_rid_buffer_id"]));
        if (p_props.has("spline_slot_index"))             spline_slot_index             = static_cast<uint32_t>( std::max<int64_t>(0, static_cast<int64_t>(p_props["spline_slot_index"])));
        if (p_props.has("number_of_physics_solvers"))     number_of_physics_solvers     = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["number_of_physics_solvers"]), 2, 512));
        if (p_props.has("number_of_segments_per_solver")) number_of_segments_per_solver = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["number_of_segments_per_solver"]), 1, 16));
        if (p_props.has("catmull_rom_tension"))           catmull_rom_tension           = static_cast<float>(static_cast<double>(p_props["catmull_rom_tension"]));
        if (p_props.has("rope_segment_length"))           rope_segment_length           = std::max(1e-4f, static_cast<float>(static_cast<double>(p_props["rope_segment_length"])));
        if (p_props.has("sync_mode"))                     sync_mode                     = static_cast<SplineRopeSyncMode>(std::clamp<int64_t>(static_cast<int64_t>(p_props["sync_mode"]), 0, 1));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_Elem>
    [[nodiscard]] static T_Elem *_resolve_ptr(const TaskContextPOD &p_ctx, uint32_t p_buf_id, size_t &r_cap) noexcept {
        r_cap = 0u;
        if (p_buf_id == INVALID_ID) return nullptr;
        if (const GrantPartPOD *part = p_ctx.get_grant_part(p_buf_id)) {
            if (part->raw_base_ptr) {
                const size_t cap = part->capacity_bytes / sizeof(T_Elem);
                r_cap = (part->selection.element_count > 0)
                    ? std::min<size_t>(static_cast<size_t>(part->selection.element_count), cap)
                    : cap;
                return reinterpret_cast<T_Elem *>(part->raw_base_ptr);
            }
        }
        if (p_ctx.manager) {
            if (MemoryBufferPOD *buf = p_ctx.manager->get_buffer(p_buf_id)) {
                if (buf->master_block_ptr) {
                    uint8_t *raw = buf->master_block_ptr + buf->memory_offset;
                    r_cap = static_cast<size_t>(std::max<int64_t>(0, buf->current_count > 0 ? buf->current_count : buf->max_elements));
                    return reinterpret_cast<T_Elem *>(raw);
                }
            }
        }
        return nullptr;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t seg_cap = 0, hdr_cap = 0, pos_cap = 0, rid_cap = 0;
        auto *coeffs      = _resolve_ptr<SplineSegmentCoeffsPOD<4, float>>(p_ctx, target_buffer_id, seg_cap);
        auto *headers     = _resolve_ptr<SplineHeaderPOD>(p_ctx, spline_header_buffer_id, hdr_cap);
        auto *solver_pos  = _resolve_ptr<glm::vec3>(p_ctx, solver_position_buffer_id, pos_cap);
        const auto *rids  = _resolve_ptr<uint64_t>(p_ctx, body_rid_buffer_id, rid_cap);

        if (!solver_pos || pos_cap < 2u) return;

        const size_t n_solvers = std::min<size_t>(pos_cap, std::max<size_t>(2u, number_of_physics_solvers));
        godot::PhysicsServer3D *ps = godot::PhysicsServer3D::get_singleton();

        uint32_t seg_offset = 0u;
        uint32_t seg_limit  = static_cast<uint32_t>(seg_cap);
        if (headers && spline_slot_index < hdr_cap) {
            seg_offset = headers[spline_slot_index].segment_offset;
            seg_limit  = std::min<uint32_t>(
                static_cast<uint32_t>(seg_cap),
                seg_offset + headers[spline_slot_index].segment_count
            );
        }

        if (sync_mode == SplineRopeSyncMode::ROPE_SYNC_PHYSICS_TO_SPLINE) {
            // 1. Pull live positions from Jolt3D rigid bodies if RID buffer is bound
            if (rids && ps) {
                const size_t n_rids = std::min<size_t>(n_solvers, rid_cap);
                for (size_t i = 0; i < n_rids; ++i) {
                    const uint64_t raw_id = rids[i];
                    if (raw_id == 0ULL) continue;
                    godot::RID body_rid;
                    std::memcpy(&body_rid, &raw_id, sizeof(uint64_t));
                    if (body_rid.is_valid()) {
                        const godot::Transform3D xf = ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM);
                        solver_pos[i] = glm::vec3(xf.origin.x, xf.origin.y, xf.origin.z);
                    }
                }
            }

            // 2. Enforce rope distance constraint along chain (`rope_segment_length`) and write Catmull-Rom segments
            for (size_t i = 1; i < n_solvers; ++i) {
                const glm::vec3 diff = solver_pos[i] - solver_pos[i - 1u];
                const float len = glm::length(diff);
                if (len > rope_segment_length * 1.25f && len > 1e-6f) {
                    solver_pos[i] = solver_pos[i - 1u] + (diff / len) * rope_segment_length;
                }
            }

            if (coeffs && seg_offset < seg_limit) {
                uint32_t out_seg_idx = seg_offset;
                float total_len      = 0.0f;
                const uint32_t sub_segs = std::max<uint32_t>(1u, number_of_segments_per_solver);

                for (size_t i = 0; i + 1u < n_solvers && out_seg_idx < seg_limit; ++i) {
                    const glm::vec3 &p0 = (i > 0u) ? solver_pos[i - 1u] : solver_pos[0];
                    const glm::vec3 &p1 = solver_pos[i];
                    const glm::vec3 &p2 = solver_pos[i + 1u];
                    const glm::vec3 &p3 = (i + 2u < n_solvers) ? solver_pos[i + 2u] : solver_pos[n_solvers - 1u];

                    const glm::vec3 m1 = (p2 - p0) * catmull_rom_tension;
                    const glm::vec3 m2 = (p3 - p1) * catmull_rom_tension;

                    // Hermite / Catmull-Rom cubic polynomial over u in [0, 1]
                    const glm::vec3 c0 = p1;
                    const glm::vec3 c1 = m1;
                    const glm::vec3 c2 = 3.0f * (p2 - p1) - 2.0f * m1 - m2;
                    const glm::vec3 c3 = 2.0f * (p1 - p2) + m1 + m2;

                    for (uint32_t s = 0; s < sub_segs && out_seg_idx < seg_limit; ++s) {
                        const float u0 = static_cast<float>(s) / static_cast<float>(sub_segs);
                        const float du = 1.0f / static_cast<float>(sub_segs);

                        // Reparameterize sub-segment onto local tau in [0, 1]
                        const glm::vec3 sub_p0 = c0 + u0 * (c1 + u0 * (c2 + u0 * c3));
                        const glm::vec3 sub_d1 = (c1 + u0 * (2.0f * c2 + 3.0f * u0 * c3)) * du;
                        const glm::vec3 sub_d2 = (c2 + 3.0f * u0 * c3) * (du * du);
                        const glm::vec3 sub_d3 = c3 * (du * du * du);

                        auto &seg = coeffs[out_seg_idx++];
                        seg.c0 = glm::vec4(sub_p0, 0.0f);
                        seg.c1 = glm::vec4(sub_d1, 0.0f);
                        seg.c2 = glm::vec4(sub_d2, 0.0f);
                        seg.c3 = glm::vec4(sub_d3, 0.0f);
                        total_len += glm::length(p2 - p1) * du;
                    }
                }

                if (headers && spline_slot_index < hdr_cap) {
                    headers[spline_slot_index].segment_count = out_seg_idx - seg_offset;
                    headers[spline_slot_index].total_length  = total_len;
                }
            }
        } else {
            // ROPE_SYNC_SPLINE_TO_PHYSICS: Sample spline segments to position solver bodies
            if (!coeffs || seg_offset >= seg_limit) return;
            const uint32_t active_segs = seg_limit - seg_offset;

            for (size_t i = 0; i < n_solvers; ++i) {
                const float global_u = (n_solvers > 1u)
                    ? (static_cast<float>(i) / static_cast<float>(n_solvers - 1u))
                    : 0.0f;
                const float scaled   = global_u * static_cast<float>(active_segs);
                const uint32_t s_idx = std::min<uint32_t>(static_cast<uint32_t>(scaled), active_segs - 1u);
                const float local_u  = std::clamp(scaled - static_cast<float>(s_idx), 0.0f, 1.0f);

                const auto &seg = coeffs[seg_offset + s_idx];
                const glm::vec4 pos4 = seg.evaluate(local_u);
                solver_pos[i] = glm::vec3(pos4.x, pos4.y, pos4.z);

                if (rids && ps && i < rid_cap && rids[i] != 0ULL) {
                    godot::RID body_rid;
                    std::memcpy(&body_rid, &rids[i], sizeof(uint64_t));
                    if (body_rid.is_valid()) {
                        godot::Transform3D xf = ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM);
                        xf.origin = godot::Vector3(solver_pos[i].x, solver_pos[i].y, solver_pos[i].z);
                        ps->body_set_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM, xf);
                    }
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(SplinePhysicsRopeBridgeTransformLogic<>) == 64, "SplinePhysicsRopeBridgeTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<SplinePhysicsRopeBridgeTransformLogic<>>, "SplinePhysicsRopeBridgeTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
