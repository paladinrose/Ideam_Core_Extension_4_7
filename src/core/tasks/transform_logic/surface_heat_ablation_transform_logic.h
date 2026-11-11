#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * M33: SurfaceHeatAblationTransformLogic
 * Solves the surface thermal diffusion PDE dT/dt = kappa * Laplace(T) - cooling * (T - T_amb)
 * over 3D mesh vertices, applying inward normal ablation shrinkage and viscous gravity creep
 * wherever vertex temperature exceeds melting_temperature.
 */
struct SurfaceHeatAblationTransformLogic {
    static constexpr std::string_view ui_name = "Surface Heat Diffusion & Ablation";
    static constexpr std::string_view ui_category = "Model/SurfacePDE";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SWAP,
        MemoryView::STENCIL
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::FLOAT32
    };

    static constexpr int32_t MAX_HEAT_VERTS = 16384;

    float thermal_diffusivity = 0.35f;
    float ambient_temperature = 20.0f;
    float cooling_rate = 0.15f;
    float melting_temperature = 600.0f;
    float ablation_rate = 0.04f;
    float viscous_sag_rate = 0.02f;
    godot::Vector3 heat_source_origin = godot::Vector3(0.0f, 0.0f, 0.0f);
    float heat_source_radius = 0.0f;
    float heat_flux = 0.0f;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("thermal_diffusivity", godot::Variant::FLOAT, thermal_diffusivity);
        add_prop("ambient_temperature", godot::Variant::FLOAT, ambient_temperature);
        add_prop("cooling_rate", godot::Variant::FLOAT, cooling_rate);
        add_prop("melting_temperature", godot::Variant::FLOAT, melting_temperature);
        add_prop("ablation_rate", godot::Variant::FLOAT, ablation_rate);
        add_prop("viscous_sag_rate", godot::Variant::FLOAT, viscous_sag_rate);
        add_prop("heat_source_origin", godot::Variant::VECTOR3, heat_source_origin);
        add_prop("heat_source_radius", godot::Variant::FLOAT, heat_source_radius);
        add_prop("heat_flux", godot::Variant::FLOAT, heat_flux);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("thermal_diffusivity")) thermal_diffusivity = std::max(0.0f, static_cast<float>(p_props["thermal_diffusivity"]));
        if (p_props.has("ambient_temperature")) ambient_temperature = static_cast<float>(p_props["ambient_temperature"]);
        if (p_props.has("cooling_rate")) cooling_rate = std::max(0.0f, static_cast<float>(p_props["cooling_rate"]));
        if (p_props.has("melting_temperature")) melting_temperature = static_cast<float>(p_props["melting_temperature"]);
        if (p_props.has("ablation_rate")) ablation_rate = std::max(0.0f, static_cast<float>(p_props["ablation_rate"]));
        if (p_props.has("viscous_sag_rate")) viscous_sag_rate = std::max(0.0f, static_cast<float>(p_props["viscous_sag_rate"]));
        if (p_props.has("heat_source_origin")) heat_source_origin = p_props["heat_source_origin"];
        if (p_props.has("heat_source_radius")) heat_source_radius = std::max(0.0f, static_cast<float>(p_props["heat_source_radius"]));
        if (p_props.has("heat_flux")) heat_flux = static_cast<float>(p_props["heat_flux"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_HEAT_VERTS * sizeof(float) * 2 + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_HEAT_VERTS * sizeof(float) * 2 + 256;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 3 || !p_ctx.local_workspace) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        const GrantPartPOD& nrm_part = p_ctx.grant->parts[1];
        GrantPartPOD& temp_part = p_ctx.grant->parts[2];
        if (!vtx_part.raw_base_ptr || !nrm_part.raw_base_ptr || !temp_part.raw_base_ptr) return;

        const GrantPartPOD* idx_part = (p_ctx.grant->part_count > 3 && p_ctx.grant->parts[3].raw_base_ptr)
            ? &p_ctx.grant->parts[3]
            : nullptr;

        godot::Vector3* verts = reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr);
        const godot::Vector3* norms = reinterpret_cast<const godot::Vector3*>(nrm_part.raw_base_ptr);
        float* temp = reinterpret_cast<float*>(temp_part.raw_base_ptr);

        const int32_t raw_n = (vtx_part.selection.element_count > 0)
            ? static_cast<int32_t>(vtx_part.selection.element_count)
            : static_cast<int32_t>(vtx_part.capacity_bytes / sizeof(godot::Vector3));
        const int32_t n_verts = std::min<int32_t>({
            raw_n,
            static_cast<int32_t>(nrm_part.capacity_bytes / sizeof(godot::Vector3)),
            static_cast<int32_t>(temp_part.capacity_bytes / sizeof(float)),
            MAX_HEAT_VERTS
        });
        if (n_verts <= 0) return;

        float* lap_t = static_cast<float*>(p_ctx.local_workspace);
        float* deg_t = lap_t + MAX_HEAT_VERTS;
        for (int32_t i = 0; i < n_verts; ++i) {
            lap_t[i] = 0.0f;
            deg_t[i] = 0.0f;
        }

        if (idx_part) {
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

        const float dt = (p_ctx.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_ctx.delta)) : (1.0f / 60.0f);
        const float r2 = heat_source_radius * heat_source_radius;
        const float inv_melt = 1.0f / std::max(1.0f, melting_temperature);

        for (int32_t i = 0; i < n_verts; ++i) {
            float T = temp[i];
            if (heat_source_radius > 0.0f && std::abs(heat_flux) > 1e-6f) {
                const float d2 = (verts[i] - heat_source_origin).length_squared();
                if (d2 < r2) {
                    const float falloff = 1.0f - std::sqrt(d2) / heat_source_radius;
                    T += heat_flux * falloff * dt;
                }
            }

            const float avg_lap = (deg_t[i] > 0.0f) ? (lap_t[i] / deg_t[i]) : 0.0f;
            T += (thermal_diffusivity * avg_lap - cooling_rate * (T - ambient_temperature)) * dt;
            temp[i] = T;

            if (T > melting_temperature) {
                const float overheat = (T - melting_temperature) * inv_melt;
                // Inward Stefan ablation along -normal + downward viscous creep
                verts[i] -= norms[i] * (ablation_rate * overheat * dt);
                verts[i].y -= viscous_sag_rate * overheat * dt;
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core