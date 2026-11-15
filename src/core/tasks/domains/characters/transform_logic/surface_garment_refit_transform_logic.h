#pragma once

/**
 * surface_garment_refit_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`CHR3`) for `IdeamCharacters`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.2 — `CHR3`)
 * ============================================================================
 * Batch-evaluates `ParametricCharacterMesh::evaluate_garment_surface_point()` over
 * `(quad_id, u, v, d_normal)` curvilinear coordinates (`SurfaceGarmentBindPod`),
 * mapping garment vertices onto the deformed character body quads and optionally
 * running XPBD distance & rest-shape relaxation iterations (`XPBDVertexSoftBodyTransformLogic`
 * bridge) so clothing drapes smoothly across any body morph state.
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../../characters/parametric_character_mesh.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SurfaceGarmentRefitTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32 | DataType::VECTOR3;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Character Curvilinear Surface Garment Refit & XPBD Bridge";
    static constexpr std::string_view ui_category  = "Characters";

    uint32_t target_buffer_id            = INVALID_ID; ///< `GARMENT_POSITION_VEC3` (`glm::vec3`)
    uint32_t garment_bind_buffer_id      = INVALID_ID; ///< `GARMENT_BIND_POD` (`SurfaceGarmentBindPod`)
    uint32_t quad_face_buffer_id         = INVALID_ID; ///< `QUAD_FACE_POD` (`QuadFacePod`)
    uint32_t deformed_position_buffer_id = INVALID_ID; ///< `DEFORMED_POSITION_VEC3` (`glm::vec3`)
    uint32_t deformed_normal_buffer_id   = INVALID_ID; ///< `DEFORMED_NORMAL_VEC3` (`glm::vec3`)
    uint32_t garment_velocity_buffer_id  = INVALID_ID; ///< Optional `GARMENT_VELOCITY_VEC3` (`glm::vec3`)
    float    extra_normal_offset         = 0.0f;       ///< Additional normal clearance offset (m)
    uint32_t xpbd_relax_iterations       = 3u;         ///< Post-refit XPBD Laplacian/edge smoothing iterations
    float    xpbd_compliance             = 0.0005f;    ///< Inverse stiffness compliance alpha
    uint8_t  reserved_padding[28]        = {0};        ///< 28B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",            godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("garment_bind_buffer_id",      godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("quad_face_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("deformed_position_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("deformed_normal_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("garment_velocity_buffer_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("extra_normal_offset",         godot::Variant::FLOAT, 0.0);
        add_prop("xpbd_relax_iterations",       godot::Variant::INT,   static_cast<int64_t>(3));
        add_prop("xpbd_compliance",             godot::Variant::FLOAT, 0.0005);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : garment_bind_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))            target_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("garment_bind_buffer_id"))      garment_bind_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["garment_bind_buffer_id"]));
        if (p_props.has("quad_face_buffer_id"))         quad_face_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["quad_face_buffer_id"]));
        if (p_props.has("deformed_position_buffer_id")) deformed_position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["deformed_position_buffer_id"]));
        if (p_props.has("deformed_normal_buffer_id"))   deformed_normal_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["deformed_normal_buffer_id"]));
        if (p_props.has("garment_velocity_buffer_id"))  garment_velocity_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["garment_velocity_buffer_id"]));
        if (p_props.has("extra_normal_offset"))         extra_normal_offset         = static_cast<float>(static_cast<double>(p_props["extra_normal_offset"]));
        if (p_props.has("xpbd_relax_iterations"))       xpbd_relax_iterations       = static_cast<uint32_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["xpbd_relax_iterations"]), 0, 16));
        if (p_props.has("xpbd_compliance"))             xpbd_compliance             = std::max(0.0f, static_cast<float>(static_cast<double>(p_props["xpbd_compliance"])));
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
        size_t out_cap = 0, bind_cap = 0, q_cap = 0, pos_cap = 0, nrm_cap = 0, vel_cap = 0;
        auto *garment_pos     = _resolve_ptr<glm::vec3>(p_ctx, target_buffer_id, out_cap);
        const auto *binds     = _resolve_ptr<SurfaceGarmentBindPod>(p_ctx, garment_bind_buffer_id, bind_cap);
        const auto *quads     = _resolve_ptr<QuadFacePod>(p_ctx, quad_face_buffer_id, q_cap);
        const auto *body_pos  = _resolve_ptr<glm::vec3>(p_ctx, deformed_position_buffer_id, pos_cap);
        const auto *body_nrm  = _resolve_ptr<glm::vec3>(p_ctx, deformed_normal_buffer_id, nrm_cap);
        auto *garment_vel     = _resolve_ptr<glm::vec3>(p_ctx, garment_velocity_buffer_id, vel_cap);

        if (!garment_pos || out_cap == 0u || !binds || bind_cap == 0u || !quads || q_cap == 0u || !body_pos || pos_cap == 0u) {
            return;
        }

        const size_t n_garment = std::min<size_t>(out_cap, bind_cap);
        ParametricCharacterMesh::evaluate_garment_surface_batch(
            std::span<const SurfaceGarmentBindPod>(binds, n_garment),
            std::span<const QuadFacePod>(quads, q_cap),
            std::span<const glm::vec3>(body_pos, pos_cap),
            body_nrm ? std::span<const glm::vec3>(body_nrm, nrm_cap) : std::span<const glm::vec3>{},
            extra_normal_offset,
            garment_pos,
            n_garment
        );

        // Optional XPBD cloth edge & surface-normal clearance relaxation pass
        if (xpbd_relax_iterations > 0u && n_garment >= 2u) {
            const float dt = (p_ctx.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_ctx.delta)) : (1.0f / 60.0f);
            const float alpha_tilde = xpbd_compliance / (dt * dt);
            const float blend = 1.0f / (2.0f + alpha_tilde);

            for (uint32_t iter = 0; iter < xpbd_relax_iterations; ++iter) {
                for (size_t i = 0; i + 1u < n_garment; ++i) {
                    // Relax adjacent garment vertices sharing the same quad or neighboring ring quads
                    if (binds[i].quad_id != binds[i + 1u].quad_id) continue;
                    const glm::vec3 diff = garment_pos[i + 1u] - garment_pos[i];
                    const float dist = glm::length(diff);
                    if (dist < 1e-6f) continue;

                    const float du = binds[i + 1u].u - binds[i].u;
                    const float dv = binds[i + 1u].v - binds[i].v;
                    const float target_param_dist = std::sqrt(du * du + dv * dv) * 0.22f;
                    if (target_param_dist <= 1e-5f) continue;

                    const float C = dist - target_param_dist;
                    const glm::vec3 corr = (diff / dist) * (C * 0.25f * blend);
                    garment_pos[i]      += corr;
                    garment_pos[i + 1u] -= corr;
                }
            }

            if (garment_vel && vel_cap >= n_garment) {
                for (size_t i = 0; i < n_garment; ++i) {
                    garment_vel[i] *= 0.90f;
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(SurfaceGarmentRefitTransformLogic<>) == 64, "SurfaceGarmentRefitTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<SurfaceGarmentRefitTransformLogic<>>, "SurfaceGarmentRefitTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
