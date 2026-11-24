#include "music_zone_gizmo.h"

#include <algorithm>

namespace ideam::godot_ext {

bool MusicZoneGizmo::can_handle(godot::Node3D *p_target) const {
    return godot::Object::cast_to<MusicZoneNode3D>(p_target) != nullptr;
}

void MusicZoneGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    (void)p_view;
    auto *zone = godot::Object::cast_to<MusicZoneNode3D>(p_target);
    if (!zone) return;

    const float base_r = 1.5f;
    r_out.add_circle(godot::Vector3(0.0f, 0.0f, 0.0f), godot::Vector3(0.0f, 1.0f, 0.0f), base_r, 32, GIZMO_HANDLE_FEATURE_MAJOR);
    r_out.add_circle(godot::Vector3(0.0f, 0.0f, 0.0f), godot::Vector3(0.0f, 1.0f, 0.0f), base_r * 0.618f, 24, GIZMO_LINE_RULER);
    r_out.add_circle(godot::Vector3(0.0f, 0.0f, 0.0f), godot::Vector3(1.0f, 0.0f, 0.0f), base_r * 0.5f, 20, GIZMO_LINE_CLEARANCE);

    const godot::String lbl = godot::String("MusicZone (P=") + godot::String::num_int64(zone->get_zone_priority()) + godot::String(")");
    r_out.add_label_3d(godot::Vector3(0.0f, 0.85f, 0.0f), lbl, GIZMO_LABEL_ANNOTATION);
}

} // namespace ideam::godot_ext
