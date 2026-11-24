#pragma once

/**
 * ecology_biosphere_gizmo.h
 * Trans-Environment `IdeamGizmo` for `EcologyBiosphereNode3D` (`src/godot/ecology/`).
 *
 * Visualizes biosphere quadrat domain extents and sampled canopy-height columns.
 */

#include "../gizmos/ideam_gizmo.h"
#include "ecology_biosphere_resource.h"

namespace ideam::godot_ext {

class EcologyBiosphereGizmo : public IdeamGizmo {
    GDCLASS(EcologyBiosphereGizmo, IdeamGizmo)

protected:
    static void _bind_methods() {}

public:
    EcologyBiosphereGizmo() = default;
    ~EcologyBiosphereGizmo() override = default;

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override { return "EcologyBiosphereGizmo"; }
    [[nodiscard]] int32_t get_priority() const override { return 0; }

    void build_draw_list(
        godot::Node3D *p_target,
        IdeamGizmoDrawList &r_out,
        const GizmoViewState &p_view
    ) const override;
};

} // namespace ideam::godot_ext
