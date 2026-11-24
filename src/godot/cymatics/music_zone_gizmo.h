#pragma once

/**
 * music_zone_gizmo.h
 * Trans-Environment `IdeamGizmo` for `MusicZoneNode3D` (`src/godot/cymatics/`).
 *
 * Renders concentric harmonic acoustic zone rings and priority indicator.
 */

#include "../gizmos/ideam_gizmo.h"
#include "jukebox_resource.h"

namespace ideam::godot_ext {

class MusicZoneGizmo : public IdeamGizmo {
    GDCLASS(MusicZoneGizmo, IdeamGizmo)

protected:
    static void _bind_methods() {}

public:
    MusicZoneGizmo() = default;
    ~MusicZoneGizmo() override = default;

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override { return "MusicZoneGizmo"; }
    [[nodiscard]] int32_t get_priority() const override { return 0; }

    void build_draw_list(
        godot::Node3D *p_target,
        IdeamGizmoDrawList &r_out,
        const GizmoViewState &p_view
    ) const override;
};

} // namespace ideam::godot_ext
