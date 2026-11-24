#pragma once

/**
 * climatology_weather_gizmo.h
 * Trans-Environment `IdeamGizmo` for `ClimatologyWeatherNode3D` (`src/godot/climatology/`).
 *
 * Visualizes atmospheric domain footprint and 3D sampled wind-velocity vector arrows.
 */

#include "../gizmos/ideam_gizmo.h"
#include "climatology_weather_resource.h"

namespace ideam::godot_ext {

class ClimatologyWeatherGizmo : public IdeamGizmo {
    GDCLASS(ClimatologyWeatherGizmo, IdeamGizmo)

protected:
    static void _bind_methods() {}

public:
    ClimatologyWeatherGizmo() = default;
    ~ClimatologyWeatherGizmo() override = default;

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override { return "ClimatologyWeatherGizmo"; }
    [[nodiscard]] int32_t get_priority() const override { return 0; }

    void build_draw_list(
        godot::Node3D *p_target,
        IdeamGizmoDrawList &r_out,
        const GizmoViewState &p_view
    ) const override;
};

} // namespace ideam::godot_ext
