#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * RayTriangleIntersectQueryLogic<T, T_Strategy, T_View> (M38)
 * Möller-Trumbore ray/segment intersection query over 3D vertex and triangle buffers,
 * culling or adding hit triangles/vertices (and optional splash-radius neighbors around the closest hit).
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) RayTriangleIntersectQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Ray-Triangle Möller-Trumbore Query";
    static constexpr std::string_view ui_category  = "Model/Query";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::Vector3 ray_origin    = godot::Vector3(0.0f, 0.0f, 5.0f);  // 12B
    uint32_t target_buffer_id    = INVALID_ID;                        // 4B -> 16B: Vertex buffer
    godot::Vector3 ray_direction = godot::Vector3(0.0f, 0.0f, -1.0f); // 12B
    uint32_t index_buffer_id     = INVALID_ID;                        // 4B -> 32B: Optional triangle index buffer
    float max_distance           = 100.0f;                            // 4B
    float hit_splash_radius      = 0.0f;                              // 4B
    bool backface_cull           = true;                              // 1B
    uint8_t reserved_padding[23] = {0};                               // 23B -> Total = 64B

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
        add_buf("index_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("ray_origin", godot::Variant::VECTOR3, godot::Vector3(0.0f, 0.0f, 5.0f));
        add_prop("ray_direction", godot::Variant::VECTOR3, godot::Vector3(0.0f, 0.0f, -1.0f));
        add_prop("max_distance", godot::Variant::FLOAT, 100.0f);
        add_prop("hit_splash_radius", godot::Variant::FLOAT, 0.0f);
        add_prop("backface_cull", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("index_buffer_id")) index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        if (p_props.has("ray_origin")) ray_origin = p_props["ray_origin"];
        if (p_props.has("ray_direction")) {
            const godot::Vector3 d = p_props["ray_direction"];
            ray_direction = (d.length_squared() > 1e-8f) ? d.normalized() : godot::Vector3(0.0f, 0.0f, -1.0f);
        }
        if (p_props.has("max_distance")) max_distance = std::max(1e-4f, static_cast<float>(p_props["max_distance"]));
        if (p_props.has("hit_splash_radius")) hit_splash_radius = std::max(0.0f, static_cast<float>(p_props["hit_splash_radius"]));
        if (p_props.has("backface_cull")) backface_cull = static_cast<bool>(p_props["backface_cull"]);
    }

    bool moller_trumbore(
        const godot::Vector3& orig, const godot::Vector3& dir,
        const godot::Vector3& v0, const godot::Vector3& v1, const godot::Vector3& v2,
        float& r_t) const noexcept
    {
        constexpr float EPS = 1e-7f;
        const godot::Vector3 e1 = v1 - v0;
        const godot::Vector3 e2 = v2 - v0;
        const godot::Vector3 pvec = dir.cross(e2);
        const float det = e1.dot(pvec);

        if (backface_cull) {
            if (det < EPS) return false;
        } else {
            if (std::abs(det) < EPS) return false;
        }

        const float inv_det = 1.0f / det;
        const godot::Vector3 tvec = orig - v0;
        const float u = tvec.dot(pvec) * inv_det;
        if (u < 0.0f || u > 1.0f) return false;

        const godot::Vector3 qvec = tvec.cross(e1);
        const float v = dir.dot(qvec) * inv_det;
        if (v < 0.0f || u + v > 1.0f) return false;

        const float t = e2.dot(qvec) * inv_det;
        if (t < 0.0f || t > max_distance) return false;
        r_t = t;
        return true;
    }

    template <QueryOp Op>
    static inline void _mark_hit(MemoryBufferSelectionPOD& r_sel, const TaskContextPOD& p_ctx, uint32_t p_buf_id, int64_t idx) noexcept {
        if (idx < 0 || idx >= r_sel.capacity) return;
        if constexpr (Op == QueryOp::ADD) {
            p_ctx.queue_selection_command(p_buf_id, idx);
            if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
                const uint64_t word = static_cast<uint64_t>(idx) >> 6;
                const uint64_t bit = 1ULL << (idx & 63);
                if (!(r_sel.data.bitset[word] & bit)) {
                    r_sel.data.bitset[word] |= bit;
                    r_sel.element_count++;
                }
            }
        } else {
            if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
                const uint64_t word = static_cast<uint64_t>(idx) >> 6;
                const uint64_t bit = 1ULL << (idx & 63);
                if (r_sel.data.bitset[word] & bit) {
                    r_sel.data.bitset[word] &= ~bit;
                    r_sel.element_count = std::max<int64_t>(0, r_sel.element_count - 1);
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
            const GrantPartPOD* vtx_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!vtx_part || !vtx_part->raw_base_ptr) return;

            const GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(index_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr) ? &p_ctx.grant->parts[1] : nullptr);

            const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part->element_stride);
            const int32_t n_verts = static_cast<int32_t>(vtx_part->capacity_bytes / v_stride);
            if (n_verts <= 0) return;

            auto read_v = [&](int32_t idx) noexcept -> const godot::Vector3& {
                return *reinterpret_cast<const godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(idx) * v_stride);
            };

            const godot::Vector3 dir = (ray_direction.length_squared() > 1e-8f)
                ? ray_direction.normalized()
                : godot::Vector3(0.0f, 0.0f, -1.0f);

            float closest_t = max_distance + 1.0f;
            bool found_hit = false;

            if (idx_part && idx_part->raw_base_ptr) {
                const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
                const int32_t n_tris = static_cast<int32_t>(((idx_part->selection.element_count > 0)
                    ? idx_part->selection.element_count
                    : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t))) / 3);

                for (int32_t t = 0; t < n_tris; ++t) {
                    const int32_t i0 = indices[t * 3 + 0];
                    const int32_t i1 = indices[t * 3 + 1];
                    const int32_t i2 = indices[t * 3 + 2];
                    if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;

                    float hit_t = 0.0f;
                    if (moller_trumbore(ray_origin, dir, read_v(i0), read_v(i1), read_v(i2), hit_t)) {
                        _mark_hit<Op>(r_selection, p_ctx, target_buffer_id, i0);
                        _mark_hit<Op>(r_selection, p_ctx, target_buffer_id, i1);
                        _mark_hit<Op>(r_selection, p_ctx, target_buffer_id, i2);
                        if (hit_t < closest_t) {
                            closest_t = hit_t;
                            found_hit = true;
                        }
                    }
                }
            } else {
                for (int32_t i = 0; i + 2 < n_verts; i += 3) {
                    float hit_t = 0.0f;
                    if (moller_trumbore(ray_origin, dir, read_v(i), read_v(i + 1), read_v(i + 2), hit_t)) {
                        _mark_hit<Op>(r_selection, p_ctx, target_buffer_id, i + 0);
                        _mark_hit<Op>(r_selection, p_ctx, target_buffer_id, i + 1);
                        _mark_hit<Op>(r_selection, p_ctx, target_buffer_id, i + 2);
                        if (hit_t < closest_t) {
                            closest_t = hit_t;
                            found_hit = true;
                        }
                    }
                }
            }

            if (found_hit && hit_splash_radius > 0.0f) {
                const godot::Vector3 hit_pt = ray_origin + dir * closest_t;
                const float r2 = hit_splash_radius * hit_splash_radius;
                for (int32_t i = 0; i < n_verts; ++i) {
                    if ((read_v(i) - hit_pt).length_squared() <= r2) {
                        _mark_hit<Op>(r_selection, p_ctx, target_buffer_id, i);
                    }
                }
            }
        }
    }
};

static_assert(sizeof(RayTriangleIntersectQueryLogic<>) == 64, "RayTriangleIntersectQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<RayTriangleIntersectQueryLogic<>>, "RayTriangleIntersectQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core