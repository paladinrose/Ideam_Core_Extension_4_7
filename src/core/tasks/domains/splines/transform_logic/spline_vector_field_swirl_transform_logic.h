#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

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
 * SplineVectorFieldSwirlTransformLogic<T, T_Strategy, T_View> (SPL11)
 * Evaluates a regularized Biot-Savart / Rankine vortex filament velocity field (tangential swirl,
 * radial suction, and axial flow) around a 3D spline polyline/frame buffer from SplineDOD.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SplineVectorFieldSwirlTransformLogic {
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

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline Biot-Savart Vector Field Swirl";
    static constexpr std::string_view ui_category  = "Splines/Fields";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id    = INVALID_ID; // 4B
    uint32_t position_buffer_id  = INVALID_ID; // 4B
    uint32_t spline_buffer_id    = INVALID_ID; // 4B
    uint32_t velocity_buffer_id  = INVALID_ID; // 4B -> 16B
    float swirl_strength         = 5.0f;       // 4B
    float radial_suction         = 1.5f;       // 4B
    float axial_flow             = 2.0f;       // 4B
    float core_radius            = 1.0f;       // 4B -> 32B
    bool integrate_positions     = false;      // 1B
    uint8_t reserved_padding[31] = {0};        // 31B -> Total = 64B

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
        add_buf("position_buffer_id");
        add_buf("spline_buffer_id");
        add_buf("velocity_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("swirl_strength", godot::Variant::FLOAT, 5.0f);
        add_prop("radial_suction", godot::Variant::FLOAT, 1.5f);
        add_prop("axial_flow", godot::Variant::FLOAT, 2.0f);
        add_prop("core_radius", godot::Variant::FLOAT, 1.0f);
        add_prop("integrate_positions", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (velocity_buffer_id != INVALID_ID) ? velocity_buffer_id : position_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("position_buffer_id")) position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        if (p_props.has("spline_buffer_id")) spline_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["spline_buffer_id"]));
        if (p_props.has("velocity_buffer_id")) velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["velocity_buffer_id"]));
        if (p_props.has("swirl_strength")) swirl_strength = static_cast<float>(p_props["swirl_strength"]);
        if (p_props.has("radial_suction")) radial_suction = static_cast<float>(p_props["radial_suction"]);
        if (p_props.has("axial_flow")) axial_flow = static_cast<float>(p_props["axial_flow"]);
        if (p_props.has("core_radius")) core_radius = std::max(1e-4f, static_cast<float>(p_props["core_radius"]));
        if (p_props.has("integrate_positions")) integrate_positions = static_cast<bool>(p_props["integrate_positions"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const noexcept {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            GrantPartPOD* pos_part = (position_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(position_buffer_id))
                : ((target_buffer_id != INVALID_ID)
                    ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(target_buffer_id))
                    : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr));
            const GrantPartPOD* spline_part = (spline_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(spline_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!pos_part || !pos_part->raw_base_ptr || !spline_part || !spline_part->raw_base_ptr) return;

            GrantPartPOD* vel_part = (velocity_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(velocity_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : pos_part);
            if (!vel_part || !vel_part->raw_base_ptr) vel_part = pos_part;

            const uint32_t s_stride = std::max<uint32_t>(sizeof(godot::Vector3), spline_part->element_stride);
            const int32_t s_count = (spline_part->selection.element_count > 0)
                ? static_cast<int32_t>(spline_part->selection.element_count)
                : static_cast<int32_t>(spline_part->capacity_bytes / s_stride);
            if (s_count < 2) return;

            auto spline_pt = [&](int32_t idx) noexcept -> godot::Vector3 {
                return *reinterpret_cast<const godot::Vector3*>(spline_part->raw_base_ptr + static_cast<size_t>(idx) * s_stride);
            };

            const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part->element_stride);
            const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vel_part->element_stride);
            const int64_t n_elems = (pos_part->selection.element_count > 0)
                ? pos_part->selection.element_count
                : static_cast<int64_t>(pos_part->capacity_bytes / p_stride);

            const float rc2 = core_radius * core_radius;
            const float dt = (p_ctx.delta > 0.0) ? static_cast<float>(p_ctx.delta) : (1.0f / 60.0f);

            for (int64_t i = 0; i < n_elems; ++i) {
                godot::Vector3& pos = *reinterpret_cast<godot::Vector3*>(pos_part->raw_base_ptr + static_cast<size_t>(i) * p_stride);

                float best_d2 = 1e30f;
                godot::Vector3 closest_p = spline_pt(0);
                godot::Vector3 closest_t(0.0f, 0.0f, 1.0f);

                for (int32_t k = 0; k < s_count - 1; ++k) {
                    const godot::Vector3 a = spline_pt(k);
                    const godot::Vector3 b = spline_pt(k + 1);
                    const godot::Vector3 ab = b - a;
                    const float ab2 = ab.length_squared();
                    const float u = (ab2 > 1e-10f) ? std::clamp((pos - a).dot(ab) / ab2, 0.0f, 1.0f) : 0.0f;
                    const godot::Vector3 q = a + ab * u;
                    const float d2 = (pos - q).length_squared();
                    if (d2 < best_d2) {
                        best_d2 = d2;
                        closest_p = q;
                        closest_t = (ab2 > 1e-10f) ? (ab / std::sqrt(ab2)) : godot::Vector3(0.0f, 0.0f, 1.0f);
                    }
                }

                const godot::Vector3 radial_vec = pos - closest_p;
                const float r = std::sqrt(best_d2);
                const godot::Vector3 radial_dir = (r > 1e-6f) ? (radial_vec / r) : godot::Vector3(0.0f, 0.0f, 0.0f);
                const godot::Vector3 swirl_dir = closest_t.cross(radial_dir);

                const float inv_denom = 1.0f / (best_d2 + rc2);
                const float swirl_kernel = r * inv_denom;
                const float core_kernel = rc2 * inv_denom;

                const godot::Vector3 field_vel =
                    swirl_dir * (swirl_strength * swirl_kernel) -
                    radial_dir * (radial_suction * swirl_kernel) +
                    closest_t * (axial_flow * core_kernel);

                if (integrate_positions && vel_part == pos_part) {
                    pos += field_vel * dt;
                } else if (static_cast<size_t>(i + 1) * v_stride <= vel_part->capacity_bytes) {
                    godot::Vector3& out_v = *reinterpret_cast<godot::Vector3*>(vel_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);
                    if (vel_part == pos_part) {
                        out_v = field_vel;
                    } else {
                        out_v += field_vel * dt;
                    }
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(SplineVectorFieldSwirlTransformLogic<>) == 64, "SplineVectorFieldSwirlTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SplineVectorFieldSwirlTransformLogic<>>, "SplineVectorFieldSwirlTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core