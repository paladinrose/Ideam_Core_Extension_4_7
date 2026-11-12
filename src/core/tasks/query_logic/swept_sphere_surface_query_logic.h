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
 * SweptSphereSurfaceQueryLogic<T, T_Strategy, T_View> (M39)
 * Evaluates a swept sphere (capsule volume) from sweep_start to sweep_end with sphere_radius
 * and optional front-face normal dot-product filtering, culling/adding intersected surface vertices.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SweptSphereSurfaceQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Swept Sphere / Capsule Surface Query";
    static constexpr std::string_view ui_category  = "Model/Query";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::Vector3 sweep_start   = godot::Vector3(0.0f, 1.0f, 0.0f);  // 12B
    uint32_t target_buffer_id    = INVALID_ID;                        // 4B -> 16B: Vertex buffer
    godot::Vector3 sweep_end     = godot::Vector3(0.0f, -1.0f, 0.0f); // 12B
    uint32_t normal_buffer_id    = INVALID_ID;                        // 4B -> 32B: Optional normal buffer
    float sphere_radius          = 1.0f;                              // 4B
    float min_normal_dot         = -1.0f;                             // 4B
    bool require_front_facing    = false;                             // 1B
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
        add_buf("normal_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("sweep_start", godot::Variant::VECTOR3, godot::Vector3(0.0f, 1.0f, 0.0f));
        add_prop("sweep_end", godot::Variant::VECTOR3, godot::Vector3(0.0f, -1.0f, 0.0f));
        add_prop("sphere_radius", godot::Variant::FLOAT, 1.0f);
        add_prop("min_normal_dot", godot::Variant::FLOAT, -1.0f);
        add_prop("require_front_facing", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("normal_buffer_id")) normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        if (p_props.has("sweep_start")) sweep_start = p_props["sweep_start"];
        if (p_props.has("sweep_end")) sweep_end = p_props["sweep_end"];
        if (p_props.has("sphere_radius")) sphere_radius = std::max(1e-4f, static_cast<float>(p_props["sphere_radius"]));
        if (p_props.has("min_normal_dot")) min_normal_dot = std::clamp(static_cast<float>(p_props["min_normal_dot"]), -1.0f, 1.0f);
        if (p_props.has("require_front_facing")) require_front_facing = static_cast<bool>(p_props["require_front_facing"]);
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

            const GrantPartPOD* nrm_part = (normal_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(normal_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr) ? &p_ctx.grant->parts[1] : nullptr);

            const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part->element_stride);
            const int64_t max_verts = static_cast<int64_t>(vtx_part->capacity_bytes / v_stride);

            const godot::Vector3 seg = sweep_end - sweep_start;
            const float seg_len2 = seg.length_squared();
            const godot::Vector3 sweep_dir = (seg_len2 > 1e-10f)
                ? (seg / std::sqrt(seg_len2))
                : godot::Vector3(0.0f, -1.0f, 0.0f);
            const float r2 = sphere_radius * sphere_radius;

            auto eval_vertex = [&](int64_t i) noexcept -> bool {
                if (i < 0 || i >= max_verts) return false;
                const godot::Vector3& p = *reinterpret_cast<const godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);
                const float t = (seg_len2 > 1e-10f) ? std::clamp((p - sweep_start).dot(seg) / seg_len2, 0.0f, 1.0f) : 0.0f;
                const godot::Vector3 closest = sweep_start + seg * t;
                if ((p - closest).length_squared() > r2) return false;

                if (require_front_facing && nrm_part && nrm_part->raw_base_ptr) {
                    const uint32_t n_stride = std::max<uint32_t>(sizeof(godot::Vector3), nrm_part->element_stride);
                    if (static_cast<size_t>(i + 1) * n_stride <= nrm_part->capacity_bytes) {
                        const godot::Vector3& nrm = *reinterpret_cast<const godot::Vector3*>(nrm_part->raw_base_ptr + static_cast<size_t>(i) * n_stride);
                        if (-nrm.dot(sweep_dir) < min_normal_dot) return false;
                    }
                }
                return true;
            };

            if constexpr (Op == QueryOp::CULL) {
                if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                    for (int64_t i = 0; i < r_selection.capacity; ++i) {
                        if (r_selection.data.bitset[i >> 6] & (1ULL << (i & 63))) {
                            if (!eval_vertex(i)) {
                                r_selection.data.bitset[i >> 6] &= ~(1ULL << (i & 63));
                                r_selection.element_count = std::max<int64_t>(0, r_selection.element_count - 1);
                            }
                        }
                    }
                } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                    int64_t write_ptr = 0;
                    for (int64_t k = 0; k < r_selection.element_count; ++k) {
                        const int64_t idx = r_selection.data.indices[k];
                        if (eval_vertex(idx)) {
                            r_selection.data.indices[write_ptr++] = idx;
                        }
                    }
                    r_selection.element_count = write_ptr;
                }
            } else if constexpr (Op == QueryOp::ADD) {
                const int64_t limit = std::min<int64_t>(r_selection.capacity, max_verts);
                for (int64_t i = 0; i < limit; ++i) {
                    if (eval_vertex(i)) {
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
        }
    }
};

static_assert(sizeof(SweptSphereSurfaceQueryLogic<>) == 64, "SweptSphereSurfaceQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<SweptSphereSurfaceQueryLogic<>>, "SweptSphereSurfaceQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core