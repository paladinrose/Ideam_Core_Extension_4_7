#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

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
 * ArmorSpallPeelTransformLogic<T> (M25)
 * Curls outer surface vertices (and optional vertex normals) outward around an impact origin
 * via Rodrigues rotation, creating peeled petal / spall deformations along UV or hard-normal seams.
 */
template <typename T = godot::Vector3>
struct alignas(64) ArmorSpallPeelTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    // Restricted to floating-point 3D vectors so dot()/cross()/normalized() are always valid
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Armor Spall & Petal Peel";
    static constexpr std::string_view ui_category  = "Model/Deform";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id      = INVALID_ID; // 4B: Primary target vertex buffer (SwapView or SingleElementView)
    uint32_t normal_buffer_id      = INVALID_ID; // 4B: Optional vertex normal buffer rotated in tandem with petals
    uint32_t seam_weight_buffer_id = INVALID_ID; // 4B: Optional per-vertex float seam/peel weight buffer in [0, 1]

    // Stored as godot::Vector3 so TransformLogicSubRegistry can instantiate the class body
    // across all 17 MemoryTypes (bool, int32_t, void*, etc.) before checking required_types.
    godot::Vector3 impact_origin{0.0f, 0.0f, 0.0f}; // 12B: Local-space impact center
    godot::Vector3 impact_normal{0.0f, 1.0f, 0.0f}; // 12B: Outward surface normal at impact point

    float peel_radius            = 1.5f;  // 4B: Radial influence radius R (meters)
    float max_curl_angle_radians = 1.35f; // 4B: Peak outward Rodrigues curl angle (radians)
    float outward_lift           = 0.25f; // 4B: Axial outward lift along impact_normal (meters)
    float lobe_modulation        = 0.35f; // 4B: Azimuthal petal-seam modulation depth in [0, 1]
    uint8_t petal_lobes          = 6;     // 1B: Azimuthal petal count (0 = axisymmetric curl)
    uint8_t reserved_padding[11] = {0};   // 11B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary vtx_prop;
        vtx_prop["name"] = "vertex_buffer_id";
        vtx_prop["type"] = godot::Variant::INT;
        vtx_prop["hint_string"] = "buffer_option";
        props.push_back(vtx_prop);

        godot::Dictionary nrm_prop;
        nrm_prop["name"] = "normal_buffer_id";
        nrm_prop["type"] = godot::Variant::INT;
        nrm_prop["hint_string"] = "buffer_option";
        props.push_back(nrm_prop);

        godot::Dictionary seam_prop;
        seam_prop["name"] = "seam_weight_buffer_id";
        seam_prop["type"] = godot::Variant::INT;
        seam_prop["hint_string"] = "buffer_option";
        props.push_back(seam_prop);

        godot::Dictionary orig_prop;
        orig_prop["name"] = "impact_origin";
        orig_prop["type"] = godot::Variant::VECTOR3;
        orig_prop["hint_string"] = "suffix:m";
        props.push_back(orig_prop);

        godot::Dictionary dir_prop;
        dir_prop["name"] = "impact_normal";
        dir_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(dir_prop);

        godot::Dictionary rad_prop;
        rad_prop["name"] = "peel_radius";
        rad_prop["type"] = godot::Variant::FLOAT;
        rad_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rad_prop["hint_string"] = "0.01,25.0,0.01,suffix:m";
        props.push_back(rad_prop);

        godot::Dictionary curl_prop;
        curl_prop["name"] = "max_curl_angle_radians";
        curl_prop["type"] = godot::Variant::FLOAT;
        curl_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        curl_prop["hint_string"] = "0.0,3.14159,0.01,suffix:rad";
        props.push_back(curl_prop);

        godot::Dictionary lift_prop;
        lift_prop["name"] = "outward_lift";
        lift_prop["type"] = godot::Variant::FLOAT;
        lift_prop["hint_string"] = "suffix:m";
        props.push_back(lift_prop);

        godot::Dictionary lobes_prop;
        lobes_prop["name"] = "petal_lobes";
        lobes_prop["type"] = godot::Variant::INT;
        lobes_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        lobes_prop["hint_string"] = "0,32,1";
        props.push_back(lobes_prop);

        godot::Dictionary mod_prop;
        mod_prop["name"] = "lobe_modulation";
        mod_prop["type"] = godot::Variant::FLOAT;
        mod_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        mod_prop["hint_string"] = "0.0,1.0,0.01";
        props.push_back(mod_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("normal_buffer_id")) {
            normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        }
        if (p_props.has("seam_weight_buffer_id")) {
            seam_weight_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["seam_weight_buffer_id"]));
        }
        if (p_props.has("impact_origin")) {
            impact_origin = static_cast<godot::Vector3>(p_props["impact_origin"]);
        }
        if (p_props.has("impact_normal")) {
            const godot::Vector3 n = static_cast<godot::Vector3>(p_props["impact_normal"]);
            impact_normal = (n.length_squared() > 1e-8f) ? n.normalized() : godot::Vector3(0.0f, 1.0f, 0.0f);
        }
        if (p_props.has("peel_radius")) {
            peel_radius = std::max(1e-4f, static_cast<float>(p_props["peel_radius"]));
        }
        if (p_props.has("max_curl_angle_radians")) {
            max_curl_angle_radians = static_cast<float>(p_props["max_curl_angle_radians"]);
        }
        if (p_props.has("outward_lift")) {
            outward_lift = static_cast<float>(p_props["outward_lift"]);
        }
        if (p_props.has("petal_lobes")) {
            petal_lobes = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["petal_lobes"]), 0, 255));
        }
        if (p_props.has("lobe_modulation")) {
            lobe_modulation = std::clamp(static_cast<float>(p_props["lobe_modulation"]), 0.0f, 1.0f);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const GrantPartPOD* vert_part = p_context.get_grant_part(vertex_buffer_id);
            if (!vert_part || !vert_part->raw_base_ptr) return;

            T* __restrict normals = nullptr;
            int64_t norm_cap = 0;
            if (normal_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* norm_part = p_context.get_grant_part(normal_buffer_id)) {
                    if (norm_part->raw_base_ptr) {
                        normals  = reinterpret_cast<T*>(norm_part->raw_base_ptr);
                        norm_cap = static_cast<int64_t>(norm_part->capacity_bytes / sizeof(T));
                    }
                }
            }

            const float* __restrict seam_weights = nullptr;
            int64_t seam_cap = 0;
            if (seam_weight_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* seam_part = p_context.get_grant_part(seam_weight_buffer_id)) {
                    if (seam_part->raw_base_ptr) {
                        seam_weights = reinterpret_cast<const float*>(seam_part->raw_base_ptr);
                        seam_cap     = static_cast<int64_t>(seam_part->capacity_bytes / sizeof(float));
                    }
                }
            }

            const float r_peel = std::max(1e-4f, peel_radius);
            const float r_sq   = r_peel * r_peel;
            const float inv_r  = 1.0f / r_peel;

            const T n_dir = (impact_normal.length_squared() > 1e-8f)
                ? impact_normal.normalized()
                : T(0.0f, 1.0f, 0.0f);
            const T ref_up = (std::abs(n_dir.y) < 0.99f)
                ? T(0.0f, 1.0f, 0.0f)
                : T(1.0f, 0.0f, 0.0f);
            const T tangent_u = n_dir.cross(ref_up).normalized();
            const T tangent_v = n_dir.cross(tangent_u).normalized();

            auto process_vertex = [&](int64_t idx) noexcept {
                T pos = _read_view(p_view, idx);
                const T rel = pos - impact_origin;
                const float dist_sq = rel.length_squared();
                if (dist_sq <= 1e-10f || dist_sq >= r_sq) return;

                // Project onto tangent plane of impact
                const float axial = rel.dot(n_dir);
                const T radial_vec = rel - (n_dir * axial);
                const float radial_len = radial_vec.length();
                if (radial_len <= 1e-5f) return;

                const T radial_dir = radial_vec * (1.0f / radial_len);
                const T hinge_axis = n_dir.cross(radial_dir).normalized();

                // Normalized radial coordinate u in [0, 1] (0 = impact center, 1 = outer hinge rim)
                const float u = std::clamp(radial_len * inv_r, 0.0f, 1.0f);
                float lobe_factor = 1.0f;
                if (petal_lobes > 0) {
                    const float ang = std::atan2(radial_dir.dot(tangent_v), radial_dir.dot(tangent_u));
                    lobe_factor = 1.0f - lobe_modulation * (0.5f - 0.5f * std::cos(ang * static_cast<float>(petal_lobes)));
                }
                if (seam_weights && idx < seam_cap) {
                    lobe_factor *= std::clamp(seam_weights[idx], 0.0f, 1.0f);
                }

                const float one_minus_u = 1.0f - u;
                const float weight = (one_minus_u * one_minus_u) * lobe_factor;
                const float theta  = max_curl_angle_radians * weight;

                // Pivot around the outer rim hinge point on the impact tangent plane
                const T hinge_pivot = impact_origin + (radial_dir * r_peel) + (n_dir * axial);
                const T arm = pos - hinge_pivot;
                pos = hinge_pivot + _rodrigues_rotate(arm, hinge_axis, theta) + (n_dir * (outward_lift * weight));
                _write_view(p_view, idx, pos);

                if (normals && idx < norm_cap) {
                    normals[idx] = _rodrigues_rotate(normals[idx], hinge_axis, theta).normalized();
                }
            };

            const MemoryBufferSelectionPOD& sel = vert_part->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                for (int64_t i = 0; i < sel.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) process_vertex(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    process_vertex(sel.data.indices[i]);
                }
            } else {
                const int64_t end = sel.start_index + sel.element_count;
                for (int64_t i = sel.start_index; i < end; ++i) {
                    process_vertex(i);
                }
            }
        }
    }

private:
    static inline godot::Vector3 _rodrigues_rotate(const godot::Vector3& p_v, const godot::Vector3& p_axis, float p_theta) noexcept {
        const float c = std::cos(p_theta);
        const float s = std::sin(p_theta);
        return (p_v * c) + (p_axis.cross(p_v) * s) + (p_axis * (p_axis.dot(p_v) * (1.0f - c)));
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _read_view(const T_View& p_view, int64_t idx) const noexcept {
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<T>(p_view[idx].read());
        } else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<T>(p_view[idx].load());
        } else if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else {
            return static_cast<T>(p_view[idx]);
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void _write_view(const T_View& p_view, int64_t idx, const T& value) const noexcept {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            *reinterpret_cast<T*>(p_view[idx]) = value;
        } else if constexpr (requires { p_view[idx].write(); }) {
            p_view[idx] = value;
        } else if constexpr (requires { p_view[idx].store(value); }) {
            p_view[idx].store(value);
        } else {
            (void)(p_view[idx] = value);
        }
    }
};

static_assert(sizeof(ArmorSpallPeelTransformLogic<godot::Vector3>) == 64, "ArmorSpallPeelTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ArmorSpallPeelTransformLogic<godot::Vector3>>, "ArmorSpallPeelTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core