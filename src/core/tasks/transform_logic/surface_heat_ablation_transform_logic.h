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
 * SurfaceHeatAblationTransformLogic<T> (M33)
 * Solves the surface thermal diffusion PDE dT/dt = kappa * Laplace(T) - cooling * (T - T_amb)
 * over 3D mesh vertices, applying inward normal ablation shrinkage and viscous gravity creep
 * wherever vertex temperature exceeds melting_temperature.
 */
template <typename T = godot::Vector3>
struct alignas(64) SurfaceHeatAblationTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    static constexpr int32_t MAX_HEAT_VERTS = 16384;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::FLOAT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_HEAT_VERTS * sizeof(float) * 2 + 256;

    static constexpr std::string_view display_name = "Surface Heat Diffusion & Ablation";
    static constexpr std::string_view ui_category  = "Model/SurfacePDE";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id      = INVALID_ID; // 4B: Primary target vertex buffer (Vector3)
    uint32_t normal_buffer_id      = INVALID_ID; // 4B: Vertex normal buffer (Vector3)
    uint32_t temperature_buffer_id = INVALID_ID; // 4B: Per-vertex float temperature buffer
    uint32_t index_buffer_id       = INVALID_ID; // 4B: Optional triangle index buffer (int32_t triplets)

    godot::Vector3 heat_source_origin{0.0f, 0.0f, 0.0f}; // 12B: Local-space thermal source center

    float thermal_diffusivity   = 0.35f;  // 4B: Surface heat diffusivity kappa
    float ambient_temperature   = 20.0f;  // 4B: Ambient equilibrium temperature
    float cooling_rate          = 0.15f;  // 4B: Newton cooling coefficient
    float melting_temperature   = 600.0f; // 4B: Phase-change ablation threshold temperature
    float ablation_rate         = 0.04f;  // 4B: Inward normal recession speed per unit overheat
    float viscous_sag_rate      = 0.02f;  // 4B: Downward gravity creep speed per unit overheat
    float heat_source_radius    = 0.0f;   // 4B: Thermal source radius (0 = disabled)
    float heat_flux             = 0.0f;   // 4B: Thermal energy injection rate (deg/s)
    uint8_t reserved_padding[4] = {0};    // 4B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (const GrantPartPOD* vtx_part = p_context.get_grant_part(vertex_buffer_id)) {
            if (vtx_part->selection.capacity > 0) {
                const size_t n = std::min<size_t>(static_cast<size_t>(vtx_part->selection.capacity), MAX_HEAT_VERTS);
                return n * sizeof(float) * 2 + 256;
            }
        }
        return transient_workspace_bytes;
    }

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

        godot::Dictionary tmp_prop;
        tmp_prop["name"] = "temperature_buffer_id";
        tmp_prop["type"] = godot::Variant::INT;
        tmp_prop["hint_string"] = "buffer_option";
        props.push_back(tmp_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary diff_prop;
        diff_prop["name"] = "thermal_diffusivity";
        diff_prop["type"] = godot::Variant::FLOAT;
        props.push_back(diff_prop);

        godot::Dictionary amb_prop;
        amb_prop["name"] = "ambient_temperature";
        amb_prop["type"] = godot::Variant::FLOAT;
        props.push_back(amb_prop);

        godot::Dictionary cool_prop;
        cool_prop["name"] = "cooling_rate";
        cool_prop["type"] = godot::Variant::FLOAT;
        props.push_back(cool_prop);

        godot::Dictionary melt_prop;
        melt_prop["name"] = "melting_temperature";
        melt_prop["type"] = godot::Variant::FLOAT;
        props.push_back(melt_prop);

        godot::Dictionary abl_prop;
        abl_prop["name"] = "ablation_rate";
        abl_prop["type"] = godot::Variant::FLOAT;
        props.push_back(abl_prop);

        godot::Dictionary sag_prop;
        sag_prop["name"] = "viscous_sag_rate";
        sag_prop["type"] = godot::Variant::FLOAT;
        props.push_back(sag_prop);

        godot::Dictionary orig_prop;
        orig_prop["name"] = "heat_source_origin";
        orig_prop["type"] = godot::Variant::VECTOR3;
        orig_prop["hint_string"] = "suffix:m";
        props.push_back(orig_prop);

        godot::Dictionary rad_prop;
        rad_prop["name"] = "heat_source_radius";
        rad_prop["type"] = godot::Variant::FLOAT;
        rad_prop["hint_string"] = "suffix:m";
        props.push_back(rad_prop);

        godot::Dictionary flx_prop;
        flx_prop["name"] = "heat_flux";
        flx_prop["type"] = godot::Variant::FLOAT;
        props.push_back(flx_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (vertex_buffer_id != INVALID_ID) ? vertex_buffer_id : temperature_buffer_id;
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
        if (p_props.has("temperature_buffer_id")) {
            temperature_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["temperature_buffer_id"]));
        }
        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("thermal_diffusivity")) {
            thermal_diffusivity = std::max(0.0f, static_cast<float>(p_props["thermal_diffusivity"]));
        }
        if (p_props.has("ambient_temperature")) {
            ambient_temperature = static_cast<float>(p_props["ambient_temperature"]);
        }
        if (p_props.has("cooling_rate")) {
            cooling_rate = std::max(0.0f, static_cast<float>(p_props["cooling_rate"]));
        }
        if (p_props.has("melting_temperature")) {
            melting_temperature = static_cast<float>(p_props["melting_temperature"]);
        }
        if (p_props.has("ablation_rate")) {
            ablation_rate = std::max(0.0f, static_cast<float>(p_props["ablation_rate"]));
        }
        if (p_props.has("viscous_sag_rate")) {
            viscous_sag_rate = std::max(0.0f, static_cast<float>(p_props["viscous_sag_rate"]));
        }
        if (p_props.has("heat_source_origin")) {
            heat_source_origin = static_cast<godot::Vector3>(p_props["heat_source_origin"]);
        }
        if (p_props.has("heat_source_radius")) {
            heat_source_radius = std::max(0.0f, static_cast<float>(p_props["heat_source_radius"]));
        }
        if (p_props.has("heat_flux")) {
            heat_flux = static_cast<float>(p_props["heat_flux"]);
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
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, float>) {
            if (!p_context.local_workspace) return;

            const GrantPartPOD* vtx_part  = p_context.get_grant_part(vertex_buffer_id);
            const GrantPartPOD* nrm_part  = p_context.get_grant_part(normal_buffer_id);
            const GrantPartPOD* temp_part = p_context.get_grant_part(temperature_buffer_id);
            if (!vtx_part || !nrm_part || !temp_part || !vtx_part->raw_base_ptr || !nrm_part->raw_base_ptr || !temp_part->raw_base_ptr) return;

            const GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID) ? p_context.get_grant_part(index_buffer_id) : nullptr;

            godot::Vector3* __restrict verts       = reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr);
            const godot::Vector3* __restrict norms = reinterpret_cast<const godot::Vector3*>(nrm_part->raw_base_ptr);
            float* __restrict temp                 = reinterpret_cast<float*>(temp_part->raw_base_ptr);

            const int32_t raw_n = (vtx_part->selection.element_count > 0)
                ? static_cast<int32_t>(vtx_part->selection.element_count)
                : static_cast<int32_t>(vtx_part->capacity_bytes / sizeof(godot::Vector3));
            const int32_t n_verts = std::min<int32_t>({
                raw_n,
                static_cast<int32_t>(nrm_part->capacity_bytes / sizeof(godot::Vector3)),
                static_cast<int32_t>(temp_part->capacity_bytes / sizeof(float)),
                MAX_HEAT_VERTS
            });
            if (n_verts <= 0) return;

            float* lap_t = static_cast<float*>(p_context.local_workspace);
            float* deg_t = lap_t + n_verts;
            for (int32_t i = 0; i < n_verts; ++i) {
                lap_t[i] = 0.0f;
                deg_t[i] = 0.0f;
            }

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

                    const float t0 = temp[i0], t1 = temp[i1], t2 = temp[i2];
                    lap_t[i0] += (t1 - t0) + (t2 - t0); deg_t[i0] += 2.0f;
                    lap_t[i1] += (t0 - t1) + (t2 - t1); deg_t[i1] += 2.0f;
                    lap_t[i2] += (t0 - t2) + (t1 - t2); deg_t[i2] += 2.0f;
                }
            }

            const float dt = (p_context.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_context.delta)) : (1.0f / 60.0f);
            const float r2 = heat_source_radius * heat_source_radius;
            const float inv_melt = 1.0f / std::max(1.0f, melting_temperature);

            for (int32_t i = 0; i < n_verts; ++i) {
                float cur_temp = temp[i];
                if (heat_source_radius > 0.0f && std::abs(heat_flux) > 1e-6f) {
                    const float d2 = (verts[i] - heat_source_origin).length_squared();
                    if (d2 < r2) {
                        const float falloff = 1.0f - std::sqrt(d2) / heat_source_radius;
                        cur_temp += heat_flux * falloff * dt;
                    }
                }

                const float avg_lap = (deg_t[i] > 0.0f) ? (lap_t[i] / deg_t[i]) : 0.0f;
                cur_temp += (thermal_diffusivity * avg_lap - cooling_rate * (cur_temp - ambient_temperature)) * dt;
                temp[i] = cur_temp;

                if (cur_temp > melting_temperature) {
                    const float overheat = (cur_temp - melting_temperature) * inv_melt;
                    verts[i] -= norms[i] * (ablation_rate * overheat * dt);
                    verts[i].y -= viscous_sag_rate * overheat * dt;
                }
            }
        }
    }
};

static_assert(sizeof(SurfaceHeatAblationTransformLogic<godot::Vector3>) == 64, "SurfaceHeatAblationTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SurfaceHeatAblationTransformLogic<godot::Vector3>>, "SurfaceHeatAblationTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core